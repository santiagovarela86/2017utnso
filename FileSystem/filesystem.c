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
t_list* lista_File_global;
t_list* lista_File_proceso;
FileSystem_Config * configuracion;

int socketFileSystem;
struct sockaddr_in direccionSocket;

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	char * pathConfig = argv[1];
	inicializarEstructuras(pathConfig);

	imprimirConfiguracion(configuracion);

	pthread_t thread_kernel;
	creoThread(&thread_kernel, hilo_conexiones_kernel, NULL);

	pthread_join(thread_kernel, NULL);

	liberarEstructuras();

	return EXIT_SUCCESS;
}

void inicializarEstructuras(char * pathConfig){
	configuracion = leerConfiguracion(pathConfig);

	lista_File_global = list_create();
	lista_File_proceso = list_create();

	montaje = string_new();

	creoSocket(&socketFileSystem, &direccionSocket, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketFileSystem, &direccionSocket);
	escuchoSocket(&socketFileSystem);
}

void liberarEstructuras(){
	free(configuracion);
	free(montaje);

	shutdown(socketFileSystem, 0);
	close(socketFileSystem);
}

void * hilo_conexiones_kernel(void * args){

	int socketKernel;
	struct sockaddr_in direccionKernel;
	socklen_t length = sizeof direccionKernel;

	socketKernel = accept(socketFileSystem, (struct sockaddr *) &direccionKernel, &length);

	if (socketKernel > 0) {
		printf("%s:%d conectado\n", inet_ntoa(direccionKernel.sin_addr), ntohs(direccionKernel.sin_port));
		handShakeListen(&socketKernel, "100", "401", "499", "Kernel");
		char message[MAXBUF];

		int result = recv(socketKernel, message, sizeof(message), 0);

		while (result > 0) {
			printf("%s", message);

			char**mensajeAFileSystem = string_split(message, ";");
			int codigo = atoi(mensajeAFileSystem[0]);

			switch (codigo){

				case 803: //de CPU a File system (abrir)

					if("r" == mensajeAFileSystem[1]){
						abrir_archivo(mensajeAFileSystem[0]);
					}
					if("c" == mensajeAFileSystem[1]){
						crear_archivo(mensajeAFileSystem[0]);
					}

					break;

				case 802:  //de CPU a File system (borrar)

					break;

				case 801://de CPU a File system (cerrar)


					break;

			}

			result = recv(socketKernel, message, sizeof(message), 0);
		}

			if (result <= 0) {
				printf("Se desconecto el Kernel\n");
			}
		} else {
			perror("Fallo en el manejo del hilo Kernel");
			return EXIT_FAILURE;
   	}

	shutdown(socketKernel, 0);
	close(socketKernel);

	return EXIT_SUCCESS;
}

		/*int socketCliente;
		struct sockaddr_in direccionCliente;
		socklen_t length = sizeof direccionKernel;

		socketCliente = accept(threadSocketInfoKernel->sock, (struct sockaddr *) &direccionCliente, &length);

		printf("%s:%d conectado\n", inet_ntoa(direccionCliente.sin_addr), ntohs(direccionCliente.sin_port));

		handShakeListen(&socketCliente, "100", "401", "499", "Kernel");

		atender_peticiones(socketCliente);

		shutdown(socketCliente, 0);
		close(socketCliente);

		//Lo comento porque es un while 1
		//La idea seria usar el message en el while de arriba para transmitirse mensajes
		//entre el kernel y el FS
		//atender_peticiones(socketCliente);*/


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
					int i= 0;
				while (i < atoi(message[1])){
					t_fileProceso * elem = malloc(sizeof(elem));
					elem->fileDescriptor = atoi(message[i]);
					i++;
					elem->flags = (message[i]);
					i++;
					elem->global_fd = atoi(message[i]);
					i++;
					list_add(lista_File_proceso, elem);
				}
	}
*/
	while(1){

	}
}


char* abrir_archivo(char* directorio){


	int fd_script = open(directorio, O_RDWR);
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);

	t_fileGlobal* archReemplazar = malloc(sizeof(t_fileGlobal));
	archReemplazar->cantidadDeAperturas = 0;
	archReemplazar->fileDescriptor = fd_script;
	archReemplazar->path = directorio;

	int encontrar_arch(t_fileGlobal* glo){
		return string_equals_ignore_case(directorio, glo->path);
	}
	t_fileGlobal* archAbrir = malloc(sizeof(t_fileGlobal));
	archAbrir = list_find(lista_File_global,(void*) encontrar_arch);
	list_remove_by_condition(lista_File_global,(void*) encontrar_arch);
	list_add(lista_File_global,archAbrir);

	archAbrir->cantidadDeAperturas = 1;
	archAbrir->fileDescriptor = fd_script;
	archAbrir->path = directorio;

	list_add(lista_File_global, archAbrir);

		//list_add()
	/*	if(permiso == 'r'){
			char* arch = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);
			close(fd_script);
			return arch;
		}else if(permiso == 'w'){
			char* arch = mmap(0, scriptFileStat.st_size, PROT_WRITE, MAP_SHARED, fd_script, 0);
			close(fd_script);
			return arch;
		}else{
			puts("Permiso incorrecto");
		}*/

	close(fd_script);
	char* extra = string_new();
	return extra;
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
char* crear_archivo(char* directorio){
	  FILE * pFile;
	  pFile = fopen (directorio,"w"); //por defecto lo crea
	  if (pFile!=NULL)
	  {
	    fputs ("asigno_Bloque_Prueba",pFile);
		int fd_script = open(directorio, O_WRONLY);

		t_fileGlobal* archNuevo = malloc(sizeof(t_fileGlobal));
		archNuevo->cantidadDeAperturas = 0;
		archNuevo->fileDescriptor = fd_script;
		archNuevo->path = directorio;

		list_add(lista_File_global, archNuevo);

		struct stat scriptFileStat;
		fstat(fd_script, &scriptFileStat);

			char* arch = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);
			close(fd_script);
		    fclose (pFile);
			return arch;
	  	  }
	    return 0;
      fclose(pFile);
}

char* obtener_datos(char* directorio, int offset, int size, char permiso){

	  FILE *file;
	  file = fopen(directorio, "r");
	    if(permiso == 'r'){
	    if (file != NULL) {
	        char* mensaje = string_new();

	        fseek(file,offset,SEEK_SET);
	        fgets(mensaje, size, file );

	        fclose(file);
	        return mensaje;

	    } // End if file

	}
	else
	{
		puts("Permiso incorrecto");
	}
	fclose(file);
	return 0;

}

char* guardar_datos(char* directorio, int offset, int size, char* buffer, char permiso){

   FILE *file;
   file = fopen(directorio, "w");

	if(permiso == 'w'){

	    if (file != NULL) {
	        char* mensaje = string_new();

	        fseek(file,offset,SEEK_SET);
	        fputs(buffer, file );

	        fclose(file);
	        return mensaje;

	    } // End if file
	   }

    	else
    	{
    		puts("Permiso incorrecto");
    	}
	  fclose(file);
	return 0;

}

