#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helperFunctions.h"
#include <pthread.h>

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto) {

	*sock = socket(AF_INET, SOCK_STREAM, 0);

	if (*sock < 0) {
		perror("Error al Crear el Socket");
		exit(errno);
	}

	memset(direccion, 0, sizeof(*direccion));

	direccion->sin_family = AF_INET;
	direccion->sin_addr.s_addr = ip;
	direccion->sin_port = htons(puerto);
}

void bindSocket(int * sock, struct sockaddr_in * direccion) {

	int resultado = bind(*sock, direccion, sizeof(*direccion));

	if (resultado != 0) {
		perror("Error en hacer Bind al Socket");
		exit(errno);
	}
}

void escuchoSocket(int * sock) {

	int resultado = listen(*sock, 20);

	if (resultado != 0) {
		perror("Error al escuchar Socket");
		exit(errno);
	}
}

void conectarSocket(int * sock, struct sockaddr_in * direccion){

	int resultado = connect(* sock, (struct sockaddr *) direccion, sizeof(* direccion));

	if (resultado < 0) {
		perror("Error al conectarse al Socket");
		exit(errno);
	}
}

void enviarMensaje(int * sock, char * message){
	int resultado = send(* sock, message, strlen(message), 0);

	if (resultado < 0) {
		puts("Error al enviar mensaje");
		exit(errno);
	}
}

void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args) {
	int resultado = pthread_create(threadID, NULL, threadHandler, args);

	if (resultado < 0) {
		perror("Error al crear el Hilo");
		exit(errno);
	}
}

void handShakeListen(int * socketCliente, char * codigoEsperado, char * codigoAceptado, char * codigoRechazado, char * proceso){
	char message[MAXBUF];
	char * codigo;
	char * separador = ";";

	int result = recv(* socketCliente, message, sizeof(message), 0);

	if (result > 0) {
		codigo = strtok(message, separador);

		if(strcmp(codigo, codigoEsperado) == 0){
			printf("Se acepto la conexion del proceso %s \n", proceso);
			strcpy(message, codigoAceptado);
			strcat(message, separador);
			enviarMensaje(socketCliente, message);
		}else{
			strcpy(message, codigoRechazado);
			strcat(message, separador);
			printf("Se rechazo la conexion del proceso %s \n", proceso);
			enviarMensaje(socketCliente, message);
		}
	} else {
		printf("Error al recibir datos del %s", proceso);
	}

}

void handShakeSend(int * socketServer, char * codigoEnvio, char * codigoEsperado, char * proceso){
	char message[MAXBUF];
	char * codigo;
	char * separador = ";";

	strcpy(message, codigoEnvio);
	strcat(message, separador);
	enviarMensaje(socketServer, message);

	int result = recv(* socketServer, message, sizeof(message), 0);

	if (result > 0) {
		codigo = strtok(message, separador);

		if (strcmp(codigo, codigoEsperado) == 0) {
			printf("El proceso %s acepto la conexion \n", proceso);
			printf("\n");
		} else {
			printf("El proceso %s rechazo la conexion \n", proceso);
			exit(errno);
		}
	} else {
		printf("Error al recibir datos del %s", proceso);
	}

}
