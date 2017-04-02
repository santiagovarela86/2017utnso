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

#define MAXBUF 1024

void aceptoConexiones(int * socketFileSystem, char * buffer);

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos \n");
		return EXIT_FAILURE;
	}

	char buffer[MAXBUF];
	int socketFileSystem;
	struct sockaddr_in direccionSocket;
	FileSystem_Config * configuracion;
	char* pathConfiguracion;

	pathConfiguracion = argv[1];
	configuracion = cargar_config(pathConfiguracion);
	imprimir_config(configuracion);
	creoSocket(&socketFileSystem, &direccionSocket, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketFileSystem, &direccionSocket);
	escuchoSocket(&socketFileSystem);
	aceptoConexiones(&socketFileSystem, buffer);

	close(socketFileSystem);
	return EXIT_SUCCESS;
}

void aceptoConexiones(int * socketFileSystem, char * buffer) {
	char consola_message[1000] = "";
	char* codigo;

	while (1) {
		int socketCliente;
		struct sockaddr_in direccionCliente;
		uint addrlen = sizeof(direccionCliente);

		socketCliente = accept(*socketFileSystem,
				(struct sockaddr_in *) &direccionCliente, &addrlen);
		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr),
				ntohs(direccionCliente.sin_port));

		while((recv(socketCliente, consola_message, sizeof(consola_message), 0)) > 0)
		{
			codigo = strtok(consola_message, ";");

			if(atoi(codigo) == 100){
				printf("Se acepto la conexion del Kernel \n");

				consola_message[0] = '4';
				consola_message[1] = '0';
				consola_message[2] = '1';
				consola_message[3] = ';';

			    if(send(socketCliente, consola_message, strlen(consola_message) , 0) < 0)
			    {
			        puts("Fallo el envio al servidor");
			    }

			}else{
				printf("Se rechazo una conexion incorrecta \n");

				consola_message[0] = '4';
				consola_message[1] = '9';
				consola_message[2] = '9';
				consola_message[3] = ';';

			    if(send(socketCliente, consola_message, strlen(consola_message) , 0) < 0)
			    {
			        puts("Fallo el envio al servidor");
			    }
			}

		}

		send(socketCliente, buffer, recv(socketCliente, buffer, MAXBUF, 0), 0);

		close(socketCliente);
	}
}
