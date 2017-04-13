//CODIGOS
//401 FIL A KER - RESPUESTA HANDSHAKE DE FS
//499 FIL A OTR - RESPUESTA A CONEXION INCORRECTA

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/bitarray.h>
#include "configuracion.h"
#include <errno.h>
#include <pthread.h>
#include "helperFunctions.h"
#include "filesystem.h"

char* montaje;

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	montaje = string_new();

	FileSystem_Config * configuracion;
	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	montaje = configuracion->punto_montaje;

	metadata_Config* metadata;
	metadata = leerMetaData(configuracion->punto_montaje);
	imprimirMetadata(metadata);

	size_t tamanio_bitmap = (metadata->cantidad_bloques);
	t_bitarray* bitmap = crearBitmap(configuracion->punto_montaje, tamanio_bitmap);

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
	creoThread(&thread_kernel, hilo_conexiones_kernel, threadSocketInfoKernel);

	pthread_join(thread_kernel, NULL);

	free(configuracion);
	free(threadSocketInfoKernel);

	shutdown(socketFileSystem, 0);
	close(socketFileSystem);

	return EXIT_SUCCESS;
}

void * hilo_conexiones_kernel(void * args){
	threadSocketInfo * threadSocketInfoKernel = (threadSocketInfo *) args;

	int socketCliente;
	struct sockaddr_in direccionCliente;
	socklen_t length = sizeof direccionCliente;

	socketCliente = accept(threadSocketInfoKernel->sock, (struct sockaddr *) &direccionCliente, &length);

	if (socketCliente) {
		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));
		handShakeListen(&socketCliente, "100", "401", "499", "Kernel");
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
		int socketCliente;
		struct sockaddr_in direccionCliente;
		socklen_t length = sizeof direccionCliente;

		socketCliente = accept(threadSocketInfoKernel->sock, (struct sockaddr *) &direccionCliente, &length);

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShakeListen(&socketCliente, "100", "401", "499", "Kernel");

		atender_peticiones(socketCliente);

		shutdown(socketCliente, 0);
		close(socketCliente);
	*/

	//Lo comento porque es un while 1
	//La idea seria usar el message en el while de arriba para transmitirse mensajes
	//entre el kernel y el FS
	//atender_peticiones(socketCliente);

	shutdown(socketCliente, 0);
	close(socketCliente);

	return EXIT_SUCCESS;
}

void atender_peticiones(int socket){
/*
	puts("escriba path archivo");
	char directorio[1000];

	scanf("%s", directorio);

	char permi = 'r';

	if(validar_archivo(directorio) == 1){

		char* archi = abrir_archivo(directorio, permi);

	}else{
	puts("El archivo no existe");
	}
*/
	while(1){

	}
}

char* abrir_archivo(char* directorio, char permiso){
	int fd_script = open(directorio, O_RDWR);
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);

	if(permiso == 'r'){
		char* arch = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);
		close(fd_script);
		return arch;
	}else if(permiso == 'w'){
		char* arch = mmap(0, scriptFileStat.st_size, PROT_WRITE, MAP_SHARED, fd_script, 0);
		close(fd_script);
		return arch;
	}else{
		puts("Permiso incorrecto");
	}

	close(fd_script);
	return 0;

}

int validar_archivo(char* directorio){

	char* path = string_new();

	path = string_substring(montaje,1,string_length(montaje));

	string_append(&path,"Archivos/");
	string_append(&path,directorio);

	if (fopen(path, "r") == NULL){
	 //El archivo no existe
		return -1;
	}else{
	 //El archivo existe
		return 1;
	}

}
