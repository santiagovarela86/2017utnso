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

int socketKernel;
char* montaje;
t_list* lista_archivos;
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

	lista_archivos = list_create();

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
				case 804://de CPU a File system (cerrar)
					guardar_datos(mensajeAFileSystem[1],mensajeAFileSystem[2],mensajeAFileSystem[3],mensajeAFileSystem[4]);

				break;

				case 803: //de CPU a File system (abrir)

					crear_archivo(mensajeAFileSystem[1], mensajeAFileSystem[2]);

					break;

				case 802:  //de CPU a File system (borrar)

					borrarArchivo(mensajeAFileSystem[1]);

					break;


				case 800://de CPU a File system (leer)

					obtener_datos(mensajeAFileSystem[3],atoi(mensajeAFileSystem[1]),atoi(mensajeAFileSystem[2]));
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
}


/*char* abrir_archivo(char* directorio){


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
	archAbrir = list_find(lista_archivos,(void*) encontrar_arch);
	list_remove_by_condition(lista_archivos,(void*) encontrar_arch);
	list_add(lista_archivos,archAbrir);

	archAbrir->cantidadDeAperturas = 1;
	archAbrir->fileDescriptor = fd_script;
	archAbrir->path = directorio;

	list_add(lista_archivos, archAbrir);*/

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
		}

	close(fd_script);
	char* extra = string_new();
	return extra;
}
*/
int validar_archivo(char* directorio){

	int encontrar_sem(t_archivosFileSystem* archivo) {
		return string_starts_with(directorio, archivo->path);
	}


	if(list_any_satisfy(lista_archivos, (void *) encontrar_sem))

	{
	return 2;

	}
	else
	{
		return 1;
	}
	/*char* path = string_new();

	path = string_substring(montaje,1,string_length(montaje));

	string_append(&path,"Archivos/");
	string_append(&path,directorio);

	if (fopen(path, "r") == NULL){
	 //El archivo no existe
		return -1;
	}else{
	 //El archivo existe
	  FILE * pFile;
	  pFile = fopen (directorio,"w"); //por defecto lo crea
	  if (pFile!=NULL)
	  {
	    fputs ("asigno_Bloque_Prueba",pFile);
		int fd_script = open(directorio, O_WRONLY);
		struct stat scriptFileStat;
		fstat(fd_script, &scriptFileStat);

			char* arch = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);
			list_add(lista_archivos, arch);
			close(fd_script);
		    fclose (pFile);
			return arch;
	  	  }
	    return 0;*/
		return 1;
	}

void crear_archivo(char* flag, char* directorio){

   	  int result = validar_archivo(directorio);

   	  if (result == 1)
   	  {

   		  FILE * pFile;
   		  pFile = fopen (directorio,"w"); //por defecto lo crea
   		  t_archivosFileSystem* archNuevo = malloc(sizeof(t_archivosFileSystem));
   		  archNuevo->referenciaArchivo = pFile;
   		  archNuevo->path = directorio;
		  list_add(lista_archivos, directorio);

   	  }
   	  else
   	  {
			int encontrar_Arch(t_archivosFileSystem* archivo) {
				return string_starts_with(directorio, archivo->path);
			}

		  t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_Arch);
		  list_remove_by_condition(lista_archivos,(void*) encontrar_Arch);
   		  FILE * pFileReabrir;
   		  pFileReabrir = freopen(directorio,flag,archBuscado->referenciaArchivo); //le cambio el tipo de apertura
   		  archBuscado->referenciaArchivo = pFileReabrir;
   		  list_add(lista_archivos, archBuscado);
   	  }

}

void obtener_datos(char* directorio, int offset, int size){

        char* mensaje = string_new();

			int encontrar_sem(t_archivosFileSystem* archivo) {
				return string_starts_with(directorio, archivo->path);
			}

			t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

	        fseek(archBuscado->referenciaArchivo,offset,SEEK_SET);
	        fgets(mensaje, size, archBuscado->referenciaArchivo );

	    	enviarMensaje(&socketKernel, mensaje);
	        return ;

	    } // End if file


void guardar_datos(char* directorio, int offset, int size, char* buffer){


		int encontrar_sem(t_archivosFileSystem* archivo) {
			return string_starts_with(directorio, archivo->path);
		}

		t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

        fseek(archBuscado->referenciaArchivo,offset,SEEK_SET);
        fputs((FILE*)archBuscado->referenciaArchivo, buffer);

        //return mensaje;

    } // End if file

void borrarArchivo(char* directorio){

		int encontrar_sem(t_archivosFileSystem* archivo) {
			return string_starts_with(directorio, archivo->path);
		}

		t_archivosFileSystem* archDestroy = list_find(lista_archivos, (void *) encontrar_sem);
		close((FILE*)archDestroy->referenciaArchivo);
		list_remove_by_condition(lista_archivos, (void *) encontrar_sem);

        //return mensaje;

    } // End if file

