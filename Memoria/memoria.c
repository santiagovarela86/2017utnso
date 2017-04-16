//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL

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
t_list* tabla_paginas;
t_queue* memoria_cache;

void * hilo_conexiones_kernel(void * args);
void * hilo_conexiones_cpu(void * args);
void * handler_conexiones_cpu(void * args);

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_mutex_init(&mutex_tiempo_retardo, NULL);

	Memoria_Config * configuracion;
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
	free(threadSocketInfoMemoria);

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

	if (socketCliente) {
		semaforo = 1;
		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));
		handShakeListen(&socketCliente, "100", "201", "299", "Kernel");
		char message[MAXBUF];

		int result = recv(socketCliente, message, sizeof(message), 0);

		while (result) {
			printf("%s", message);
			result = recv(socketCliente, message, sizeof(message), 0);
		}

		if (result <= 0) {
			printf("Se desconecto el Kernel\n");
		}
	} else {
		perror("Fallo en el manejo del hilo Kernel");
		return EXIT_FAILURE;
	}

	/*
	while (socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length)) {
		semaforo = 1;

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));
		handShakeListen(&socketCliente, "100", "201", "299", "Kernel");

		char message[MAXBUF];
		int result = recv(socketCliente, message, sizeof(message), 0);

		if (result) {
			printf("%s", message);
		} else {
			printf("Se desconecto el Kernel\n");
		}
	}

	if (socketCliente < 0) {
		perror("Fallo en el manejo del hilo Kernel");
		return EXIT_FAILURE;
	}
	*/

	/*
	socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length);

	semaforo = 1;

	printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

	handShakeListen(&socketCliente, "100", "201", "299", "Kernel");

	char message[MAXBUF];

	recv(socketCliente, message, sizeof(message), 0);
	printf("%s", message);
	//re recv(socketCliente, message, sizeof(message), 0); con el len
	*/

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

		/*
		if (socketCliente < 0) {
			perror("Fallo en el manejo del hilo CPU");
			return EXIT_FAILURE;
		}
		*/

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		creoThread(&thread_cpu, handler_conexiones_cpu, (void *) socketCliente);
	}

	shutdown(socketCliente, 0);
	close(socketCliente);

	return EXIT_SUCCESS;
}

void * handler_conexiones_cpu(void * socketCliente) {


	//socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length);

		if (socketCliente > 0) {

			handShakeListen(&socketCliente, "500", "202", "299", "CPU");
			char message[MAXBUF];

			int result = recv(socketCliente, message, sizeof(message), 0);

			while (result) {
				printf("%s", message);
				result = recv(socketCliente, message, sizeof(message), 0);
			}

			if (result <= 0) {
				printf("Se desconecto un CPU\n");
			}
		} else {
			perror("Fallo en el manejo del hilo CPU");
			return EXIT_FAILURE;
		}





	//handShakeListen(&socketCliente, "500", "202", "299", "CPU");

	return EXIT_SUCCESS;
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
					puts("***********************************************************");
					puts("TAMANIO DE LA MEMORIA");
					printf("CANTIDAD TOTAL DE MARCOS: %d \n", configuracion->marcos);
					printf("CANTIDAD DE MARCOS OCUPADOS: %d \n", 0);
					printf("CANTIDAD DE MARCOS LIBRES: %d \n", 0);
					puts("***********************************************************");
					break;
				case 7:
					printf("Ingrese PID de proceso: ");
					scanf("%d", &pid_buscado);
					printf("El tamanio total del proceso %d es: %d", pid_buscado, tamanio_proceso_buscado);
					break;
				default:
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
	int* bloque_memoria = calloc(config->marcos, config->marco_size);
	if (bloque_memoria == NULL){
		perror("No se pudo reservar el bloque de memoria del Sistema\n");
	}

	tabla_paginas = list_create();
	memoria_cache = crear_cola_cache();
}

void log_cache_in_disk(t_queue* cache) {
	t_log* logger = log_create("cache.log", "cache",true, LOG_LEVEL_INFO);

    log_info(logger, "LOGUEO DE INFO DE CACHE %s", "INFO");

    log_destroy(logger);

    printf("cache.log generado con exito! \n");
}

void log_estructuras_memoria_in_disk(t_list* tabla_paginas) {

	t_log* logger = log_create("estructura_memoria.log", "est_memoria", true, LOG_LEVEL_INFO);

	log_info(logger, "LOGUEO DE INFO DE ESTRUCTURAS DE MEMORIA %s", "INFO");

    log_destroy(logger);

    printf("estructura_memoria.log generado con exito! \n");
}

void log_contenido_memoria_in_disk(t_list* tabla_paginas) {

	t_log* logger = log_create("contenido_memoria.log", "contenido_memoria", true, LOG_LEVEL_INFO);
	log_info(logger, "LOGUEO DE INFO DE CONTENIDO DE MEMORIA %s", "INFO");
    log_destroy(logger);

    printf("contenido_memoria.log generado con exito! \n");
}

/* FUNCION DE HASH */
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

