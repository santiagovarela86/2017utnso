//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include "configuracion.h"
#include "socketHelper.h"
#include "memoria.h"
#include <pthread.h>

int sumarizador_conecciones;
int tiempo_retardo;
Memoria_Config * config;
pthread_mutex_t mutex_tiempo_retardo = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
	char message[1000] = "";
	char server_reply[2000] = "";

	int socketMemoria;
	struct sockaddr_in direccionMemoria;
	int socketCliente;
	//struct sockaddr direccionCliente;
	int length = 0;
	sumarizador_conecciones = 0;

	if (argc != 2) {
		printf("Error. Parametros incorrectos \n");
		return EXIT_FAILURE;
	}

	config = leer_configuracion(argv[1]);
	imprimir_configuracion(config);

	pthread_mutex_lock(&mutex_tiempo_retardo);
	tiempo_retardo = config->retardo_memoria;
	pthread_mutex_unlock(&mutex_tiempo_retardo);

	creoSocket(&socketMemoria, &direccionMemoria, INADDR_ANY, config->puerto);
	bindSocket(&socketMemoria, &direccionMemoria);
	listen(socketMemoria, 3);

	inicializar_estructuras_administrativas(config);

	pthread_t hilo_consola_memoria;
	if (pthread_create(&hilo_consola_memoria, NULL, inicializar_consola, NULL) < 0) {
		perror("Error al crear el Hilo de Consola Memoria");
		return EXIT_FAILURE;
	}

	while ((socketCliente = accept(socketMemoria, (struct sockaddr *) &direccionMemoria, (socklen_t*) &length))) {
		pthread_t thread_id;
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

void * inicializar_consola(void* args){

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

		int numero = 0;
		int numero_correcto = 0 ;
		int intentos_fallidos = 0;
		int nuevo_tiempo_retardo = 0;

		while(numero_correcto == 0 && intentos_fallidos < 3){

			scanf("%d", &numero);

			if(intentos_fallidos == 3){
				return EXIT_FAILURE;
			}

			switch(numero){
				case 1:
					numero_correcto = 1;
					puts("Tiempo de retardo:");
					scanf("%d", &nuevo_tiempo_retardo);
					pthread_mutex_lock(&mutex_tiempo_retardo);
					tiempo_retardo = nuevo_tiempo_retardo;
					printf("Ahora el tiempo de retardo es: %d \n", tiempo_retardo);
					pthread_mutex_unlock(&mutex_tiempo_retardo);
					break;
				case 2:
					numero_correcto = 1;
					break;
				default:
					intentos_fallidos++;
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
	int bloque_memoria = malloc(sizeof(tamanio_memoria));
	if (bloque_memoria == NULL){
		perror("No se pudo reservar el bloque de memoria del Sistema\n");
	}

	t_list* tabla_paginas = list_create();
	t_list* memoria_cache = list_create();


}

void* handler_conexion(void *socket_desc) {
	sumarizador_conecciones++;
	printf("Tengo %d Conectados \n", sumarizador_conecciones);

	estructura_socket* est_socket = malloc(sizeof(estructura_socket));
	est_socket = (estructura_socket*) socket_desc;

	char consola_message[1000] = "";
	char* codigo;

	while ((recv(est_socket->socket_consola, consola_message,
			sizeof(consola_message), 0)) > 0) {
		codigo = strtok(consola_message, ";");

		if (atoi(codigo) == 100) {
			printf("Se acepto la conexion del Kernel \n");

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '1';
			consola_message[3] = ';';

			if (send(est_socket->socket_consola, consola_message,
					strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}

		} else if (atoi(codigo) == 500) {
			printf("Se acepto la conexion de la CPU \n");

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '2';
			consola_message[3] = ';';

			if (send(est_socket->socket_consola, consola_message,
					strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		} else {
			printf("Se rechazo una conexion incorrecta \n");

			consola_message[0] = '2';
			consola_message[1] = '9';
			consola_message[2] = '9';
			consola_message[3] = ';';

			if (send(est_socket->socket_consola, consola_message,
					strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		}
	}

	while (1) {
	}

	return EXIT_SUCCESS;
}

