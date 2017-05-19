//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//250 KER A MEM - ENVIO PROGRAMA A MEMORIA
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL
//510 CPU A MEM - SOLICITUD SCRIPT
//511 CPU A MEM - ASIGNAR VARIABLE
//512 CPU A MEM - DEFINIR VARIABLE
//513 CPU A MEM - DEREFERENCIAR VARIABLE
//600 KER A MEM - RESERVAR MEMORIA HEAP
//601 CPU A MEM - SOLICITAR POSICION DE VARIABLE
//612 KER A MEM - ENVIO DE CANT MAXIMA DE PAGINAS DE STACK POR PROCESO

#include <pthread.h>
#include "configuracion.h"
#include "memoria.h"
#include <errno.h>
#include "helperFunctions.h"
#include <time.h>
#include <semaphore.h>

int conexionesKernel = 0;
int conexionesCPU = 0;
int tiempo_retardo;
pthread_mutex_t mutex_tiempo_retardo;
pthread_mutex_t mutex_estructuras_administrativas;
pthread_mutex_t mutex_bloque_memoria;
//int semaforo = 0;
t_list* tabla_paginas;
t_list* tabla_cache;
int stack_size = 2; //TODO: Al realizar el handshake con Kernel le deberia pasar a memoria el stack_size
char* bloque_memoria;
char* bloque_cache;
Memoria_Config* configuracion;
t_max_cantidad_paginas* tamanio_maximo;
t_list* tabla_programas;

int socketKernel;
struct sockaddr_in direccionKernel;
socklen_t length = sizeof direccionKernel;

int socketMemoria;
struct sockaddr_in direccionMemoria;
sem_t semaforoKernel;

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

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

	bloque_memoria = string_new();
	bloque_cache = string_new();

	//Alocacion de bloque de memoria contigua
	//Seria el tamanio del marco * la cantidad de marcos

	bloque_memoria = calloc(configuracion->marcos, configuracion->marco_size);
	if (bloque_memoria == NULL){
		perror("No se pudo reservar el bloque de memoria del Sistema\n");
	}

	bloque_cache = calloc(configuracion->entradas_cache, configuracion->marco_size);
	if (bloque_cache == NULL){
		perror("No se pudo reservar el bloque para la memoria cache del Sistema\n");
	}

	pthread_mutex_lock(&mutex_tiempo_retardo);
	tiempo_retardo = configuracion->retardo_memoria;
	pthread_mutex_unlock(&mutex_tiempo_retardo);

	pthread_mutex_lock(&mutex_estructuras_administrativas);
	inicializar_tabla_paginas(configuracion);
	pthread_mutex_unlock(&mutex_estructuras_administrativas);

	tabla_cache = list_create();
	tabla_programas = list_create();

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
	list_destroy_and_destroy_elements(tabla_programas, free);

	free(configuracion);
	free(tamanio_maximo);
	free(bloque_memoria);
	free(bloque_cache);

	sem_destroy(&semaforoKernel);

	shutdown(socketMemoria, 0);
	close(socketMemoria);
}

void * hilo_conexiones_kernel(){

	socketKernel = accept(socketMemoria, (struct sockaddr *) &direccionKernel, &length);

	if (socketKernel > 0) {
		sem_post(&semaforoKernel);
		printf("%s:%d conectado\n", inet_ntoa(direccionKernel.sin_addr), ntohs(direccionKernel.sin_port));
		handShakeListen(&socketKernel, "100", "201", "299", "Kernel");
		char message[MAXBUF];

		int result = recv(socketKernel, message, sizeof(message), 0);

		while (result > 0) {

			retardo_acceso_memoria();

			char**mensajeDelKernel = string_split(message, ";");
			int operacion = atoi(mensajeDelKernel[0]);
			int pid;

			switch(operacion){

				case 250:
					;
					pid = atoi(mensajeDelKernel[1]);
					char * codigo_programa = mensajeDelKernel[2];
					int cantidadDePaginas = string_length(codigo_programa) / configuracion->marco_size;
					if (cantidadDePaginas == 0) cantidadDePaginas++;
					iniciarPrograma(pid, cantidadDePaginas, codigo_programa);
					break;

				case 600:
					;
					pid = atoi(mensajeDelKernel[1]);
					int paginaActual = atoi(mensajeDelKernel[2]);
					int bytes = atoi(mensajeDelKernel[3]);
					crearPaginaHeap(pid, paginaActual, bytes);
					break;

				case 612:
					stack_size = atoi(mensajeDelKernel[1]);
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

void iniciarPrograma(int pid, int paginas, char * codigo_programa) {
	if (string_length(bloque_memoria) == 0 || string_length(codigo_programa) <= string_length(bloque_memoria)) {

		pthread_mutex_lock(&mutex_estructuras_administrativas);
		t_pagina_invertida* pagina = grabar_en_bloque(pid, paginas, codigo_programa);
		pthread_mutex_unlock(&mutex_estructuras_administrativas);
		char* respuestaAKernel = string_new();

		if (pagina != NULL) {

			printf("MARCO ASIGNADO CODIGO: %d\n", pagina->nro_marco);

			string_append(&respuestaAKernel, "203;");
			string_append(&respuestaAKernel, string_itoa(obtener_inicio_pagina(pagina)));
			string_append(&respuestaAKernel, ";");
			string_append(&respuestaAKernel, string_itoa(paginas));
			enviarMensaje(&socketKernel, respuestaAKernel);
		}

		else {
			//Si el codigo del programa supera el tamanio del bloque
			//Le aviso al Kernel que no puede reservar el espacio para el programa
			string_append(&respuestaAKernel, "298;");
			enviarMensaje(&socketKernel, respuestaAKernel);
		}

		free(respuestaAKernel);
	}
}

void crearPaginaHeap(int pid, int paginaActual, int bytesPedidos){
	//POR AHORA HAGO UNA SOLA PAGINA
	//int cantidadPaginas = bytesPedidos / configuracion->marco_size;

	//SI ALCANZA UNA PAGINA PARA GUARDAR LO QUE ME PIDEN
	int freeSpace = configuracion->marco_size - sizeof(heapMetadata);
	if (bytesPedidos < freeSpace){
		t_pagina_invertida * pagina = buscar_pagina_para_insertar(pid, paginaActual);
		//ESTO ME ESTA DEVOLVIENDO PAGINA CERO

		char * respuestaAKernel = string_new();

		//FALTA UN CODIGO DE OPERACION?
		string_append(&respuestaAKernel, string_itoa(pagina->pid));
		string_append(&respuestaAKernel, ";");
		string_append(&respuestaAKernel, string_itoa(pagina->nro_pagina));
		string_append(&respuestaAKernel, ";");
		string_append(&respuestaAKernel, string_itoa(freeSpace));
		string_append(&respuestaAKernel, ";");
		enviarMensaje(&socketKernel, respuestaAKernel);

		printf("Se creo una pagina de Heap\n");

		printf("PID: %d\n", pagina->pid);
		printf("Nro Pagina: %d\n", pagina->nro_pagina);
		printf("Free Space: %d\n", freeSpace);

		free(respuestaAKernel);
	} else {
		perror("Error al crear Pagina de Heap\n");
	}
}

void * hilo_conexiones_cpu(){

	int socketCPU;
	struct sockaddr_in direccionCPU;
	socklen_t length = sizeof direccionCPU;

	while (socketCPU = accept(socketMemoria, (struct sockaddr *) &direccionCPU, &length)){
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

	handShakeListen(&sock, "500", "202", "299", "CPU");

	char message[MAXBUF];
	int result = recv(sock, message, sizeof(message), 0);

	while (result > 0){

		retardo_acceso_memoria();

		char**mensajeDesdeCPU = string_split(message, ";");
		int codigo = atoi(mensajeDesdeCPU[0]);

		if (codigo == 510) {

			enviarInstACPU(&sock, mensajeDesdeCPU);

		} else if (codigo == 511) {

			int direccion = atoi(mensajeDesdeCPU[1]);
			int valor = atoi(mensajeDesdeCPU[2]);

			grabar_valor(direccion, valor);


		} else if (codigo == 512) {

			char nombreVariable = *mensajeDesdeCPU[1];

			int pid = atoi(mensajeDesdeCPU[2]);

			int paginaParaVariables = atoi(mensajeDesdeCPU[3]);

			t_pagina_invertida* pag_encontrada;

			t_manejo_programa * manejo_programa = get_manejo_programa(pid);

			if(manejo_programa == NULL){

				//puts("primera variable del programa");

				pthread_mutex_lock(&mutex_estructuras_administrativas);

				t_pagina_invertida* pag_a_cargar = buscar_pagina_para_insertar(pid, paginaParaVariables);

				pag_a_cargar->nro_pagina = paginaParaVariables;
				pag_a_cargar->pid = pid;
				list_replace(tabla_paginas, pag_a_cargar->nro_marco, pag_a_cargar);

				manejo_programa = crear_nuevo_manejo_programa(pid, pag_a_cargar->nro_pagina);
				list_add(tabla_programas, manejo_programa);

				pthread_mutex_unlock(&mutex_estructuras_administrativas);

				t_Stack* entrada_stack = crear_entrada_stack(nombreVariable, pag_a_cargar);

				grabar_valor(obtener_inicio_pagina(pag_a_cargar), 0);

				char* mensajeACpu = string_new();
				string_append(&mensajeACpu, serializar_entrada_indice_stack(entrada_stack));
				enviarMensaje(&sock, mensajeACpu);

				free(entrada_stack);

			}else{

				//puts("ya existen otras variables de ese programa");

				pag_encontrada = buscar_pagina_para_consulta(manejo_programa->pid, manejo_programa->numero_pagina);

				if (!pagina_llena(pag_encontrada)) {

					pthread_mutex_lock(&mutex_estructuras_administrativas);
					list_replace(tabla_paginas, pag_encontrada->nro_marco, pag_encontrada);
					pthread_mutex_unlock(&mutex_estructuras_administrativas);

					t_Stack* entrada_stack = crear_entrada_stack(nombreVariable, pag_encontrada);

					grabar_valor(obtener_inicio_pagina(pag_encontrada) + obtener_offset_pagina(pag_encontrada), 0);

					char* mensajeACpu = string_new();
					string_append(&mensajeACpu, serializar_entrada_indice_stack(entrada_stack));

					string_append(&mensajeACpu, ";");

					enviarMensaje(&sock, mensajeACpu);
				}
				else {
					//TODO Verificar que el proceso no exceda el stack_size enviado por Kernel
					//Y asignarle una nueva pagina de stack si no excede
				}
			}

		} else if (codigo == 513) {

			int posicion_de_la_Variable = atoi(mensajeDesdeCPU[1]);
			char* valor_variable  = string_new();

			valor_variable = leer_memoria(posicion_de_la_Variable, OFFSET_VAR);

			int valor = atoi(valor_variable);

			char* mensajeACpu = string_new();
			string_append(&mensajeACpu, string_itoa(valor));
			string_append(&mensajeACpu, ";");

			enviarMensaje(&sock, mensajeACpu);

		} else if (codigo == 601) {

			int pid = atoi(mensajeDesdeCPU[1]);
			int pagina = atoi(mensajeDesdeCPU[2]);
			int offset = atoi(mensajeDesdeCPU[3]);

			t_pagina_invertida* pag_a_buscar = buscar_pagina_para_consulta(pid, pagina);

			if (pag_a_buscar != NULL){

				int inicio = obtener_inicio_pagina(pag_a_buscar);

				int direccion_memoria = inicio + offset;

				char* mensajeACpu = string_new();
				string_append(&mensajeACpu, string_itoa(direccion_memoria));
				string_append(&mensajeACpu, ";");

				enviarMensaje(&sock, mensajeACpu);
			}
		}

		free(mensajeDesdeCPU);

		result = recv(sock, message, sizeof(message), 0);
	}

	printf("Se desconecto un CPU\n");

	return EXIT_SUCCESS;
}

int marco_libre_para_variables(){

	t_pagina_invertida* pag;
	int i = 0;

	int encontrado = 0;

	while (encontrado == 0){
		int encontrar_pag(t_pagina_invertida *pagina) {
			return ((pagina->nro_marco == configuracion->marcos - i) && (pagina->pid == 0));
		}

		pag = list_find(tabla_paginas, (void*) encontrar_pag);
		if(pag != NULL){
			encontrado = 1;
		}
		i++;
	}

	if(pag != NULL){
		return pag->nro_marco;
	}else{
		return -1;
	}
}

t_pagina_invertida* list_encontrar_pag_variables(t_list* lista){

	t_pagina_invertida* pag;

	int size = list_size(lista);
	int i;
	int maximo = 0;
	int igual = 1;

	for(i = 0; i < size; i++){
		int num_marco = ((t_pagina_invertida*) list_get(lista, i))->nro_marco;

		if(num_marco == maximo){
			igual++;
		}else if(num_marco > maximo){
			maximo = num_marco;
		}
	}


	if(igual == size){
		return NULL;
	}else{

		int encontrar_pagina_maxima(t_pagina_invertida *pag) {
			return (pag->nro_marco == maximo);
		}

		pag = (t_pagina_invertida*) list_find(lista, (void *) encontrar_pagina_maxima);

		return pag;
	}

}

void enviarInstACPU(int * socketCliente, char ** mensajeDesdeCPU){
	int pid = atoi(mensajeDesdeCPU[1]);
	int inicio_instruccion = atoi(mensajeDesdeCPU[2]);
	int offset = atoi(mensajeDesdeCPU[3]);
	int cantidadPaginas = atoi(mensajeDesdeCPU[4]);
	int paginaALeer = 0;
	if (cantidadPaginas > 1){
		paginaALeer = (inicio_instruccion + offset) % configuracion->marco_size;
	}
	//int inicio = inicio_bloque + inicio_instruccion;

	char* respuestaACPU = string_new();
	//string_append(&respuestaACPU, leer_codigo_programa(pid, inicio, offset));
	pthread_mutex_lock(&mutex_estructuras_administrativas);
	string_append(&respuestaACPU, solicitar_datos_de_pagina(pid, paginaALeer, inicio_instruccion, offset));
	pthread_mutex_unlock(&mutex_estructuras_administrativas);
	enviarMensaje(socketCliente, respuestaACPU);
	free(respuestaACPU);
}

void finalizar_programa(int pid){

	int _obtenerPaginaProceso(t_pagina_invertida *p) {
		return p->pid == pid;
	}

	t_pagina_invertida* paginaProceso = NULL;
	while((paginaProceso = list_remove_by_condition(tabla_paginas, (void*) _obtenerPaginaProceso)) != NULL){
		liberar_pagina(paginaProceso);
	}
}

void liberar_pagina(t_pagina_invertida* pagina){
	pagina->pid = 0;
	list_replace(tabla_paginas, pagina->nro_marco, pagina);
}

void * inicializar_consola(void* args){

	Memoria_Config * configuracion = (Memoria_Config *) args;

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

			scanf("%d", &accion);

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
					log_contenido_memoria_in_disk();
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
					printf("El tamanio total del proceso %d es: %d", pid_buscado, tamanio_proceso_buscado);
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
		t_pagina_invertida *aux = memory_read(bloque_memoria, k * sizeof(t_pagina_invertida), sizeof(t_pagina_invertida));
		list_add(tabla_paginas, aux);
	}
}

t_pagina_invertida *memory_read(char *base, int offset, int size){
    char *buffer;
    buffer = malloc(size);
    memcpy(buffer,base+offset,size);
    return (t_pagina_invertida*)buffer;
}

char* solicitar_datos_de_pagina(int pid, int pagina, int offset, int tamanio){
	char* datos_pagina = string_new();
	t_pagina_invertida* pagina_buscada = buscar_pagina_para_consulta(pid, pagina);
	int inicio = obtener_inicio_pagina(pagina_buscada);
	if (pagina_buscada != NULL){
		datos_pagina = leer_memoria(inicio + offset, tamanio);
	}
	return datos_pagina;
}

char* leer_codigo_programa(int pid, int inicio, int offset){
	char* codigo_programa = string_new();
	codigo_programa = string_substring(bloque_memoria, inicio, offset);
	return codigo_programa;
}

char* leer_memoria(int inicio, int offset){
	char* codigo_programa = string_new();
	codigo_programa = string_substring(bloque_memoria, inicio, offset);
	return codigo_programa;
}

t_manejo_programa* crear_nuevo_manejo_programa(int pid, int pagina){

	int encontrar_pid(t_manejo_programa *p) {
		return (p->pid == pid);
	}

	t_manejo_programa* nuevo_manejo_programa = malloc(sizeof(t_manejo_programa));
	nuevo_manejo_programa->pid = pid;
	nuevo_manejo_programa->numero_pagina = pagina;

	return nuevo_manejo_programa;
}

t_manejo_programa* get_manejo_programa(int pid){

	int esElProgramaBuscado(t_manejo_programa *p) {
		return p->pid == pid;
	}

	return list_find(tabla_programas, (void*) esElProgramaBuscado);
}

void log_cache_in_disk(t_list* cache) {
	t_log* logger = log_create("cache.log", "cache",true, LOG_LEVEL_INFO);

    log_info(logger, "LOGUEO DE INFO DE CACHE %s", "INFO");

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
				list_add(listado_procesos_activos, pagina->pid);
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

	log_info(logger, "LOGUEO DE CONTENIDO DE MEMORIA %s", "INFO");

    log_destroy(logger);
}

void grabar_valor(int direccion, int valor){

	if(valor > 999){
		int milesima = valor / 1000;
		int centena = (valor % 1000) / 100;
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		pthread_mutex_lock(&mutex_bloque_memoria);
		bloque_memoria[direccion] = (char) (milesima + 48);
		bloque_memoria[direccion + 1] = (char) (centena + 48);
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
		pthread_mutex_unlock(&mutex_bloque_memoria);
	}else if(valor > 99){
		int centena = (valor % 1000) / 100;
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		pthread_mutex_lock(&mutex_bloque_memoria);
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = (char) (centena + 48);
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
		pthread_mutex_unlock(&mutex_bloque_memoria);
	}else if(valor > 9){
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		pthread_mutex_lock(&mutex_bloque_memoria);
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = '0';
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
		pthread_mutex_unlock(&mutex_bloque_memoria);
	}else{
		int unidad = valor % 10;
		pthread_mutex_lock(&mutex_bloque_memoria);
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = '0';
		bloque_memoria[direccion + 2] = '0';
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
		pthread_mutex_unlock(&mutex_bloque_memoria);
	}

return;

}

t_pagina_invertida* grabar_en_bloque(int pid, int cantidad_paginas, char* codigo){

	t_pagina_invertida* pagina_invertida = NULL;

	int i = 0, j = 0;

	if (cantidad_paginas == 1){
		pagina_invertida = buscar_pagina_para_insertar(pid, 0);
		pagina_invertida->nro_pagina = 0;
		pagina_invertida->pid = pid;

		grabar_codigo_programa(&j, pagina_invertida, codigo);

		list_replace(tabla_paginas, pagina_invertida->nro_marco, pagina_invertida);
	}
	else {
		for(i = 0; i < cantidad_paginas; i++){
			int nro_pagina = 0;
			pagina_invertida = buscar_pagina_para_insertar(pid, cantidad_paginas);

			if (pagina_invertida == NULL){
				break;
			}

			grabar_codigo_programa(&j, pagina_invertida, codigo);

			//Actualizo la tabla de paginas
			pagina_invertida->nro_pagina = nro_pagina;
			pagina_invertida->pid = pid;
			list_replace(tabla_paginas, pagina_invertida->nro_marco, pagina_invertida);

			nro_pagina++;
		}
	}

	return pagina_invertida;
}

void grabar_codigo_programa(int* j, t_pagina_invertida* pagina, char* codigo){

	int indice_bloque = obtener_inicio_pagina(pagina);
	while(codigo[*j] != NULL && indice_bloque < (indice_bloque * configuracion->marco_size)){
		bloque_memoria[indice_bloque] = codigo[*j];
		indice_bloque++;
		(*j)++;
	}
}

int paginaLibre(t_pagina_invertida* pagina){
	return pagina->pid == 0;
}

int f_hash_nene_malloc(int pid, int pagina) {
	int nro_marco_insertar = 0;

	nro_marco_insertar = tamanio_maximo->maxima_cant_paginas_administracion;

	int i = 0;
	for(i = 0; i < pagina; i++){
		nro_marco_insertar += configuracion->marco_size + pid + i;
	}

	nro_marco_insertar = (pid + nro_marco_insertar) % configuracion->marcos + 1;

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

t_pagina_invertida* buscar_pagina_para_insertar(int pid, int pagina){

	int nro_marco = f_hash_nene_malloc(pid, pagina);
	t_pagina_invertida* pagina_encontrada = list_get(tabla_paginas, nro_marco);

	//Si la pagina encontrada no es una estructura administrativa
	//Ni esta ocupada por otro proceso

	if (pagina_encontrada->pid != -1 && pagina_encontrada->pid == 0) {
		return pagina_encontrada;
	}
	else {
		//Si la pagina que devuelve esta ocupada, o sea hubo una colision
		//Busco un marco libre a partir del ultimo marco de estructuras administrativas
		//Hasta final de la Memoria
		int i = tamanio_maximo->maxima_cant_paginas_administracion;
		while (i < tamanio_maximo->maxima_cant_paginas_procesos){
			pagina_encontrada = list_get(tabla_paginas, i);
			if (pagina_encontrada->pid != -1 && pagina_encontrada->pid == 0) {
				return pagina_encontrada;
			}
			i++;
		}
	}
	return NULL;
}

t_pagina_invertida* buscar_pagina_para_consulta(int pid, int pagina){
	int nro_marco = f_hash_nene_malloc(pid, pagina);
	t_pagina_invertida* pagina_encontrada = list_get(tabla_paginas, nro_marco);

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

int obtener_offset_pagina(t_pagina_invertida* pagina){

	int inicio = obtener_inicio_pagina(pagina);

	int offset = 0;

	char* bloque_asignado = string_substring(bloque_memoria, inicio, configuracion->marco_size);

	int longitud_datos_bloque_asignado = string_length(bloque_asignado);

	offset = longitud_datos_bloque_asignado;

	return offset;
}

int obtener_inicio_pagina(t_pagina_invertida* pagina){
	int inicio = pagina->nro_marco * configuracion->marco_size + 1;
	return inicio;
}

bool pagina_llena(t_pagina_invertida* pagina){

	bool pagina_llena = false;

	int inicio = obtener_inicio_pagina(pagina);

	char* bloque_asignado = string_substring(bloque_memoria, inicio, configuracion->marco_size);
	int longitud_datos_bloque_asignado = string_length(bloque_asignado);

	if (longitud_datos_bloque_asignado == configuracion ->marco_size)
		pagina_llena = true;

	return pagina_llena;
}

