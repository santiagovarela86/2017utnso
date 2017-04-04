//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL

#include <pthread.h>
#include "configuracion.h"
#include "memoria.h"
#include <errno.h>

int conexionesKernel = 0;
int conexionesCPU = 0;
int tiempo_retardo;
//Memoria_Config * configuracion;
pthread_mutex_t mutex_tiempo_retardo = PTHREAD_MUTEX_INITIALIZER;
t_list* tabla_paginas;
t_queue* memoria_cache;

void * handler_kernel(void * args);
void * handler_cpu(void * args);

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	//char message[1000] = "";
	//char server_reply[2000] = "";

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
	creoThread(&thread_kernel, handler_kernel, threadSocketInfoMemoria);
	creoThread(&thread_cpu, handler_cpu, threadSocketInfoMemoria);

	pthread_join(thread_consola, NULL);
	pthread_join(thread_kernel, NULL);
	pthread_join(thread_cpu, NULL);

	free(configuracion);
	free(threadSocketInfoMemoria);

	shutdown(socketMemoria, 0);
	close(socketMemoria);

	return EXIT_SUCCESS;
}

void * handler_kernel(void * args){
	threadSocketInfo * threadSocketInfoMemoria = (threadSocketInfo *) args;

	while (1) {
		int socketCliente;
		struct sockaddr_in direccionCliente;
		socklen_t length = sizeof direccionCliente;

		socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length);

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShake(threadSocketInfoMemoria->sock, &socketCliente, 100, 201, 299, "Kernel");

		shutdown(socketCliente, 0);
		close(socketCliente);
	}

	return EXIT_SUCCESS;
}

void * handler_cpu(void * args){
	threadSocketInfo * threadSocketInfoMemoria = (threadSocketInfo *) args;

	while (1) {
		int socketCliente;
		struct sockaddr_in direccionCliente;
		socklen_t length = sizeof direccionCliente;

		socketCliente = accept(threadSocketInfoMemoria->sock, (struct sockaddr *) &direccionCliente, &length);

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShake(threadSocketInfoMemoria->sock, &socketCliente, 500, 202, 499, "CPU");

		shutdown(socketCliente, 0);
		close(socketCliente);
	}

	return EXIT_SUCCESS;
}

/*

	inicializar_estructuras_administrativas(configuracion);

	creoThread(&hilo_consola_memoria, inicializar_consola, NULL);



	if (pthread_create(&hilo_consola_memoria, NULL, inicializar_consola, NULL) < 0) {
		perror("Error al crear el Hilo de Consola Memoria");
		return EXIT_FAILURE;
	}

	while ((socketCliente = accept(socketMemoria, (struct sockaddr *) &direccionMemoria, (socklen_t*) &length))) {
		pthread_t thread_id;

		creoThread(&thread_id, handler_conexion, (void *) socketCliente);

		if (pthread_create(&thread_id, NULL, handler_conexion, (void*) &socketCliente)< 0) {
			perror("Error al crear el Hilo");
			return 1;
		}

	}

	if (socketCliente < 0) {
		perror("Fallo en la conexion");
		return 1;
	}

	pthread_join(hilo_consola_memoria, NULL);

	return EXIT_SUCCESS;
}

*/

/*
void* handler_conexion(void * socket_desc) {

	//int socketHandler;
	char consola_message[1000] = "";
	char* codigo;

	while ((recv((int) socket_desc, consola_message, sizeof(consola_message), 0)) > 0) {
		codigo = strtok(consola_message, ";");

		if (atoi(codigo) == 100) {
			printf("Se acepto la conexion del Kernel \n");
			conexionesKernel++;
			printf("Tengo %d Kernel(s) conectado(s) \n", conexionesKernel);

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '1';
			consola_message[3] = ';';

			if (send((int) socket_desc, consola_message, strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				exit(errno);
				//return EXIT_FAILURE;
			}

		} else if (atoi(codigo) == 500) {
			printf("Se acepto la conexion de la CPU \n");
			conexionesCPU++;
			printf("Tengo %d CPU(s) conectado(s) \n", conexionesCPU);

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '2';
			consola_message[3] = ';';

			if (send((int) socket_desc, consola_message,
					strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				exit(errno);
				//return EXIT_FAILURE;
			}
		} else {
			printf("Se rechazo una conexion incorrecta \n");

			consola_message[0] = '2';
			consola_message[1] = '9';
			consola_message[2] = '9';
			consola_message[3] = ';';

			if (send((int) socket_desc, consola_message, strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				exit(errno);
				//return EXIT_FAILURE;
			}
		}
	}

	while (1) {
	}

	return EXIT_SUCCESS;
}

*/

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

	int tamanio_memoria = config->marcos * config->marco_size;
	int bloque_memoria = malloc(sizeof(tamanio_memoria)); //no deberia ser int * bloque_memoria esto?
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

