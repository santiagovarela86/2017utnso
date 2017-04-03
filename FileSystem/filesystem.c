//CODIGOS
//401 FIL A KER - RESPUESTA HANDSHAKE DE FS
//499 FIL A OTR - RESPUESTA A CONEXION INCORRECTA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "configuracion.h"
#include "socketHelper.h"
#include <errno.h>

#define MAXBUF 1024

void esperoKernel(int * socketFileSystem, char * buffer);
void handShake(int * socketServer, int * socketCliente, int codigoEsperado, int codigoAceptado, int codigoRechazado);

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	char buffer[MAXBUF];
	int socketFileSystem;
	struct sockaddr_in direccionSocket;
	FileSystem_Config * configuracion;
	char* pathConfiguracion;

	pathConfiguracion = argv[1];
	configuracion = leerConfiguracion(pathConfiguracion);
	imprimirConfiguracion(configuracion);

	creoSocket(&socketFileSystem, &direccionSocket, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketFileSystem, &direccionSocket);
	escuchoSocket(&socketFileSystem);
	esperoKernel(&socketFileSystem, buffer);

	shutdown(socketFileSystem, 0);
	close(socketFileSystem);
	return EXIT_SUCCESS;
}

void esperoKernel(int * socketFileSystem, char * buffer) {
	while (1) {
		int socketCliente;
		struct sockaddr_in direccionCliente;
		int length = 0;

		socketCliente = accept(*socketFileSystem, (struct sockaddr_in *) &direccionCliente, (socklen_t*) &length);
		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShake(&socketFileSystem, &socketCliente, 100, 401, 499);

		//send(socketCliente, buffer, recv(socketCliente, buffer, MAXBUF, 0), 0);

		shutdown(socketCliente, 0);
		close(socketCliente);
	}
}

void handShake(int * socketServer, int * socketCliente, int codigoEsperado, int codigoAceptado, int codigoRechazado){
	char message[MAXBUF];
	char * codigo;

	while((recv(* socketCliente, message, sizeof(message), 0)) > 0){
		codigo = strtok(message, ";");

		if(atoi(codigo) == codigoEsperado){
			printf("Se acepto la conexion del Kernel \n");

			strcpy(message, strcat(string_itoa(codigoAceptado), ";"));
			enviarMensaje(socketCliente, message);

		}else{
			printf("Se rechazo una conexion incorrecta \n");

			strcpy(message, strcat(string_itoa(codigoRechazado), ";"));
			enviarMensaje(socketCliente, message);
		}
	}
}
