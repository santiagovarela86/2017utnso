//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//250 KER A MEM - ENVIO PROGRAMA A MEMORIA
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL
//510 CPU A MEM - SOLICITUD INSTRUCCION
//511 CPU A MEM - ASIGNAR VARIABLE
//512 CPU A MEM - DEFINIR VARIABLE
//513 CPU A MEM - DEREFERENCIAR VARIABLE
//605 KER A MEM - RESERVAR MEMORIA HEAP
//605 MEM A KER - PAGINA HEAP NUEVA
//606 MEM A KER - USAR PAGINA HEAP EXISTENTE
//601 CPU A MEM - SOLICITAR POSICION DE VARIABLE
//610 KER A MEM - REORDENAR METADATA
//612 KER A MEM - ENVIO DE CANT MAXIMA DE PAGINAS DE STACK POR PROCESO
//616 KER A MEM - FINALIZAR PROGRAMA

#include <pthread.h>
#include "configuracion.h"
#include "memoria.h"
#include <errno.h>
#include "helperFunctions.h"
#include <time.h>
#include <semaphore.h>
#include "cache.h"
#include <sys/time.h>

int conexionesKernel = 0;
int conexionesCPU = 0;
int tiempo_retardo;

pthread_mutex_t mutex_tiempo_retardo;
pthread_mutex_t mutex_estructuras_administrativas;
pthread_mutex_t mutex_bloque_memoria;

t_list* tabla_paginas;
t_list* tabla_cache;
//t_list* paginasCodigoProceso;
int stack_size = 0;
char* bloque_memoria;
Memoria_Config* configuracion;
t_max_cantidad_paginas* tamanio_maximo;
t_list* lista_paginas_stack;
bool cache_habilitada = false;

int socketKernel;
struct sockaddr_in direccionKernel;
socklen_t length = sizeof direccionKernel;

int socketMemoria;
struct sockaddr_in direccionMemoria;
sem_t semaforoKernel;

t_log* logger_memoria;

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	t_log* logger_memoria = log_create("memoria_log_interno.log", "memoria_log_interno",true, LOG_LEVEL_INFO);

	char * pathConfig = argv[1];
	inicializarEstructuras(pathConfig);

	imprimirConfiguracion(configuracion);

	/*PRUEBAS FHASH*/
	//pruebas_f_hash();

	pthread_t thread_consola;
	pthread_t thread_kernel;
	pthread_t thread_cpu;

	creoThread(&thread_consola, inicializar_consola, configuracion);
	creoThread(&thread_kernel, hilo_conexiones_kernel, NULL);

	sem_wait(&semaforoKernel);

	creoThread(&thread_cpu, hilo_conexiones_cpu, NULL);

	pthread_join(thread_consola, NULL);
	pthread_join(thread_kernel, NULL);
	pthread_join(thread_cpu, NULL);

	liberarEstructuras();

	return EXIT_SUCCESS;
}

void inicializarEstructuras(char * pathConfig){
	pthread_mutex_init(&mutex_tiempo_retardo, NULL);
	pthread_mutex_init(&mutex_estructuras_administrativas, NULL);
	pthread_mutex_init(&mutex_bloque_memoria, NULL);

	configuracion = leerConfiguracion(pathConfig);

	//bloque_memoria = string_new();

	//Alocacion de bloque de memoria contigua
	//Seria el tamanio del marco * la cantidad de marcos

	bloque_memoria = malloc(configuracion->marcos * configuracion->marco_size);
	if (bloque_memoria == NULL){
		perror("No se pudo reservar el bloque de memoria del Sistema\n");
	}

	pthread_mutex_lock(&mutex_tiempo_retardo);
	tiempo_retardo = configuracion->retardo_memoria;
	cache_habilitada = configuracion->entradas_cache > 0 ? true : false;
	pthread_mutex_unlock(&mutex_tiempo_retardo);

	pthread_mutex_lock(&mutex_estructuras_administrativas);
	inicializar_tabla_paginas(configuracion);
	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	tabla_cache = list_create();
	lista_paginas_stack = list_create();
	//paginasCodigoProceso = list_create();

	sem_init(&semaforoKernel, 0, 0);

	creoSocket(&socketMemoria, &direccionMemoria, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketMemoria, &direccionMemoria);
	escuchoSocket(&socketMemoria);
}

void liberarEstructuras(){
	pthread_mutex_destroy(&mutex_tiempo_retardo);
	pthread_mutex_destroy(&mutex_estructuras_administrativas);
	pthread_mutex_destroy(&mutex_bloque_memoria);

	list_destroy_and_destroy_elements(tabla_cache, free);
	list_destroy_and_destroy_elements(lista_paginas_stack, free);
	//list_destroy_and_destroy_elements(paginasCodigoProceso, free);

	free(configuracion);
	free(tamanio_maximo);
	free(bloque_memoria);

	sem_destroy(&semaforoKernel);

	log_destroy(logger_memoria);

	shutdown(socketMemoria, 0);
	close(socketMemoria);
}

void * hilo_conexiones_kernel(){

	socketKernel = accept(socketMemoria, (struct sockaddr *) &direccionKernel, &length);

	if (socketKernel > 0) {
		sem_post(&semaforoKernel);
		printf("%s:%d conectado\n", inet_ntoa(direccionKernel.sin_addr), ntohs(direccionKernel.sin_port));
		handShakeListen(&socketKernel, "100", "201", "299", "Kernel", configuracion->marco_size);
		char message[MAXBUF];

		int result = recv(socketKernel, message, sizeof(message), 0);

		while (result > 0) {

			retardo_acceso_memoria(); //SERIA MEJOR UNIFICAR EL RETARDO EN UN UNICO ACCEDER BLOQUE MEMORIA

			char**mensajeDelKernel = string_split(message, ";");
			int operacion = atoi(mensajeDelKernel[0]);
			int pid;

			switch(operacion){

				case 250:
					;
					pid = atoi(mensajeDelKernel[1]);
					char * codigo_programa = mensajeDelKernel[2];
					iniciarPrograma(pid, codigo_programa);
					break;

				case 605:
					;
					pid = atoi(mensajeDelKernel[1]);
					int paginaActual = atoi(mensajeDelKernel[2]);
					int bytes = atoi(mensajeDelKernel[3]);
					crearPaginaHeap(pid, paginaActual, bytes); //ESTO USA ALMACENAR BYTES EN PAGINA
					break;

				case 607:
					;
					pid = atoi(mensajeDelKernel[1]);
					int paginaExistente = atoi(mensajeDelKernel[2]);
					int direccionAUsar = atoi(mensajeDelKernel[3]);
					int bytesSolicitados = atoi(mensajeDelKernel[4]);
					usarPaginaHeap(pid, paginaExistente, direccionAUsar, bytesSolicitados); //ESTO USA SOLICITAR DATOS Y ALMACENAR BYTES EN PAGINA
					break;

				case 610:
					;
					pid = atoi(mensajeDelKernel[1]);
					int paginaConDosBloques = atoi(mensajeDelKernel[2]);
					int direccionMeta1 = atoi(mensajeDelKernel[3]);
					int direccionMeta2 = atoi(mensajeDelKernel[4]);
					int bytesAUnir = atoi(mensajeDelKernel[5]);
					reordenarMetadata(pid, paginaConDosBloques, direccionMeta1, direccionMeta2, bytesAUnir); //ESTO USA SOLICITAR DATOS Y ALMACENAR BYTES EN PAGINA
					break;

				case 705:
					;
					int pid = atoi(mensajeDelKernel[1]);
					int pagina = atoi(mensajeDelKernel[2]);
					int direccion = atoi(mensajeDelKernel[3]);
					eliminarMemoriaHeap(pid, pagina, direccion); //ESTO USA SOLICITAR DATOS Y ALMACENAR BYTES EN PAGINA
					break;

				case 612:
					stack_size = atoi(mensajeDelKernel[1]);
					break;

				case 616:
					pid = atoi(mensajeDelKernel[1]);
					finalizar_programa(pid); //ESTO NO NECESITA NI SOLICITAR DATOS NI ALMACENAR BYTES
					break;
			}

			result = recv(socketKernel, message, sizeof(message), 0);
		}

		if (result <= 0) {
			printf("Se desconecto el Kernel\n");
			exit(errno);
		}
	} else {
		perror("Fallo en el manejo del hilo Kernel\n");
		return EXIT_FAILURE;
	}

	shutdown(socketKernel, 0);
	close(socketKernel);

	return EXIT_SUCCESS;
}

void reordenarMetadata(int pid, int paginaConDosBloques, int direccionMeta1, int direccionMeta2, int bytesSolicitados){
	heapMetadata * meta1 = malloc(sizeof(heapMetadata));
	int offset1 = direccionMeta1 % configuracion->marco_size;

	heapMetadata * meta2 = malloc(sizeof(heapMetadata));
	int offset2 = direccionMeta2 % configuracion->marco_size;

	meta1 = solicitar_datos_de_pagina(pid, paginaConDosBloques, offset1, sizeof(heapMetadata));
	meta2 = solicitar_datos_de_pagina(pid, paginaConDosBloques, offset2, sizeof(heapMetadata));

	//SI ENTRA JUSTO
	if ((meta1->size + meta2->size + sizeof(heapMetadata)) == bytesSolicitados){
		//TENGO QUE ELIMINAR LA SEGUNDA METADATA Y EDITAR LA PRIMERA
		meta1->isFree = false;
		meta1->size = meta1->size + meta2->size + sizeof(heapMetadata);

		//NO SE SI HACE FALTA BORRAR LA METADATA EN MEMORIA YA QUE AL EDITAR EL PRIMER
		//METADATA CON EL NUEVO ESPACIO LIBRE LUEGO LA APLICACION
		//ESCRIBIRA ENCIMA DE ESTA METADATA ////VALIDAR QUE SEA COHERENTE
		//???
		//free(meta2); //PINCHA
	}else{
		//SI QUEDA ESPACIO LIBRE INCLUYENDO EL QUE OCUPA EL META FREE
		//TENGO QUE EDITAR EL PRIMERO Y EDITAR EL SEGUNDO

		int nuevaDireccionMeta2 = direccionMeta1 + sizeof(heapMetadata) + bytesSolicitados;

		meta2->isFree = true;
		meta2->size = meta1->size + meta2->size - bytesSolicitados;

		//memcpy(&bloque_memoria[nuevaDireccionMeta2], meta2, sizeof(heapMetadata));

		offset2 = nuevaDireccionMeta2 % configuracion->marco_size;

		almacenarBytesEnPagina(pid, paginaConDosBloques, offset2, sizeof(heapMetadata), meta2);

		meta1->isFree = false;
		meta1->size = bytesSolicitados;
	}

	almacenarBytesEnPagina(pid, paginaConDosBloques, offset1, sizeof(heapMetadata), meta1);

	enviarMensaje(&socketKernel, serializarMensaje(1, 611));

	//free(meta1);
	//free(meta2);
}

void eliminarMemoriaHeap(int pid, int pagina, int direccion){
	int posicionMetadata = direccion - sizeof(heapMetadata);
	int offsetMeta = posicionMetadata % configuracion->marco_size;

	heapMetadata * metadataALiberar = solicitar_datos_de_pagina(pid, pagina, offsetMeta, sizeof(heapMetadata));

	//heapMetadata * metadataALiberar = (heapMetadata *) (bloque_memoria + posicionMetadata);

	metadataALiberar->isFree = true;

	almacenarBytesEnPagina(pid, pagina, offsetMeta, sizeof(heapMetadata), metadataALiberar);

	enviarMensaje(&socketKernel, serializarMensaje(1, 706));
}

void iniciarPrograma(int pid, char * codigo_programa) {

	printf("INICIAR PROGRAMA %d\n", pid);
	puts("");

	int paginas = ceiling(strlen(codigo_programa), configuracion->marco_size);

	void imprimoInfo(t_pagina_invertida * elem){
		printf("Se asigno el marco %d al PID %d para almacenar codigo del programa\n", elem->nro_marco, elem->pid);
		puts("");
	}

	void agregoACache(t_pagina_invertida * elem){
		pthread_mutex_lock(&mutex_estructuras_administrativas);
		almacenar_pagina_en_cache_para_pid(pid, elem);
		pthread_mutex_unlock(&mutex_estructuras_administrativas);
	}

	if (strlen(codigo_programa) <=
			(configuracion->marcos - tamanio_maximo->maxima_cant_paginas_administracion)
				* configuracion->marco_size){
//	if (string_length(bloque_memoria) == 0 || string_length(codigo_programa) <= string_length(bloque_memoria)) {

		pthread_mutex_lock(&mutex_estructuras_administrativas);
		t_list * listaPaginasPrograma = list_create();
		//t_pagina_invertida* pagina = grabar_en_bloque(pid, paginas, codigo_programa);
		listaPaginasPrograma = grabar_en_bloque(pid, codigo_programa);
		pthread_mutex_unlock(&mutex_estructuras_administrativas);

		list_iterate(listaPaginasPrograma, (void *) imprimoInfo);

		if (cache_habilitada) list_iterate(listaPaginasPrograma, (void *) agregoACache);

		t_pagina_invertida * primeraPagina = list_get(listaPaginasPrograma, 0);

		enviarMensaje(&socketKernel, serializarMensaje(3, 203, obtener_inicio_pagina(primeraPagina), paginas));
	}else {
		//Si el codigo del programa supera el tamanio del bloque
		//Le aviso al Kernel que no puede reservar el espacio para el programa
		enviarMensaje(&socketKernel, serializarMensaje(1, 298));
	}
}

void usarPaginaHeap(int pid, int paginaExistente, int direccion, int bytesPedidos){
	//printf("Me pasaron: PID: %d, Pagina: %d, Bytes: %d\n", pid, paginaExistente, bytesPedidos);
	//BUSCO LA PAGINA EXISTENTE
	t_pagina_invertida * pagina = buscar_pagina_para_consulta(pid, paginaExistente);
	printf("USO UNA PAGINA EXISTENTE\n");
	printf("Marco: %d, Pagina: %d, Pid: %d\n", pagina->nro_marco, pagina->nro_pagina, pagina->pid);
	puts(" ");

	int offsetMeta = direccion % configuracion->marco_size;

	heapMetadata * metadata = solicitar_datos_de_pagina(pid, paginaExistente, offsetMeta, sizeof(heapMetadata));

	//heapMetadata * metadata = (heapMetadata *) (bloque_memoria + direccion);

	//SI ENTRA JUSTO
	if (bytesPedidos == metadata->size){
		metadata->isFree = false;
	}else{
		//SI QUEDA ESPACIO LIBRE INCLUYENDO EL QUE OCUPA EL META FREE
		heapMetadata * metadataNuevo = malloc(sizeof(heapMetadata));
		metadataNuevo->isFree = true;
		metadataNuevo->size = metadata->size - bytesPedidos - sizeof(heapMetadata);

		metadata->isFree = false;
		metadata->size = bytesPedidos;

		//memcpy(&bloque_memoria[direccion+sizeof(heapMetadata)+metadata->size], metadataNuevo, sizeof(heapMetadata));
		//free(metadataNuevo);

		almacenarBytesEnPagina(pid, paginaExistente, offsetMeta + sizeof(heapMetadata) + metadata->size, sizeof(heapMetadata), metadataNuevo);
	}

	almacenarBytesEnPagina(pid, paginaExistente, offsetMeta, sizeof(heapMetadata), metadata);

	//printf("Envio PID: %d, Pagina: %d, Direccion: %d, Tamaño: %d\n", pagina->pid, pagina->nro_pagina, direccion, bytesPedidos);

	enviarMensaje(&socketKernel, serializarMensaje(1, 608));

	//free(metadata);

}

void crearPaginaHeap(int pid, int paginaActual, int bytesPedidos){

	t_pagina_invertida * pagina = buscar_pagina_para_insertar(pid, paginaActual);

	if(pagina != NULL){
		pagina->nro_pagina = paginaActual;
		pagina->pid = pid;

		int dirInicioPagina = obtener_inicio_pagina(pagina);
		int freeSpace = configuracion->marco_size - sizeof(heapMetadata) * 2;
		int direccionFree = dirInicioPagina + sizeof(heapMetadata);

		heapMetadata * meta_used = malloc(sizeof(heapMetadata));
		meta_used->isFree = false;
		meta_used->size = bytesPedidos;

		almacenarBytesEnPagina(pagina->pid, pagina->nro_pagina, 0, sizeof(heapMetadata), meta_used);

		heapMetadata * meta_free = malloc(sizeof(heapMetadata));
		meta_free->isFree = true;
		meta_free->size = freeSpace - bytesPedidos;

		almacenarBytesEnPagina(pagina->pid, pagina->nro_pagina, sizeof(heapMetadata) + bytesPedidos, sizeof(heapMetadata), meta_free);

		enviarMensaje(&socketKernel, serializarMensaje(3, 605, direccionFree, meta_free->size));

		//free(meta_used);
		//free(meta_free);

		printf("Se creo una nueva pagina de Heap, PID: %d, Pagina: %d, Marco: %d, Free Space: %d, Direccion Puntero: %d\n",
		pagina->pid, pagina->nro_pagina, pagina->nro_marco, meta_free->size, direccionFree);
		printf("\n");

	}else{
		enviarMensaje(&socketKernel, serializarMensaje(1, 655));
	}
}

void * hilo_conexiones_cpu(){

	int socketCPU;
	struct sockaddr_in direccionCPU;
	socklen_t length = sizeof direccionCPU;

	while ((socketCPU = accept(socketMemoria, (struct sockaddr *) &direccionCPU, &length)) > 0){
		pthread_t thread_cpu;

		printf("%s:%d conectado\n", inet_ntoa(direccionCPU.sin_addr), ntohs(direccionCPU.sin_port));

		creoThread(&thread_cpu, handler_conexiones_cpu, (void *) socketCPU);
	}

	if (socketCPU <= 0){
		printf("Error al aceptar conexiones de CPU\n");
		exit(errno);
	}

	shutdown(socketCPU, 0);
	close(socketCPU);

	return EXIT_SUCCESS;
}

void * handler_conexiones_cpu(void * socketCliente) {
	int sock = (int *) socketCliente;

	//int paginaNueva = 0;

	handShakeListen(&sock, "500", "202", "299", "CPU", 0);

	char * message = malloc(MAXBUF);
	int result = recv(sock, message, MAXBUF, 0);

	while (result > 0){

		char**mensajeDesdeCPU = string_split(message, ";");
		int codigo = atoi(mensajeDesdeCPU[0]);

		if (codigo == 510) {

			puts("SOLICITUD DE INSTRUCCION");
			puts("");

			enviarInstACPU(&sock, mensajeDesdeCPU); //ESTO USA SOLICITAR DATOS DE PAGINA

		} else if (codigo == 511) {

			puts("ASIGNACION DE VARIABLE");
			puts("");

			asignarVariable(mensajeDesdeCPU, sock); //ESTO USA ALMACENAR BYTES EN PAGINA

		} else if (codigo == 512) {

			//Ocurre el retardo para acceder a la memoria principal
			retardo_acceso_memoria();

			char nombreVariable = *mensajeDesdeCPU[1];
			int pid = atoi(mensajeDesdeCPU[2]);
			int pagina = atoi(mensajeDesdeCPU[3]);

			printf("DEFINICION DE VARIABLE %c\n", nombreVariable);
			printf("\n");

			definirVariable(nombreVariable, pid, pagina, sock);

		} else if (codigo == 513) {

			puts("DEREFERENCIAR VARIABLE");
			puts("");

			obtenerValorDeVariable(mensajeDesdeCPU, sock); //ESTO USA SOLICITAR DATOS DE PAGINA

		} else if (codigo == 601) {

			puts("OBTENER POSICION DE VARIABLE");
			puts("");

			int pid = atoi(mensajeDesdeCPU[1]);
			int pagina = atoi(mensajeDesdeCPU[2]);
			int offset = atoi(mensajeDesdeCPU[3]);

			obtenerPosicionVariable(pid, pagina, offset, sock); //ESTO NO NECESITA NI SOLICITAR DATOS NI ALMACENAR BYTES
		}

		free(mensajeDesdeCPU);

		result = recv(sock, message, MAXBUF, 0);
	}

	printf("Se desconecto un CPU\n");

	return EXIT_SUCCESS;
}

/* NO SE USA
t_list * asignarPaginasAProceso(int pid, int paginasRequeridas, int nroPaginaBase){
	pthread_mutex_lock(&mutex_estructuras_administrativas);

	t_list * paginas;

	int i = 0;

	while (i < paginasRequeridas){
		t_pagina_invertida * pagina = buscar_pagina_para_insertar(pid, nroPaginaBase);

		list_add(paginas, pagina);

		nroPaginaBase = nroPaginaBase + 1;
	}

	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	return paginas;
}
*/

void definirVariable(char nombreVariable, int pid, int pagina, int sock){

	t_pagina_invertida * pag_encontrada;
	t_pagina_proceso * manejo_programa = obtenerUltimaPaginaStack(pid);

	if(manejo_programa == NULL){
		//puts("Primera variable del programa");
		definirPrimeraVariable(nombreVariable, pid, pagina, sock);
	} else {
		//puts("ya existen otras variables de ese programa");
		pag_encontrada = buscar_pagina_para_consulta(manejo_programa->pid, manejo_programa->pagina);

		if (!pagina_llena(pag_encontrada)) {
			definirVariableEnPagina(nombreVariable, pag_encontrada, sock);
		}
		//SI LA PAGINA ESTA LLENA PIDO UNA NUEVA
		else {

			bool _paginas_stack_de_proceso(void *pagina) {
                return ((t_pagina_proceso *)pagina)->pid == pid;
            }

			int cantPaginasStackAsignadas = list_count_satisfying(lista_paginas_stack, _paginas_stack_de_proceso);

			if (cantPaginasStackAsignadas < stack_size){
				//El proceso no supera el limite de stack
				//Se le asigna una nueva pagina
				definirVariableEnNuevaPagina(nombreVariable, pid, pagina, sock);
			}
			else {
				//Entonces llego al limite de paginas de stack
				//Se envia mensaje a CPU informando que no puede continuar la ejecucion
				finalizar_programa(pid);

				char* mensajeACpu = string_new();
				string_append(&mensajeACpu, string_itoa(STACK_OVERFLOW));
				string_append(&mensajeACpu, ";");
				enviarMensaje(&sock, mensajeACpu);
			}
		}
	}
}

void definirPrimeraVariable(char nombreVariable, int pid, int pagina, int sock){

	pthread_mutex_lock(&mutex_estructuras_administrativas);

	t_pagina_invertida* pag_a_cargar = buscar_pagina_para_insertar(pid, pagina);

	if (pag_a_cargar == NULL){
		//No hay pagina para asignar
		//Se avisa a la CPU que no se pudo asignar Memoria

		char* mensajeACpu = string_new();
		string_append(&mensajeACpu, string_itoa(ASIGNACION_MEMORIA_ERROR));
		string_append(&mensajeACpu, ";");
		enviarMensaje(&sock, mensajeACpu);

		return;
	}

	int paginaNueva = 1;

	pag_a_cargar->nro_pagina = pagina;
	pag_a_cargar->pid = pid;
	//pag_a_cargar->offset = 0; ////SE INICIALIZA EL OFFSET

	printf("Se asigno el marco %d para la pagina de stack %d del PID %d \n", pag_a_cargar->nro_marco, pag_a_cargar->nro_pagina, pag_a_cargar->pid);
	puts("");

	list_replace(tabla_paginas, pag_a_cargar->nro_marco, pag_a_cargar);

	t_pagina_proceso* manejo_programa = crear_nuevo_manejo_programa(pid, pag_a_cargar->nro_pagina);
	list_add(lista_paginas_stack, manejo_programa);

	t_Stack* entrada_stack = crear_entrada_stack(nombreVariable, pag_a_cargar);

	//grabar_valor(obtener_inicio_pagina(pag_a_cargar), 0);

	////ACTUALIZO EL VALOR DEL OFFSET
	pag_a_cargar->offset = pag_a_cargar->offset + OFFSET_VAR;

	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	char* mensajeACpu = string_new();
	string_append(&mensajeACpu, string_itoa(ASIGNACION_MEMORIA_OK));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, string_itoa(paginaNueva));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, serializar_entrada_indice_stack(entrada_stack));
	string_append(&mensajeACpu, ";");
	enviarMensaje(&sock, mensajeACpu);

	free(entrada_stack);
	free(mensajeACpu);
}

void definirVariableEnPagina(char nombreVariable, t_pagina_invertida* pag_encontrada, int sock){

	int paginaNueva = 0;

	pthread_mutex_lock(&mutex_estructuras_administrativas);

	list_replace(tabla_paginas, pag_encontrada->nro_marco, pag_encontrada);

	t_Stack* entrada_stack = crear_entrada_stack(nombreVariable, pag_encontrada);

	//grabar_valor(obtener_inicio_pagina(pag_encontrada) + obtener_offset_pagina(pag_encontrada), 0);

	////ACTUALIZO EL VALOR DEL OFFSET
	pag_encontrada->offset = pag_encontrada->offset + OFFSET_VAR;

	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	char* mensajeACpu = string_new();
	string_append(&mensajeACpu, string_itoa(ASIGNACION_MEMORIA_OK));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, string_itoa(paginaNueva));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, serializar_entrada_indice_stack(entrada_stack));
	string_append(&mensajeACpu, ";");

	enviarMensaje(&sock, mensajeACpu);
	free(mensajeACpu);
}

void definirVariableEnNuevaPagina(char nombreVariable, int pid, int pagina, int sock){

	int paginaNueva = 1;

	puts("definirVariableEnNuevaPagina");

	pthread_mutex_lock(&mutex_estructuras_administrativas);

	t_pagina_invertida* pag_encontrada = buscar_pagina_para_insertar(pid, pagina);

	if (pag_encontrada == NULL){
		//No hay pagina para asignar
		//Se avisa a la CPU que no se pudo asignar Memoria

		char* mensajeACpu = string_new();
		string_append(&mensajeACpu, string_itoa(ASIGNACION_MEMORIA_ERROR));
		string_append(&mensajeACpu, ";");
		enviarMensaje(&sock, mensajeACpu);

		return;
	}

	pag_encontrada->nro_pagina = pagina;
	pag_encontrada->pid = pid;

	t_Stack* entrada_stack = crear_entrada_stack(nombreVariable, pag_encontrada);

	////ACTUALIZO EL VALOR DEL OFFSET
	pag_encontrada->offset = pag_encontrada->offset + OFFSET_VAR;

	list_replace(tabla_paginas, pag_encontrada->nro_marco, pag_encontrada);

	printf("Se asigno el marco %d para la pagina de stack %d del PID %d \n", pag_encontrada->nro_marco, pag_encontrada->nro_pagina, pag_encontrada->pid);
	puts("");

	t_pagina_proceso* pag_stack = malloc(sizeof(t_pagina_proceso));

	pag_stack->pagina = pag_encontrada->nro_pagina;
	pag_stack->pid = pid;
	list_add(lista_paginas_stack, pag_stack);

	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	char* mensajeACpu = string_new();
	string_append(&mensajeACpu, string_itoa(ASIGNACION_MEMORIA_OK));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, string_itoa(paginaNueva));
	string_append(&mensajeACpu, ";");
	string_append(&mensajeACpu, serializar_entrada_indice_stack(entrada_stack));
	string_append(&mensajeACpu, ";");
	enviarMensaje(&sock, mensajeACpu);

	free(entrada_stack);
	free(mensajeACpu);
}

void asignarVariable(char** mensajeDesdeCPU, int sock){

	int pid = atoi(mensajeDesdeCPU[1]);
	int direccion = atoi(mensajeDesdeCPU[2]);
	int valor = atoi(mensajeDesdeCPU[3]);

	int marco = direccion / configuracion->marco_size;
	t_pagina_invertida * pagina = list_get(tabla_paginas, marco);
	int offset = direccion % configuracion->marco_size;

	almacenarBytesEnPagina(pid, pagina->nro_pagina, offset, OFFSET_VAR, decimalABinarioUnsigned(valor));

	printf("Se asigno el valor %d en Memoria \n", valor);
			printf("\n");
			enviarMensaje(&sock, serializarMensaje(2, direccion, valor));

}

void obtenerValorDeVariable(char** mensajeDesdeCPU, int sock){

	int valor_variable;
	int pid = atoi(mensajeDesdeCPU[1]);
	int direccion = atoi(mensajeDesdeCPU[2]);
	int marco = direccion / configuracion->marco_size;

	char* mensajeACpu = string_new();

	if (marco < configuracion->marcos) {


		t_pagina_invertida* pagina_buscada = list_get(tabla_paginas, marco);
		int offset = direccion % configuracion->marco_size;

		char * bloquePagina = solicitar_datos_de_pagina(pid, pagina_buscada->nro_pagina, offset, OFFSET_VAR);

		valor_variable = ((unsigned char)bloquePagina[0] << 24) +
							((unsigned char)bloquePagina[1] << 16) +
								((unsigned char)bloquePagina[2] << 8) +
									((unsigned char)bloquePagina[3] << 0);

		printf("Se leyo el valor %d en memoria\n", valor_variable);
		puts("");

		string_append(&mensajeACpu, string_itoa(DEREFERENCIAR_OK));
		string_append(&mensajeACpu, ";");
		string_append(&mensajeACpu, string_itoa(valor_variable));
		string_append(&mensajeACpu, ";");

		enviarMensaje(&sock, mensajeACpu);
	}

	else {

		finalizar_programa(pid);

		string_append(&mensajeACpu, string_itoa(DEREFERENCIAR_ERROR));
		string_append(&mensajeACpu, ";");

		enviarMensaje(&sock, mensajeACpu);
	}
	free(mensajeACpu);
}

void obtenerPosicionVariable(int pid, int pagina, int offset, int sock){
	t_pagina_invertida* pagina_buscada = buscar_pagina_para_consulta(pid, pagina);

	if (pagina_buscada == NULL){
		////MANEJAR ERROR, REVISAR
		printf("Error al obtener posicion variable, se finaliza el proceso\n");
	}else{
		int inicio = obtener_inicio_pagina(pagina_buscada);
		int direccion_memoria = inicio + offset;

		printf("Se obtuvo posicion %d a partir de PID %d, Pagina %d y Offset %d en Memoria\n", direccion_memoria, pid, pagina, offset);
		printf("\n");

		char* mensajeACpu = string_new();
		string_append(&mensajeACpu, string_itoa(direccion_memoria));
		string_append(&mensajeACpu, ";");

		enviarMensaje(&sock, mensajeACpu);
		free(mensajeACpu);
	}
}

void enviarInstACPU(int * socketCliente, char ** mensajeDesdeCPU){
	int pid = atoi(mensajeDesdeCPU[1]);
	int inicio_instruccion = atoi(mensajeDesdeCPU[2]);
	int offset = atoi(mensajeDesdeCPU[3]);

	int paginaALeer = inicio_instruccion / configuracion->marco_size;
	int posicionInicioInstruccion = inicio_instruccion % configuracion->marco_size;

	char * instruccion;
	pthread_mutex_lock(&mutex_estructuras_administrativas);

	//SI EL CODIGO ESTA ENTERO EN LA PAGINA
	if (posicionInicioInstruccion + offset <= configuracion->marco_size){
		instruccion = solicitar_datos_de_pagina(pid, paginaALeer, posicionInicioInstruccion, offset);
	}else {
		//SI SE CORTA LA INSTRUCCION VOY LEO LA PRIMERA PARTE, LUEGO LA SEGUNDA Y DESPUES CONCATENO
		int derecha = offset - (configuracion->marco_size - posicionInicioInstruccion);
		char * instruccionFaltante = solicitar_datos_de_pagina(pid, paginaALeer + 1, 0, derecha);
		int izquierda = (offset - derecha);
		instruccion = solicitar_datos_de_pagina(pid, paginaALeer, posicionInicioInstruccion, izquierda);
		//string_append(&instruccion, instruccionFaltante);
		//string_append(&instruccion, " "); //No deberia hacer falta
		string_append(&instruccion, instruccionFaltante);
	}

	pthread_mutex_unlock(&mutex_estructuras_administrativas);
	char * instr = string_substring(instruccion, 0, offset);
	printf("Se envia la instruccion %s\n", instr);
	//printf("Longitud Instruccion: %d\n", strlen(instr));
	//printf("\n");

	enviarMensaje(socketCliente, instr);

	free(instruccion);
}

void finalizar_programa(int pid){

	int _obtenerPaginaProceso(t_pagina_invertida *p) {
		return p->pid == pid;
	}

	int _obtenerCacheProceso(t_entrada_cache* e){
		return e->pid == pid;
	}

	int _obtenerPaginaStackProceso(t_pagina_proceso * p) {
		return p->pid == pid;
	}

	void destruir_pagina_stack(t_pagina_proceso *self){
		free(self);
	}

	int i = 0;
	int cant_paginas_de_cache = list_count_satisfying(tabla_cache, (void*) _obtenerCacheProceso);

	while (i < cant_paginas_de_cache){
		list_remove_by_condition(tabla_cache, (void*) _obtenerCacheProceso);
		i++;
	}

	i = 0;
	int cant_paginas_de_stack = list_count_satisfying(lista_paginas_stack, (void*) _obtenerPaginaStackProceso);

	while (i < cant_paginas_de_stack){
		list_remove_by_condition(lista_paginas_stack, (void*) _obtenerPaginaStackProceso);
		i++;
	}

	t_pagina_invertida* paginaProceso = NULL;
	while((paginaProceso = list_find(tabla_paginas, (void*) _obtenerPaginaProceso)) != NULL){
		liberar_pagina(paginaProceso);
	}
}

void liberar_pagina(t_pagina_invertida* pagina){

	pagina->pid = 0;
	pagina->nro_pagina = 0;

	int i = 0;
	int inicioPagina = obtener_inicio_pagina(pagina);
	int finPagina = inicioPagina + configuracion->marco_size;
	for(i = inicioPagina; i < finPagina; i++){
		bloque_memoria[i] = '\0';
	}

	list_replace(tabla_paginas, pagina->nro_marco, pagina);
}

void * inicializar_consola(void* args){

	Memoria_Config * configuracion = (Memoria_Config *) args;
	char* valorIngresado = string_new();

	while (1) {
		puts("");
		puts("***********************************************************");
		puts("CONSOLA MEMORIA - ACCIONES");
		puts("1) Retardo");
		puts("2) Dump Cache");
		puts("3) Dump Estructuras de memoria");
		puts("4) Dump Contenido de memoria");
		puts("5) Flush Cache");
		puts("6) Size memory");
		puts("7) Size PID");
		puts("***********************************************************");

		int accion = 0;
		int accion_correcta = 0;
		int nuevo_tiempo_retardo = 0, pid_buscado = 0, tamanio_proceso_buscado = 0;

		bool _pagina_disponible(void *pagina) {
			//NO ESTA ASIGNADO Y NO PERTENECE A UNA ESTRUCTURA ADMINISTRATIVA
		    return ((t_pagina_invertida*)pagina)->pid == 0 && ((t_pagina_invertida*)pagina)->pid != -1;
		}

		bool _pagina_ocupada(void *pagina){
			return ((t_pagina_invertida *)pagina)->pid != 0;
		}

		while(accion_correcta == 0){

			scanf("%s", valorIngresado);
			accion = atoi(valorIngresado);

			switch(accion){
				case 1:
					accion_correcta = 1;
					printf("Ingrese nuevo tiempo de retardo (en milisegundos): ");
					scanf("%d", &nuevo_tiempo_retardo);
					pthread_mutex_lock(&mutex_tiempo_retardo);
					tiempo_retardo = nuevo_tiempo_retardo;
					printf("Ahora el tiempo de retardo es: %d \n", tiempo_retardo);
					pthread_mutex_unlock(&mutex_tiempo_retardo);
					break;
				case 2:
					accion_correcta = 1;
					log_cache_in_disk(tabla_cache);
					break;
				case 3:
					accion_correcta = 1;
					log_estructuras_memoria_in_disk();
					break;
				case 4:
					accion_correcta = 1;
					subconsola_contenido_memoria();
					break;
				case 5:
					accion_correcta = 1;
					flush_memoria_cache(tabla_cache);
					break;
				case 6:
					accion_correcta = 1;
					puts("***********************************************************");
					puts("TAMANIO DE LA MEMORIA");
					printf("CANTIDAD TOTAL DE MARCOS: %d \n", configuracion->marcos);
					printf("CANTIDAD DE MARCOS OCUPADOS: %d \n", list_count_satisfying(tabla_paginas, _pagina_ocupada));
					printf("CANTIDAD DE MARCOS LIBRES: %d \n", list_count_satisfying(tabla_paginas, _pagina_disponible));
					puts("***********************************************************");
					break;
				case 7:
					accion_correcta = 1;
					printf("Ingrese PID de proceso: ");
					scanf("%d", &pid_buscado);
					tamanio_proceso_buscado = calcular_tamanio_proceso(pid_buscado);
					printf("La cantidad de Marcos del proceso %d es: %d", pid_buscado, tamanio_proceso_buscado);
					break;
				default:
					accion_correcta = 0;
					puts("Comando invalido. A continuacion se detallan las acciones:");
					puts("1) Retardo");
					puts("2) Dump Cache");
					puts("3) Dump Estructuras de memoria");
					puts("4) Dump Contenido de memoria");
					puts("5) Flush Cache");
					puts("6) Size memory");
					puts("7) Size PID");
					break;
			}
		}
	}

	free(valorIngresado);

	return EXIT_SUCCESS;
}

void inicializar_tabla_paginas(Memoria_Config* config){

	//PID -1 ESTRUCTURAS ADMINISTRATIVAS
	//PID 0 NO ASIGNADO

	tamanio_maximo = obtenerMaximaCantidadDePaginas(config, sizeof(t_pagina_invertida));

	tabla_paginas = list_create();

	int i = 0;
	for(i = 0; i < config->marcos; i++){
		t_pagina_invertida* nueva_pagina = malloc(sizeof(t_pagina_invertida));
		nueva_pagina->nro_marco = i;
		nueva_pagina->nro_pagina = 0;

		//Del 0 al tamanio_maximo->maxima_cant_paginas_administracion (calculado)
		//Se marcan como ocupadas para que ningun proceso pueda escribir en ellas
		if (i < tamanio_maximo->maxima_cant_paginas_administracion){
			nueva_pagina->pid = -1;
		}
		else {
			nueva_pagina->pid = 0;
		}

		memcpy(bloque_memoria + (i * sizeof(t_pagina_invertida)), nueva_pagina, sizeof(t_pagina_invertida));
	}

	int k = 0;
	for (k = 0; k < config->marcos; k++){
		t_pagina_invertida *aux = leer_pagina_de_bloque(bloque_memoria, k * sizeof(t_pagina_invertida), sizeof(t_pagina_invertida));
		list_add(tabla_paginas, aux);
	}
}

t_pagina_invertida *leer_pagina_de_bloque(char *base, int offset, int size){
    char *buffer;
    buffer = malloc(size);
    memcpy(buffer,base+offset,size);
    return (t_pagina_invertida*)buffer;
}

void * solicitar_datos_de_pagina(int pid, int pagina, int offset, int tamanio){
	void * datos_pagina = malloc(tamanio);

	t_entrada_cache* entrada_cache = obtener_entrada_cache(pid, pagina); ////ESTO A VECES TRAE BASURA

	if (entrada_cache == NULL){

		if (cache_habilitada){

		printf("CACHE_MISS para PID %d pagina %d \n", pid, pagina);
		puts("");

		}

		//Ocurre el retardo para acceder a la memoria principal
		retardo_acceso_memoria();

		t_pagina_invertida* pagina_buscada = buscar_pagina_para_consulta(pid, pagina);
		int inicio = obtener_inicio_pagina(pagina_buscada);
		if (pagina_buscada != NULL){
			datos_pagina = leer_memoria(inicio + offset, tamanio);
		}

		//Se carga la nueva pagina en cache
		if (cache_habilitada) almacenar_pagina_en_cache_para_pid(pid, pagina_buscada);
	} else {

		//Leo el contenido de la misma utilizando offset y tamanio
		//datos_pagina = string_substring(entrada_cache->contenido_pagina, offset, tamanio);
		memcpy(datos_pagina, &entrada_cache->contenido_pagina[offset], tamanio);

		entrada_cache->referencia = obtenerTiempoReferencia();

		list_replace(tabla_cache, entrada_cache->indice, entrada_cache);

	}
	return datos_pagina;
}

char* leer_memoria(int inicio, int bytes){
	char * buffer = malloc(bytes);
	memcpy(buffer, &bloque_memoria[inicio], bytes);
	return buffer;
}

t_pagina_proceso* crear_nuevo_manejo_programa(int pid, int pagina){

	int encontrar_pid(t_pagina_proceso *p) {
		return (p->pid == pid);
	}

	t_pagina_proceso* nuevo_manejo_programa = malloc(sizeof(t_pagina_proceso));
	nuevo_manejo_programa->pid = pid;
	nuevo_manejo_programa->pagina = pagina;

	return nuevo_manejo_programa;
}

t_pagina_proceso * obtenerUltimaPaginaStack(int pid){

	int esElProgramaBuscado(t_pagina_proceso *p) {
		return p->pid == pid;
	}

	int ultimaPosicion = list_size(lista_paginas_stack) - 1;

	//return list_find(lista_paginas_stack, (void*) esElProgramaBuscado);

	return list_get(lista_paginas_stack, ultimaPosicion);
}

void log_cache_in_disk(t_list* cache) {

	t_log* logger = log_create("cache.log", "cache",true, LOG_LEVEL_INFO);

	char* dump_memoria = string_new();

	t_list* listado_procesos_cache = list_create();

	char* dump_procesos_activos = string_new();

	string_append(&dump_memoria, "\n");
	string_append(&dump_memoria, "MEMORIA CACHE \n");

	void agregar_registro_dump(t_entrada_cache* ent_cache){

		bool _encontrar_procesos_activo(int valor){
			return valor == ent_cache->pid;
		}

		if (ent_cache != NULL){
			string_append(&dump_memoria, "INDICE: ");
			string_append(&dump_memoria, string_itoa(ent_cache->indice));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "PID: ");
			string_append(&dump_memoria, string_itoa(ent_cache->pid));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "PAGINA: ");
			string_append(&dump_memoria, string_itoa(ent_cache->nro_pagina));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "REFERENCIA: ");
			string_append(&dump_memoria, string_itoa(ent_cache->referencia));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "CONTENIDO: ");
			string_append(&dump_memoria, ent_cache->contenido_pagina);
			string_append(&dump_memoria, "\n");
		}
	}

	if (list_size(tabla_cache) > 0){

		list_iterate(tabla_cache, (void*) agregar_registro_dump);

		if (string_length(dump_procesos_activos) > 0){
			string_append(&dump_memoria, "\n");
			string_append(&dump_memoria, "LISTADO DE PROCESOS ACTIVOS EN CACHE \n");
			string_append(&dump_memoria, dump_procesos_activos);
		}

		log_info(logger, dump_memoria, "INFO");
	}
	else {
		printf("ESTRUCTURA DE CACHE VACIA");
	}

	list_destroy(listado_procesos_cache);
	free(dump_procesos_activos);
	free(dump_memoria);

    log_destroy(logger);

}

void log_estructuras_memoria_in_disk() {

	t_log* logger = log_create("estructura_memoria.log", "est_memoria", true, LOG_LEVEL_INFO);

	char* dump_memoria = string_new();

	t_list* listado_procesos_activos = list_create();

	char* dump_procesos_activos = string_new();

	string_append(&dump_memoria, "\n");
	string_append(&dump_memoria, "TABLA DE PAGINAS \n");

	void agregar_registro_dump(t_pagina_invertida* pagina){

		bool _encontrar_procesos_activo(int valor){
			return valor == pagina->pid;
		}

		if (pagina != NULL){
			string_append(&dump_memoria, "PID: ");
			string_append(&dump_memoria, string_itoa(pagina->pid));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "MARCO: ");
			string_append(&dump_memoria, string_itoa(pagina->nro_marco));
			string_append(&dump_memoria, " ");
			string_append(&dump_memoria, "PAGINA: ");
			string_append(&dump_memoria, string_itoa(pagina->nro_pagina));
			string_append(&dump_memoria, "\n");

			if (pagina->pid != -1 && pagina->pid != 0
					&& list_find(listado_procesos_activos, (void *)_encontrar_procesos_activo) == 0){
				list_add(listado_procesos_activos, (int*)pagina->pid);
				string_append(&dump_procesos_activos, "PID: ");
				string_append(&dump_procesos_activos, string_itoa(pagina->pid));
				string_append(&dump_procesos_activos, "\n");
			}
		}
	}

	if (list_size(tabla_paginas) > 0){

		list_iterate(tabla_paginas, (void*) agregar_registro_dump);

		if (string_length(dump_procesos_activos) > 0){
			string_append(&dump_memoria, "\n");
			string_append(&dump_memoria, "LISTADO DE PROCESOS ACTIVOS \n");
			string_append(&dump_memoria, dump_procesos_activos);
		}

		log_info(logger, dump_memoria, "INFO");
	}
	else {
		printf("ESTRUCTURAS VACIAS");
	}

	list_destroy(listado_procesos_activos);
	free(dump_procesos_activos);
	free(dump_memoria);

    log_destroy(logger);
}

void log_contenido_memoria_in_disk() {

	t_log* logger = log_create("contenido_memoria.log", "contenido_memoria", true, LOG_LEVEL_INFO);

	char* dump = string_new();

	int i = 0;
	string_append(&dump, "\n");
	string_append(&dump, "BLOQUE DE MEMORIA \n");
	for(i = 0; i < configuracion->marcos; i++){

		char * contenido_marco = string_new();
		char * subBloqueMarco = string_new();
		string_append(&contenido_marco, "Contenido del Marco ");
		string_append(&contenido_marco, string_itoa(i));
		string_append(&contenido_marco, ": \n");
		if (i == 0)
			subBloqueMarco = string_substring(bloque_memoria, i * configuracion->marco_size, configuracion->marco_size);
		else
			subBloqueMarco = string_substring(bloque_memoria, i * configuracion->marco_size + 1, configuracion->marco_size);

		if (string_length(subBloqueMarco) == 0) {
			string_append(&contenido_marco, "Marco Vacio");
		}
		else {
			string_append(&contenido_marco, subBloqueMarco);
		}
		string_append(&contenido_marco, "\n");

		string_append(&dump, contenido_marco);

		free(subBloqueMarco);
		free(contenido_marco);
	}
	puts("***********************************************************");

	log_info(logger, dump, "INFO");

	free(dump);

    log_destroy(logger);
}

void log_contenido_memoria_in_disk_for_pid(int pid){

	char* nombre_archivo = string_new();
	char* dump = string_new();

	int _obtenerPaginaProceso(t_pagina_invertida *p) {
		return p->pid == pid;
	}

	void _appendToDump(t_pagina_invertida* p){
		int inicio = obtener_inicio_pagina(p);
		string_append(&dump, "Contenido del Marco ");
		string_append(&dump, string_itoa(p->nro_marco));
		string_append(&dump, ": \n");
		char *subBloque = string_substring(bloque_memoria, inicio, configuracion->marco_size);
		string_append(&dump, subBloque);
		string_append(&dump, " \n");
		free(subBloque);
	}

	string_append(&nombre_archivo, "contenido_memoria_PID_");
	string_append(&nombre_archivo, string_itoa(pid));
	string_append(&nombre_archivo, ".log");

	t_log* logger = log_create(nombre_archivo, nombre_archivo, true, LOG_LEVEL_INFO);

	string_append(&dump, "\n");
	string_append(&dump, "BLOQUE DE MEMORIA DE PID: ");
	string_append(&dump, string_itoa(pid));
	string_append(&dump, "\n");

	t_list* paginas_proceso = list_filter(tabla_paginas, (void*) _obtenerPaginaProceso);

	if (list_size(paginas_proceso) == 0){
		string_append(&dump, "No existe el proceso en Memoria");
	}

	list_iterate(paginas_proceso, (void*) _appendToDump);

	log_info(logger, dump, "INFO");

	free(nombre_archivo);
	free(dump);
	list_destroy_and_destroy_elements(paginas_proceso, free);
	log_destroy(logger);
}

int calcular_tamanio_proceso(int pid_buscado){

	int tamanio_proceso = 0;

	int _obtenerPaginaProceso(t_pagina_invertida *p) {
		return p->pid == pid_buscado;
	}

	tamanio_proceso = list_count_satisfying(tabla_paginas, (void*)_obtenerPaginaProceso);

	return tamanio_proceso;
}

void subconsola_contenido_memoria(){
	int subaccion = -1;
	int subaccionCorrecta = 0;

	puts("Ingrese accion a realizar: ");
	puts("0: Contenido de todo el bloque de Memoria");
	puts("1: Contenido de un proceso particular");
	puts("-1: Salir");

	while(subaccionCorrecta == 0){

		scanf("%d", &subaccion);
		switch(subaccion) {
			case 0:
				subaccionCorrecta = 1;
				log_contenido_memoria_in_disk();
				break;
			case 1:
				subaccionCorrecta = 1;
				int pid = 0;
				printf("Ingrese PID de proceso: ");
				scanf("%d", &pid);
				log_contenido_memoria_in_disk_for_pid(pid);
				break;
			case -1:
				subaccionCorrecta = 1;
				break;
			default:
				subaccionCorrecta = 0;
				puts("Comando invalido. Por favor ingrese alguna de las siguientes opciones:");
				puts("0: Contenido de todo el bloque de Memoria");
				puts("1: Contenido de un proceso particular");
				puts("-1: Salir");
				break;
		}
	}
}

char * decimalABinarioUnsigned(int valor){

	char * buffer = malloc(OFFSET_VAR);

	//ESTO GUARDA EL NUMERO EN BIG ENDIAN EN 4 BYTES
	buffer[0] = (valor >> 24) & 0xFF;
	buffer[1] = (valor >> 16) & 0xFF;
	buffer[2] = (valor >> 8) & 0xFF;
	buffer[3] = valor & 0xFF;

	return buffer;

}

//t_pagina_invertida* grabar_en_bloque(int pid, int cantidad_paginas, char* codigo){
t_list * grabar_en_bloque(int pid, char* codigo){

	int cantidad_paginas = ceiling(strlen(codigo), configuracion->marco_size);
	int longUltimaPagina = strlen(codigo) % configuracion->marco_size;

	t_list * listaPaginas = list_create();

	t_pagina_invertida* pagina_invertida = NULL;

	int i = 0;

	while (i < cantidad_paginas - 1){
		pagina_invertida = buscar_pagina_para_insertar(pid, i);




		pagina_invertida->nro_pagina = i;
		pagina_invertida->pid = pid;

		memcpy(&bloque_memoria[obtener_inicio_pagina(pagina_invertida)], &codigo[i*configuracion->marco_size], configuracion->marco_size);
		list_replace(tabla_paginas, pagina_invertida->nro_marco, pagina_invertida);
		list_add(listaPaginas, pagina_invertida);
		i++;
	}

	if (cantidad_paginas == 1){
		//TRATO LA PRIMERA
		pagina_invertida = buscar_pagina_para_insertar(pid, i);
		pagina_invertida->nro_pagina = i;
		pagina_invertida->pid = pid;

		memcpy(&bloque_memoria[obtener_inicio_pagina(pagina_invertida)], &codigo[i * configuracion->marco_size], strlen(codigo));
		list_replace(tabla_paginas, pagina_invertida->nro_marco, pagina_invertida);
		list_add(listaPaginas, pagina_invertida);
	}else{
		//TRATO LA ULTIMA
		pagina_invertida = buscar_pagina_para_insertar(pid, i);
		pagina_invertida->nro_pagina = i;
		pagina_invertida->pid = pid;

		memcpy(&bloque_memoria[obtener_inicio_pagina(pagina_invertida)], &codigo[i * configuracion->marco_size], longUltimaPagina);
		list_replace(tabla_paginas, pagina_invertida->nro_marco, pagina_invertida);
		list_add(listaPaginas, pagina_invertida);
	}

	return listaPaginas;
}

/*
void grabar_codigo_programa(int* j, t_pagina_invertida* pagina, char* codigo){

	int indice_bloque = obtener_inicio_pagina(pagina);
	while(codigo[*j] != NULL && indice_bloque < (indice_bloque * configuracion->marco_size)){
		bloque_memoria[indice_bloque] = codigo[*j];
		indice_bloque++;
		(*j)++;
	}

}
*/

int f_hash_nene_malloc(int pid, int pagina) {
	int nro_marco_insertar = 0;

	nro_marco_insertar = tamanio_maximo->maxima_cant_paginas_administracion + 1;

	int i = 0;
	for(i = 0; i < pagina; i++){
		nro_marco_insertar += configuracion->marco_size + pid + i;
	}

	nro_marco_insertar = (pid + nro_marco_insertar) % configuracion->marcos;

	return nro_marco_insertar;
}

void pruebas_f_hash(){
	printf("PID 1000 PAG 0: %d\n", f_hash_nene_malloc(1000, 0));
	printf("PID 1000 PAG 1 %d\n", f_hash_nene_malloc(1000, 1));
	printf("PID 1000 PAG 2 %d\n", f_hash_nene_malloc(1000, 2));
	printf("PID 1000 PAG 3 %d\n", f_hash_nene_malloc(1000, 3));
	printf("PID 1000 PAG 4 %d\n", f_hash_nene_malloc(1000, 4));
	printf("PID 1000 PAG 5 %d\n", f_hash_nene_malloc(1000, 5));
	printf("PID 1001 PAG 0 %d\n", f_hash_nene_malloc(1001, 0));
	printf("PID 1001 PAG 1 %d\n", f_hash_nene_malloc(1001, 1));
	printf("PID 1001 PAG 2 %d\n", f_hash_nene_malloc(1001, 2));
	printf("PID 1001 PAG 3 %d\n", f_hash_nene_malloc(1001, 3));
	printf("PID 1001 PAG 4 %d\n", f_hash_nene_malloc(1001, 4));
	printf("PID 1001 PAG 5 %d\n", f_hash_nene_malloc(1001, 5));
	printf("PID 1002 PAG 0 %d\n", f_hash_nene_malloc(1002, 0));
	printf("PID 1002 PAG 1 %d\n", f_hash_nene_malloc(1002, 1));
	printf("PID 1002 PAG 2 %d\n", f_hash_nene_malloc(1002, 2));
	printf("PID 1002 PAG 3 %d\n", f_hash_nene_malloc(1002, 3));
	printf("PID 1002 PAG 4 %d\n", f_hash_nene_malloc(1002, 4));
	printf("PID 1002 PAG 5 %d\n", f_hash_nene_malloc(1002, 5));
	printf("PID 1003 PAG 0 %d\n", f_hash_nene_malloc(1003, 0));
	printf("PID 1003 PAG 1 %d\n", f_hash_nene_malloc(1003, 1));
	printf("PID 1003 PAG 2 %d\n", f_hash_nene_malloc(1003, 2));
	printf("PID 1003 PAG 3 %d\n", f_hash_nene_malloc(1003, 3));
	printf("PID 1003 PAG 4 %d\n", f_hash_nene_malloc(1003, 4));
	printf("PID 1003 PAG 5 %d\n", f_hash_nene_malloc(1003, 5));
	printf("PID 1004 PAG 0 %d\n", f_hash_nene_malloc(1004, 0));
	printf("PID 1004 PAG 1 %d\n", f_hash_nene_malloc(1004, 1));
	printf("PID 1004 PAG 2 %d\n", f_hash_nene_malloc(1004, 2));
	printf("PID 1004 PAG 3 %d\n", f_hash_nene_malloc(1004, 3));
	printf("PID 1004 PAG 4 %d\n", f_hash_nene_malloc(1004, 4));
	printf("PID 1004 PAG 5 %d\n", f_hash_nene_malloc(1004, 5));
	printf("PID 1005 PAG 0 %d\n", f_hash_nene_malloc(1005, 0));
	printf("PID 1005 PAG 1 %d\n", f_hash_nene_malloc(1005, 1));
	printf("PID 1005 PAG 2 %d\n", f_hash_nene_malloc(1005, 2));
	printf("PID 1005 PAG 3 %d\n", f_hash_nene_malloc(1005, 3));
	printf("PID 1005 PAG 4 %d\n", f_hash_nene_malloc(1005, 4));
	printf("PID 1005 PAG 5 %d\n", f_hash_nene_malloc(1005, 5));
	printf("PID 1006 PAG 0 %d\n", f_hash_nene_malloc(1006, 0));
	printf("PID 1006 PAG 1 %d\n", f_hash_nene_malloc(1006, 1));
	printf("PID 1006 PAG 2 %d\n", f_hash_nene_malloc(1006, 2));
	printf("PID 1006 PAG 3 %d\n", f_hash_nene_malloc(1006, 3));
	printf("PID 1006 PAG 4 %d\n", f_hash_nene_malloc(1006, 4));
	printf("PID 1006 PAG 5 %d\n", f_hash_nene_malloc(1006, 5));
	printf("PID 1007 PAG 0 %d\n", f_hash_nene_malloc(1007, 0));
	printf("PID 1007 PAG 1 %d\n", f_hash_nene_malloc(1007, 1));
	printf("PID 1007 PAG 2 %d\n", f_hash_nene_malloc(1007, 2));
	printf("PID 1007 PAG 3 %d\n", f_hash_nene_malloc(1007, 3));
	printf("PID 1007 PAG 4 %d\n", f_hash_nene_malloc(1007, 4));
	printf("PID 1007 PAG 5 %d\n", f_hash_nene_malloc(1007, 5));
	printf("PID 1008 PAG 0 %d\n", f_hash_nene_malloc(1008, 0));
	printf("PID 1008 PAG 1 %d\n", f_hash_nene_malloc(1008, 1));
	printf("PID 1008 PAG 2 %d\n", f_hash_nene_malloc(1008, 2));
	printf("PID 1008 PAG 3 %d\n", f_hash_nene_malloc(1008, 3));
	printf("PID 1008 PAG 4 %d\n", f_hash_nene_malloc(1008, 4));
	printf("PID 1008 PAG 5 %d\n", f_hash_nene_malloc(1008, 5));
	printf("PID 1009 PAG 0 %d\n", f_hash_nene_malloc(1009, 0));
	printf("PID 1009 PAG 1 %d\n", f_hash_nene_malloc(1009, 1));
	printf("PID 1009 PAG 2 %d\n", f_hash_nene_malloc(1009, 2));
	printf("PID 1009 PAG 3 %d\n", f_hash_nene_malloc(1009, 3));
	printf("PID 1009 PAG 4 %d\n", f_hash_nene_malloc(1009, 4));
	printf("PID 1009 PAG 5 %d\n", f_hash_nene_malloc(1009, 5));
	printf("PID 1010 PAG 0 %d\n", f_hash_nene_malloc(1010, 0));
	printf("PID 1010 PAG 1 %d\n", f_hash_nene_malloc(1010, 1));
	printf("PID 1010 PAG 2 %d\n", f_hash_nene_malloc(1010, 2));
	printf("PID 1010 PAG 3 %d\n", f_hash_nene_malloc(1010, 3));
	printf("PID 1010 PAG 4 %d\n", f_hash_nene_malloc(1010, 4));
	printf("PID 1010 PAG 5 %d\n", f_hash_nene_malloc(1010, 5));
}

_Bool quedanPaginasLibres(){

	_Bool libre(t_pagina_invertida * elem){
		return elem->pid == 0;
	}

	int resultado = list_count_satisfying(tabla_paginas, (void *) libre);

	//return list_any_satisfy(tabla_paginas, (void *) libre);
	return resultado >= 1;
}

t_pagina_invertida * obtenerPrimerPaginaLibre(int marco){
	_Bool estaLibre(t_pagina_invertida * elem){
		return elem->pid == 0;
	}

	//RECORRO BUSCANDO PAGINAS LIBRES A PARTIR DEL MARCO QUE ENCONTRE CON LA FUNCION DE HASH
	int i = marco;
	while (i < configuracion->marcos){
		t_pagina_invertida * pagina = list_get(tabla_paginas, i);

		if (estaLibre(pagina)){
			pagina->offset = 0;
			return pagina;
		}else{
			i++;
		}

		//SI LLEGUE AL FINAL
		if (i == configuracion->marcos){
			//ARRANCO DEL PRINCIPIO
			i = tamanio_maximo->maxima_cant_paginas_administracion;
		}
		//CUIDADO RIESGO DE LOOP INFINITO JAJA
	}

	/*
	//SI LLEGO ACA ES PORQUE LLEGUE AL FINAL DE LA TABLA DE PAGINAS, TENGO QUE EMPEZAR DESDE EL PRINCIPIO
	i = tamanio_maximo->maxima_cant_paginas_administracion;
	while (i < marco){
		t_pagina_invertida * pagina = list_get(tabla_paginas, i);

		if (estaLibre(pagina)){
			pagina->offset = 0;
			return pagina;
		}else{
			i++;
		}
	}
	*/
}

t_pagina_invertida* buscar_pagina_para_insertar(int pid, int pagina){

	int nro_marco = f_hash_nene_malloc(pid, pagina);
	t_pagina_invertida* pagina_encontrada = list_get(tabla_paginas, nro_marco);

	//Si la pagina encontrada no es una estructura administrativa
	//Ni esta ocupada por otro proceso

	if (pagina_encontrada->pid == 0) {
		pagina_encontrada->offset = 0;
		return pagina_encontrada;
	}
	else if (quedanPaginasLibres(tabla_paginas)) {
		//Si la pagina que devuelve esta ocupada, o sea hubo una colision
		//Y ademas quedan paginas libres para asignar
		//Busco un marco libre a partir del ultimo marco de estructuras administrativas
		//Hasta final de la Memoria
		printf("HUBO COLISION, INFO MARCO: %d, PID: %d, PAGINA: %d\n", pagina_encontrada->nro_marco, pagina_encontrada->pid, pagina_encontrada->nro_pagina);

		t_pagina_invertida* pagina_aux = obtenerPrimerPaginaLibre(nro_marco);


		return pagina_aux;
	} else {
		//MANEJO QUE NO HAYAN PAGINAS LIBRES
		finalizar_programa(pid);
		return NULL;
	}
}

t_pagina_invertida* buscar_pagina_para_consulta(int pid, int pagina){
	int nro_marco = f_hash_nene_malloc(pid, pagina);
	//printf("BUSCO PARA CONSULTAR PID: %d, PAGINA: %d\n", pid, pagina);
	t_pagina_invertida* pagina_encontrada = list_get(tabla_paginas, nro_marco);
	//printf("PAGINA ENCONTrAdA PARA CONSULTAR: %d, MARCO: %d\n", pagina_encontrada->nro_pagina, pagina_encontrada->nro_marco);

	return pagina_encontrada;
}

void retardo_acceso_memoria(){
	//Se divide por mil porque esta en milisegundos y sleep espera como parametro segundos
	sleep(tiempo_retardo / 1000);
}

t_Stack* crear_entrada_stack(char variable, t_pagina_invertida* pagina){

	t_Stack* entrada_stack = malloc(sizeof(t_Stack));
	entrada_stack->nombre_variable = variable;
	entrada_stack->direccion.pagina = pagina->nro_pagina;
	entrada_stack->direccion.offset = obtener_offset_pagina(pagina);
	entrada_stack->direccion.size = OFFSET_VAR;

	return entrada_stack;
}

char* serializar_entrada_indice_stack(t_Stack* indice_stack){

	char* entrada_stack = string_new();
	entrada_stack[0] = indice_stack->nombre_variable;
	string_append(&entrada_stack, ";");
	string_append(&entrada_stack, string_itoa(indice_stack->direccion.pagina));
	string_append(&entrada_stack, ";");
	string_append(&entrada_stack, string_itoa(indice_stack->direccion.offset));
	string_append(&entrada_stack, ";");
	string_append(&entrada_stack, string_itoa(indice_stack->direccion.size));
	string_append(&entrada_stack, ";");

	return entrada_stack;
}

//ESTO BUSCA LA ULTIMA POSICION LIBRE DE LA PAGINA
int obtener_offset_pagina(t_pagina_invertida* pagina){

	return pagina->offset;

}

int obtener_inicio_pagina(t_pagina_invertida* pagina){
	int inicio = pagina->nro_marco * configuracion->marco_size;
	return inicio;
}

bool pagina_llena(t_pagina_invertida* pagina){

	//return pagina->offset == configuracion->marco_size - OFFSET_VAR;
	return pagina->offset == configuracion->marco_size;

}

t_entrada_cache * crear_entrada_cache(int indice, int pid, int nro_pagina, char * contenido_pagina){
	t_entrada_cache* entrada_nueva_cache = malloc(sizeof(t_entrada_cache));
	entrada_nueva_cache->contenido_pagina = malloc(configuracion->marco_size);
	entrada_nueva_cache->indice = indice;
	memcpy(entrada_nueva_cache->contenido_pagina, contenido_pagina, configuracion->marco_size);
	entrada_nueva_cache->nro_pagina = nro_pagina;
	entrada_nueva_cache->pid = pid;
	entrada_nueva_cache->referencia = obtenerTiempoReferencia();
	return entrada_nueva_cache;
}

bool almacenar_pagina_en_cache_para_pid(int pid, t_pagina_invertida* pagina){

	bool guardadoOK = true;

	bool _paginas_en_cache_para_proceso(t_entrada_cache* entrada){
		return entrada->pid == pid;
	}

    bool _indice_menor(t_entrada_cache* unaEntrada, t_entrada_cache* otra_entrada) {
        return unaEntrada->indice < otra_entrada->indice;
    }

	int nro_paginas_en_cache = list_count_satisfying(tabla_cache, (void*)_paginas_en_cache_para_proceso);

	if (cache_habilitada && nro_paginas_en_cache < configuracion->cache_x_proc && list_size(tabla_cache) < configuracion->entradas_cache){

		//Si no se supera el limite de entradas de cache por proceso
		//Y si no se supera el tamanio de la cache
		//Agrego la entrada

		int indice_cache = obtener_nuevo_indice_cache();

		t_entrada_cache * entrada_cache = crear_entrada_cache(indice_cache,
				pagina->pid, pagina->nro_pagina,
					leer_memoria(obtener_inicio_pagina(pagina), configuracion->marco_size));

		list_add(tabla_cache, entrada_cache);
		printf("Se almaceno el marco %d pagina %d del PID %d en el indice %d de la cache \n", pagina->nro_marco, pagina->nro_pagina, pagina->pid, indice_cache);

		puts("");

	}
	else {

		t_entrada_cache* entrada_cache_reemplazo = NULL;

		//Ejecuto el algoritmo de reemplazo
		if (list_size(tabla_cache) == configuracion->entradas_cache){
			//Reemplazo global
			entrada_cache_reemplazo = obtener_entrada_reemplazo_cache(NULL);
		}
		else {
			//Reemplazo local
			entrada_cache_reemplazo = obtener_entrada_reemplazo_cache(pid);
		}

		printf("La victima del reemplazo en Cache es PID: %d Pagina %d Indice %d", entrada_cache_reemplazo->pid, entrada_cache_reemplazo->nro_pagina, entrada_cache_reemplazo->indice);

		puts("");
		char* contenido_pagina = leer_memoria(obtener_inicio_pagina(pagina), configuracion->marco_size);

		//Hago el reemplazo de paginas y ordeno
		entrada_cache_reemplazo->pid = pid;
		entrada_cache_reemplazo->nro_pagina = pagina->nro_pagina;
		entrada_cache_reemplazo->contenido_pagina = contenido_pagina;
		entrada_cache_reemplazo->referencia = obtenerTiempoReferencia();

		list_sort(tabla_cache, (void*)_indice_menor);
		list_replace(tabla_cache, entrada_cache_reemplazo->indice, entrada_cache_reemplazo);

		printf("Se actualizo el indice %d de la Cache asignados a Pagina %d y PID %d \n", entrada_cache_reemplazo->indice, pagina->nro_pagina, pagina->pid);
		puts("");

	}

	return guardadoOK;
}

t_entrada_cache* obtener_entrada_cache(int pid, int pagina){

	int _encontrar_entrada_cache(t_entrada_cache* ent){
		return ent->pid == pid && ent->nro_pagina == pagina;
	}

	t_entrada_cache* entrada = list_find(tabla_cache, (void*) _encontrar_entrada_cache);

	return entrada;
}

//OBSOLETO
/*
void grabar_valor_en_cache(int direccion, char * buffer){

	puts("Grabar_valor_en_cache");
	puts("");
	printf("NRO MARCO: %d\n", direccion / configuracion->marco_size);

	t_pagina_invertida * pagina = list_get(tabla_paginas, direccion / configuracion->marco_size);

	t_entrada_cache* entrada_cache = obtener_entrada_cache(pagina->pid, pagina->nro_pagina);

	if (entrada_cache == NULL){
		almacenar_pagina_en_cache_para_pid(pagina->pid, pagina); ////REVISAR
	} else {
		actualizar_pagina_en_cache(pagina->pid,
				pagina->nro_pagina,
					leer_memoria(obtener_inicio_pagina(pagina), configuracion->marco_size)); ////REVISAR
	}
}
*/

t_entrada_cache* obtener_entrada_reemplazo_cache(int pid){

    bool _referencia_menor(t_entrada_cache* unaEntrada, t_entrada_cache* otra_entrada) {
        return unaEntrada->referencia < otra_entrada->referencia;
    }

	t_entrada_cache* entrada_cache = malloc(sizeof(t_entrada_cache));

	int obtener_entrada_proceso(t_entrada_cache* entrada){
		return entrada->pid == pid;
	}

	if (string_equals_ignore_case(configuracion->reemplazo_cache, "LRU") == true){

		puts("El algoritmo es LRU");
		//Obtengo a la victima del reemplazo ordenando la cache por tiempo de referencia ascendente

		list_sort(tabla_cache, (void*)_referencia_menor);

		if (pid == NULL){
			//Si es global seria la pagina mas antigua en la cache, es decir, la que tenga indice 0
			entrada_cache = list_get(tabla_cache, 0);
		}
		else {
			//Si es global seria la pagina mas antigua para ese PID en la cache, es decir, la primera que encuentre para ese PID
			entrada_cache = list_find(tabla_cache, (void*) obtener_entrada_proceso);
		}
	}

	//log_cache_in_disk(tabla_cache);

	return entrada_cache;
}

int obtener_nuevo_indice_cache(){
	int cant_entradas_cache = list_size(tabla_cache);
	return cant_entradas_cache;
}

void almacenarBytesEnPagina(int pid, int pagina, int offset, int size, void * buffer){
	//TENGO QUE VALIDAR QUE EXISTA LA PAGINA?
	t_pagina_invertida * paginaAActualizar = buscar_pagina_para_consulta(pid, pagina);
	int direccionBasePagina = obtener_inicio_pagina(paginaAActualizar);

	retardo_acceso_memoria();

	pthread_mutex_lock(&mutex_bloque_memoria);
	memcpy(&bloque_memoria[direccionBasePagina + offset], buffer, size);
	pthread_mutex_unlock(&mutex_bloque_memoria);

	if (cache_habilitada){
			t_entrada_cache * entradaCache = obtener_entrada_cache(pid, pagina);

			pthread_mutex_lock(&mutex_estructuras_administrativas);

			if (entradaCache == NULL){
				paginaAActualizar->pid = pid;
				paginaAActualizar->nro_pagina = pagina;

				almacenar_pagina_en_cache_para_pid(pid, paginaAActualizar);
				entradaCache = obtener_entrada_cache(pid, pagina);
			}

			memcpy(&entradaCache->contenido_pagina[offset], buffer, size);

			pthread_mutex_unlock(&mutex_estructuras_administrativas);
	}
}

int obtenerTiempoReferencia(){
	time_t curtime;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	curtime=tv.tv_usec;

	return (int)curtime;
}

