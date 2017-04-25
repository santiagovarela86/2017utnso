//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL
//510 CPU A MEM - SOLICITUD SCRIPT

#include <pthread.h>
#include "configuracion.h"
#include "memoria.h"
#include <errno.h>
#include "helperFunctions.h"

int conexionesKernel = 0;
int conexionesCPU = 0;
int tiempo_retardo;
pthread_mutex_t mutex_tiempo_retardo;
int semaforo = 0;
t_list* tabla_marcos;
t_list* tabla_paginas;
t_queue* memoria_cache;

char* bloque_memoria;
int indice_bloque_procesos;
int indice_bloque_estructuras_administrativas;

Memoria_Config* configuracion;
t_max_cantidad_paginas* tamanio_maximo;
t_list* tabla_programas;

void enviarScriptACPU(int * socketCliente, char ** mensajeDesdeCPU);

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_mutex_init(&mutex_tiempo_retardo, NULL);

	bloque_memoria = string_new();
	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	int socketMemoria;
	struct sockaddr_in direccionMemoria;
	creoSocket(&socketMemoria, &direccionMemoria, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketMemoria, &direccionMemoria);
	escuchoSocket(&socketMemoria);

	threadSocketInfo * threadSocketInfoMemoria;
	threadSocketInfoMemoria = malloc(sizeof(struct threadSocketInfo));
	threadSocketInfoMemoria->sock = socketMemoria;
	threadSocketInfoMemoria->direccion = direccionMemoria;

	pthread_mutex_lock(&mutex_tiempo_retardo);
	tiempo_retardo = configuracion->retardo_memoria;
	pthread_mutex_unlock(&mutex_tiempo_retardo);

	inicializar_estructuras_administrativas(configuracion);

	pthread_t thread_consola;
	pthread_t thread_kernel;
	pthread_t thread_cpu;

	creoThread(&thread_consola, inicializar_consola, configuracion);
	creoThread(&thread_kernel, hilo_conexiones_kernel, threadSocketInfoMemoria);

	while(semaforo == 0){}

	creoThread(&thread_cpu, hilo_conexiones_cpu, threadSocketInfoMemoria);

	pthread_join(thread_consola, NULL);
	pthread_join(thread_kernel, NULL);
	pthread_join(thread_cpu, NULL);

	free(configuracion);
	free(tamanio_maximo);
	free(threadSocketInfoMemoria);
	free(bloque_memoria);

	shutdown(socketMemoria, 0);
	close(socketMemoria);

	return EXIT_SUCCESS;
}

void * hilo_conexiones_kernel(void * args){
	threadSocketInfo * threadSocketInfoMemoria = (threadSocketInfo *) args;

	int socketCliente;
	struct sockaddr_in direccionCliente;
	socklen_t length = sizeof direccionCliente;

	socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length);

	if (socketCliente > 0) {
		semaforo = 1;
		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));
		handShakeListen(&socketCliente, "100", "201", "299", "Kernel");
		char message[MAXBUF];

		int result = recv(socketCliente, message, sizeof(message), 0);

		while (result > 0) {

			char**mensajeDelKernel = string_split(message, ";");
			int pid = atoi(mensajeDelKernel[0]);
			char* codigo_programa = mensajeDelKernel[1];

			int cant_paginas = string_length(codigo_programa) / configuracion->marco_size;
			if (cant_paginas == 0)
				cant_paginas++;

			iniciar_programa(pid, codigo_programa, cant_paginas, socketCliente);

			result = recv(socketCliente, message, sizeof(message), 0);
		}

		if (result <= 0) {
			printf("Se desconecto el Kernel\n");
			exit(errno);
		}
	} else {
		perror("Fallo en el manejo del hilo Kernel\n");
		return EXIT_FAILURE;
	}

	shutdown(socketCliente, 0);
	close(socketCliente);

	return EXIT_SUCCESS;
}

void * hilo_conexiones_cpu(void * args){
	threadSocketInfo * threadSocketInfoMemoria = (threadSocketInfo *) args;

	int socketCliente;
	struct sockaddr_in direccionCliente;
	socklen_t length = sizeof direccionCliente;

	while (socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length)){
		pthread_t thread_cpu;

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		creoThread(&thread_cpu, handler_conexiones_cpu, (void *) socketCliente);
	}

	if (socketCliente <= 0){
		printf("Error al aceptar conexiones de CPU\n");
		exit(errno);
	}

	//LLEGA ALGUNA VEZ A ESTO?
	shutdown(socketCliente, 0);
	close(socketCliente);

	return EXIT_SUCCESS;
}

void * handler_conexiones_cpu(void * socketCliente) {
	int sock = (int *) socketCliente;

	handShakeListen(&sock, "500", "202", "299", "CPU");

	char message[MAXBUF];
	int result = recv(sock, message, sizeof(message), 0);

	while (result > 0){
		char**mensajeDesdeCPU = string_split(message, ";");
		int codigo = atoi(mensajeDesdeCPU[0]);

		if (codigo == 510) {

			enviarScriptACPU(&sock, mensajeDesdeCPU);

		} else if (codigo == 511) {
			puts("siiiiiii");
			int direccion = atoi(mensajeDesdeCPU[1]);
			int valor = atoi(mensajeDesdeCPU[2]);

			grabar_valor(direccion, valor);

		} else if (codigo == 512) {

			char identificador_variable = *mensajeDesdeCPU[1];
			int pid = atoi(mensajeDesdeCPU[2]);

			t_manejo_programa* manejo_programa = get_manejo_programa(pid);
			int posicion = 0;

			if (manejo_programa == NULL){
				//Si es la primera asigno el proximo marco disponible para stack desde el final
				t_marco* nuevo_marco = get_marco_libre(true);
				if (nuevo_marco != NULL){

					//Agrego un nuevo manejador del programa para la variable
					manejo_programa = crear_nuevo_manejo_programa(pid, identificador_variable, nuevo_marco->nro_marco, 0);
					list_add(tabla_programas, manejo_programa);

					posicion = nuevo_marco->inicio;

					definir_variable(posicion, identificador_variable, pid);
					actualizar_marco(nuevo_marco->nro_marco, 1, nuevo_marco->disponible + 1);
					char* mensajeACpu = string_new();
					string_append(&mensajeACpu, string_itoa(posicion));
					string_append(&mensajeACpu, ";");

					enviarMensaje(&sock, mensajeACpu);
				}
			}
			else {
				//Obtengo el marco asignado
				t_marco* marco_asignado = list_get(tabla_marcos, manejo_programa->nro_marco);
				posicion = marco_asignado->disponible;

				//Valido que no este escribiendo fuera del marco
				if (posicion < marco_asignado->final){
					//Empieza a escribir en el primer byte disponible de ese bloque
					definir_variable(posicion, identificador_variable, pid);
					//Agrego un nuevo manejador del programa para la variable
					manejo_programa = crear_nuevo_manejo_programa(pid, identificador_variable, marco_asignado->nro_marco, 0);
					actualizar_marco(marco_asignado->nro_marco, 1, marco_asignado->disponible - 1);
					char* mensajeACpu = string_new();
					string_append(&mensajeACpu, string_itoa(posicion));
					string_append(&mensajeACpu, ";");

					enviarMensaje(&sock, mensajeACpu);
				}
			}

		} else if (codigo == 513) {

			int posicion_de_la_Variable = atoi(mensajeDesdeCPU[1]);
			char* valor_variable  = string_new();

			valor_variable = leer_memoria(posicion_de_la_Variable, OFFSET_VAR);

			char* mensajeACpu = string_new();
			string_append(&mensajeACpu, valor_variable);
			string_append(&mensajeACpu, ";");

			enviarMensaje(&sock, mensajeACpu);
		}

		free(mensajeDesdeCPU);

		result = recv(sock, message, sizeof(message), 0);
	}

	printf("Se desconecto un CPU\n");

	return EXIT_SUCCESS;
}

void enviarScriptACPU(int * socketCliente, char ** mensajeDesdeCPU){
	int pid = atoi(mensajeDesdeCPU[1]);
	int inicio_bloque = atoi(mensajeDesdeCPU[2]);
	int offset = atoi(mensajeDesdeCPU[3]);

	char* respuestaACPU = string_new();
	string_append(&respuestaACPU, leer_codigo_programa(pid, inicio_bloque, offset));
	enviarMensaje(socketCliente, respuestaACPU);
	free(respuestaACPU);
}

void finalizar_programa(int pid){

	int _obtenerPaginaProceso(t_pagina_invertida *p) {
		return p->pid == pid;
	}

	t_pagina_invertida* paginaProceso = NULL;
	while((paginaProceso = list_remove_by_condition(tabla_paginas, (void*) _obtenerPaginaProceso)) != NULL){
		t_marco* marco_asignado = list_get(tabla_marcos, paginaProceso->nro_marco);
		actualizar_marco(marco_asignado->nro_marco, 0, marco_asignado->final);
		destruir_pagina(paginaProceso);
	}
}

void destruir_pagina(t_pagina_invertida* pagina){
	free(pagina);
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

		while(accion_correcta == 0){

			scanf("%d", &accion);

			switch(accion){
				case 1:
					accion_correcta = 1;
					printf("Ingrese nuevo tiempo de retardo: ");
					scanf("%d", &nuevo_tiempo_retardo);
					pthread_mutex_lock(&mutex_tiempo_retardo);
					tiempo_retardo = nuevo_tiempo_retardo;
					printf("Ahora el tiempo de retardo es: %d \n", tiempo_retardo);
					pthread_mutex_unlock(&mutex_tiempo_retardo);
					break;
				case 2:
					accion_correcta = 1;
					log_cache_in_disk(memoria_cache);
					break;
				case 3:
					accion_correcta = 1;
					log_estructuras_memoria_in_disk(tabla_paginas);
					break;
				case 4:
					accion_correcta = 1;
					log_contenido_memoria_in_disk(tabla_paginas);
					break;
				case 5:
					accion_correcta = 1;
					flush_cola_cache(memoria_cache);
					break;
				case 6:
					accion_correcta = 1;
					puts("***********************************************************");
					puts("TAMANIO DE LA MEMORIA");
					printf("CANTIDAD TOTAL DE MARCOS: %d \n", configuracion->marcos);
					printf("CANTIDAD DE MARCOS OCUPADOS: %d \n", 0);
					printf("CANTIDAD DE MARCOS LIBRES: %d \n", 0);
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

void inicializar_estructuras_administrativas(Memoria_Config* config){

	//Alocacion de bloque de memoria contigua
	//Seria el tamanio del marco * la cantidad de marcos

	bloque_memoria = calloc(config->marcos, config->marco_size);
	if (bloque_memoria == NULL){
		perror("No se pudo reservar el bloque de memoria del Sistema\n");
	}

	inicializar_lista_marcos(config);

	tabla_paginas = list_create();
	memoria_cache = crear_cola_cache();
	tabla_programas = list_create();
}

void inicializar_lista_marcos(Memoria_Config* config){

	tabla_marcos = list_create();
	int i = 0;
	for(i = 0; i < config->marcos; i++){
		t_marco* nuevo_marco = malloc(sizeof(t_marco));
		nuevo_marco->nro_marco = i;
		if (i == 0){
			nuevo_marco->inicio = i * config->marco_size;
			nuevo_marco->final = nuevo_marco->inicio + config->marco_size;
		}
		else {
			nuevo_marco->inicio = i * config->marco_size + 1;
			nuevo_marco->final = nuevo_marco->inicio + config->marco_size + 1;
		}
		nuevo_marco->disponible = nuevo_marco->inicio;

		//Del 0 al 38 van a estar las estructuras administrativas
		//Se marcan como ocupadas para que ningun proceso pueda escribir en ellas
		if (i < 39)
			nuevo_marco->asignado = 1;
		else
			nuevo_marco->asignado = 0;
		list_add(tabla_marcos, nuevo_marco);
	}
}

void iniciar_programa(int pid, char* codigo, int cant_paginas, int socket_kernel){

	char* respuestaAKernel = string_new();

	if (string_length(bloque_memoria) == 0 || string_length(codigo) <= string_length(bloque_memoria)){
		t_pagina_invertida* pagina = grabar_en_bloque(pid, cant_paginas, codigo);
		if (pagina != NULL){
			string_append(&respuestaAKernel, "203;");
			string_append(&respuestaAKernel, string_itoa(pagina->inicio));
			string_append(&respuestaAKernel, ";");
			string_append(&respuestaAKernel, string_itoa(pagina->offset));
			enviarMensaje(&socket_kernel, respuestaAKernel);
		}
		else {
			//Si el codigo del programa supera el tamanio del bloque
			//Le aviso al Kernel que no puede reservar el espacio para el programa
			string_append(&respuestaAKernel, "298;");
			enviarMensaje(&socket_kernel, respuestaAKernel);
		}
	}

	free(respuestaAKernel);
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

t_pagina_invertida* crear_nueva_pagina(int pid, int marco, int pagina, int inicio, int offset){
	t_pagina_invertida* nueva_pagina = malloc(sizeof(t_pagina_invertida));
	nueva_pagina->pid = pid;
	nueva_pagina->nro_marco = marco;
	nueva_pagina->nro_pagina = pagina;
	nueva_pagina->inicio = inicio;
	nueva_pagina->offset = offset;
	return nueva_pagina;
}

t_manejo_programa* crear_nuevo_manejo_programa(int pid, char variable, int marco, int pagina){
	t_manejo_programa* nuevo_manejo_programa = malloc(sizeof(t_manejo_programa));
	nuevo_manejo_programa->pid = pid;
	nuevo_manejo_programa->variable = variable;
	nuevo_manejo_programa->nro_marco = marco;
	nuevo_manejo_programa->numero_pagina = pagina;

	return nuevo_manejo_programa;
}

t_manejo_programa* get_manejo_programa(int pid){

	int esElProgramaBuscado(t_manejo_programa *p) {
		return p->pid == pid;
	}

	return list_find(tabla_programas, (void*) esElProgramaBuscado);
}

t_marco* get_marco_libre(bool esDescendente){

	t_marco* marco_encontrado = NULL;

	int _marcoAsignado(t_marco* marco){
		return marco->asignado == 0;
	}

    bool _nro_marco_descendente(t_marco *marco1, t_marco *marco2) {
        return marco1->nro_marco > marco2->nro_marco;
    }

    bool _nro_marco_ascendente(t_marco *marco1, t_marco *marco2) {
        return marco1->nro_marco < marco2->nro_marco;
    }

    if (esDescendente) {
    	list_sort(tabla_marcos, (void*)_nro_marco_descendente);
    	marco_encontrado = list_find(tabla_marcos, (void*) _marcoAsignado);
    	//Ordeno de vuelta la estructura porque es global
    	list_sort(tabla_marcos, (void*)_nro_marco_ascendente);
    	return marco_encontrado;
    }

	return list_find(tabla_marcos, (void*) _marcoAsignado);
}

void actualizar_marco(int indice, int asignado, int disponible){

	t_marco* marco = list_get(tabla_marcos, indice);
	marco->asignado = asignado;
	marco->disponible = disponible;
	list_replace(tabla_marcos,indice, marco);
}

void log_cache_in_disk(t_queue* cache) {
	t_log* logger = log_create("cache.log", "cache",true, LOG_LEVEL_INFO);

    log_info(logger, "LOGUEO DE INFO DE CACHE %s", "INFO");

    log_destroy(logger);
}

void log_estructuras_memoria_in_disk(t_list* tabla_paginas) {

	t_log* logger = log_create("estructura_memoria.log", "est_memoria", true, LOG_LEVEL_INFO);

	log_info(logger, "LOGUEO DE INFO DE ESTRUCTURAS DE MEMORIA %s", "INFO");

    log_destroy(logger);
}

void log_contenido_memoria_in_disk(t_list* tabla_paginas) {

	t_log* logger = log_create("contenido_memoria.log", "contenido_memoria", true, LOG_LEVEL_INFO);

	char* dump_memoria = string_new();

	void agregar_registro_dump(t_pagina_invertida* pagina){
		string_append(&dump_memoria, "PID: ");
		string_append(&dump_memoria, string_itoa(pagina->pid));
		string_append(&dump_memoria, " ");
		string_append(&dump_memoria, "MARCO: ");
		string_append(&dump_memoria, string_itoa(pagina->nro_marco));
		string_append(&dump_memoria, " ");
		string_append(&dump_memoria, "PAGINA: ");
		string_append(&dump_memoria, string_itoa(pagina->nro_pagina));
		string_append(&dump_memoria, " ");
		string_append(&dump_memoria, "CODIGO: ");
		string_append(&dump_memoria, string_substring(bloque_memoria, pagina->inicio, pagina->offset));
	}

	list_iterate(tabla_paginas, (void*) agregar_registro_dump);

	log_info(logger, dump_memoria, "INFO");

	free(dump_memoria);

    log_destroy(logger);
}

void definir_variable(int posicion_donde_guardo, char identificador_variable, int pid){

	//TODO Verificar espacio suficiente en memoriA
	bloque_memoria[posicion_donde_guardo] = '0';
	bloque_memoria[posicion_donde_guardo + 1] = '0';
	bloque_memoria[posicion_donde_guardo + 2] = '0';
	bloque_memoria[posicion_donde_guardo + 3] = '0';

	printf("La variable %c se guardo en la pos: %d \n",identificador_variable, posicion_donde_guardo);

	return;
}

void grabar_valor(int direccion, int valor){

	if(valor > 999){
		int milesima = valor / 1000;
		int centena = (valor % 1000) / 100;
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		bloque_memoria[direccion] = (char) (milesima + 48);
		bloque_memoria[direccion + 1] = (char) (centena + 48);
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
	}else if(valor > 99){
		int centena = (valor % 1000) / 100;
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = (char) (centena + 48);
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
	}else if(valor > 9){
		int decena = (valor % 100) / 10;
		int unidad = valor % 10;
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = '0';
		bloque_memoria[direccion + 2] = (char) (decena + 48);
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
	}else{
		int unidad = valor % 10;
		bloque_memoria[direccion] = '0';
		bloque_memoria[direccion + 1] = '0';
		bloque_memoria[direccion + 2] = '0';
		bloque_memoria[direccion + 3] = (char) (unidad + 48);
	}

return;

}

t_pagina_invertida* grabar_en_bloque(int pid, int cantidad_paginas, char* codigo){

	t_pagina_invertida* pagina_invertida = NULL;

	int i = 0, j = 0;
	int marcosAOcupar = cantidad_paginas;

	if (marcosAOcupar == 0) //El contenido del programa es menor al tamanio del marco le asigno un marco como minimo
		marcosAOcupar = 1;

	for(i = 0; i < marcosAOcupar; i++){
		int nro_pagina = 0;
		t_marco* marco = get_marco_libre(false);

		if (marco == NULL){
			break;
		}
		int indice_bloque = marco->inicio;
		while(codigo[j] != NULL && indice_bloque < marco->final){
			bloque_memoria[indice_bloque] = codigo[j];
			indice_bloque++;
			j++;
		}

		//Actualizo el marco como ocupado
		int espacio_disponible = marco->inicio + indice_bloque;

		actualizar_marco(marco->nro_marco, 1, espacio_disponible);

		//Agrego la pagina con el codigo a la lista de paginas
		pagina_invertida = crear_nueva_pagina(pid, marco->nro_marco, nro_pagina, marco->inicio, string_length(codigo));
		list_add(tabla_paginas, pagina_invertida);
		nro_pagina++;
	}

	return pagina_invertida;
}


uint32_t f_hash(const void *buf, size_t buflength) {
     const uint8_t *buffer = (const uint8_t*)buf;

     uint32_t s1 = 1;
     uint32_t s2 = 0;
     size_t i;

     for (i = 0; i < buflength; i++) {
        s1 = (s1 + buffer[i]) % 65521;
        s2 = (s2 + s1) % 65521;
     }
     return (s2 << 16) | s1;
}

