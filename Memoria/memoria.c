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
#include "configuracion.h"
#include "socketHelper.h"

typedef struct {
	int socket_consola;
} estructura_socket;

void* handler_conexion(void *socket_desc);
int sumarizador_conecciones;

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

	Memoria_Config * config = leer_configuracion(argv[1]);
	imprimir_configuracion(config);

	creoSocket(&socketMemoria, &direccionMemoria, INADDR_ANY, config->puerto);
	bindSocket(&socketMemoria, &direccionMemoria);
	listen(socketMemoria, 3);

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

	return 0;
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

	return 0;
}

