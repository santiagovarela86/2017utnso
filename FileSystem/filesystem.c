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
#include <errno.h>
#include <pthread.h>
#include "helperFunctions.h"

void * handler_kernel(void * args);
void atender_peticiones(int socket);

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	FileSystem_Config * configuracion;
	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	metadata_Config* metadata;
	metadata = leerMetaData(configuracion->punto_montaje);
	imprimirMetadata(metadata);

	int socketFileSystem;
	struct sockaddr_in direccionSocket;
	creoSocket(&socketFileSystem, &direccionSocket, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketFileSystem, &direccionSocket);
	escuchoSocket(&socketFileSystem);

	threadSocketInfo * threadSocketInfoKernel;
	threadSocketInfoKernel = malloc(sizeof(struct threadSocketInfo));
	threadSocketInfoKernel->sock = socketFileSystem;
	threadSocketInfoKernel->direccion = direccionSocket;

	pthread_t thread_kernel;
	creoThread(&thread_kernel, handler_kernel, threadSocketInfoKernel);

	pthread_join(thread_kernel, NULL);

	free(configuracion);
	free(threadSocketInfoKernel);

	shutdown(socketFileSystem, 0);
	close(socketFileSystem);

	return EXIT_SUCCESS;
}

void * handler_kernel(void * args){
	threadSocketInfo * threadSocketInfoKernel = (threadSocketInfo *) args;

	while (1) {
		int socketCliente;
		struct sockaddr_in direccionCliente;
		socklen_t length = sizeof direccionCliente;

		socketCliente = accept(threadSocketInfoKernel->sock, (struct sockaddr *) &direccionCliente, &length);

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShake(threadSocketInfoKernel->sock, &socketCliente, 100, 401, 499, "Kernel");

		atender_peticiones(socketCliente);

		shutdown(socketCliente, 0);
		close(socketCliente);
	}

	return EXIT_SUCCESS;
}

void atender_peticiones(int socket){



	while(1){

	}
}
