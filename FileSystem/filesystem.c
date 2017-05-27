///CODIGOS
//401 FIL A KER - RESPUESTA HANDSHAKE DE FS
//499 FIL A OTR - RESPUESTA A CONEXION INCORRECTA
//805 mover cursor (esta solo en kernel)
//804 Escribir
//803 Abrir/Crear archivo
//802 Borrar
//801 Cerrar (esta solo en kernel)
//800 Leer

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
t_bitarray* bitmap;
metadata_Config* metadataSadica;
FILE* bitmapArchivo;

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
	montaje = configuracion->punto_montaje;


	metadataSadica = leerMetaData(montaje);
    bitmap = crearBitmap(montaje, (size_t)metadataSadica->cantidad_bloques);

    crearBloques(montaje, (int)metadataSadica->cantidad_bloques);

	creoSocket(&socketFileSystem, &direccionSocket, INADDR_ANY, configuracion->puerto);
	bindSocket(&socketFileSystem, &direccionSocket);
	escuchoSocket(&socketFileSystem);
}
int buscarPrimerBloqueLibre()
{
	int tamanioDisco = ((int)metadataSadica->cantidad_bloques * (int)metadataSadica->tamanio_bloques);
	int valor = 1;
	int i = 0;
	while(valor && i <= tamanioDisco) //busco el primer bloque vacio
	{
		i++;
		valor = 0;
	    valor = bitarray_test_bit(bitmap, (off_t) i);

	}
	if(i > tamanioDisco)
	{
		return -1;
	}
	else
	{
		bitarray_set_bit(bitmap,i);

		return i;
	}
}

void liberarEstructuras(){
	free(configuracion);
	free(montaje);;
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


					guardar_datos(mensajeAFileSystem[1], atoi(mensajeAFileSystem[2]), mensajeAFileSystem[3], atoi(mensajeAFileSystem[4]));

				break;

				case 803: //de CPU a File system (abrir)

					crear_archivo(mensajeAFileSystem[1], mensajeAFileSystem[2]);

					break;

				case 802:  //de CPU a File system (borrar)

					borrarArchivo(mensajeAFileSystem[1]);

					break;

				case 800://de CPU a File system (leer)

					obtener_datos(mensajeAFileSystem[1], atoi(mensajeAFileSystem[2]), mensajeAFileSystem[3], atoi(mensajeAFileSystem[4]));
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

	char* directorioAux = string_new();
	directorioAux = strtok(directorio, "\n");
	char* pathAbsoluto = string_new();
	string_append(&pathAbsoluto, montaje);
	string_append(&pathAbsoluto, directorioAux);

	int encontrar_sem(t_archivosFileSystem* archivo) {
		return string_starts_with(pathAbsoluto, archivo->path);
	}


	if(list_any_satisfy(lista_archivos, (void *) encontrar_sem))

	{
	return 2;

	}
	else
	{

		return 1;
	}
	free(pathAbsoluto);
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
t_metadataArch* leerMetadataDeArchivoCreado(FILE* arch)
{
	t_metadataArch* regMetadataArch = malloc(sizeof(t_metadataArch));
	regMetadataArch->bloquesEscritos = list_create();


        char* linea = string_new();
        char* cfline = string_new();;
        char** bloques;
		fgets(linea, sizeof(2048), arch);

		linea = strtok((char *) linea, "\n");
		cfline = strstr((char *) linea, "=");
		regMetadataArch->tamanio = atoi(cfline);

		fgets(linea, sizeof(2048), arch);
		linea = strstr((char *) linea, "[");
		cfline = strtok((char *) linea, "]");
		int cantBloques = substr_count(cfline, ",") + 1; //para saber cuantos bloques debo leer
		bloques = string_split((char *) cfline, ",");

		int i = -1; //porque los registros empiezan en 0
		while(i != cantBloques)
		{
			i++;
			list_add(regMetadataArch->bloquesEscritos, bloques[i]);

		}
		free(linea);
		free(cfline);
	 return regMetadataArch;
}


void crear_archivo(char* flag, char* directorio){

	  char* directorioAux = string_new();
      directorioAux = strtok(directorio, "\n"); //porque me agrega un \n al final del archivo
	  char* pathAbsoluto = string_new();
	  string_append(&pathAbsoluto, montaje);
	  string_append(&pathAbsoluto, directorioAux);

   	  int result = validar_archivo(directorioAux);

   	  if (result == 1)
   	  {
   		  FILE * pFile;
   		  pFile = fopen (pathAbsoluto,"w"); //por defecto lo crea
   		  t_archivosFileSystem* archNuevo = malloc(sizeof(t_archivosFileSystem));
   		  archNuevo->referenciaArchivo = pFile;

   		  archNuevo->path = string_new();
   		  string_append(&archNuevo->path, pathAbsoluto);

			char* tamanio = string_new();
			char* bloques = string_new();
			string_append(&tamanio, "TamanioeArchivo=0\n");

			int numeroBloque = buscarPrimerBloqueLibre(); //Por defecto tengo que asignarle un bloque
			string_append(&bloques, "Bloques=[");
			string_append(&bloques, string_itoa(numeroBloque));
			string_append(&bloques, "]");


		    fputs(tamanio, (FILE*)archNuevo->referenciaArchivo);
		    fseek((FILE*)archNuevo->referenciaArchivo, 0, 0);
		    fputs(bloques, (FILE*)archNuevo->referenciaArchivo);
		    fseek((FILE*)archNuevo->referenciaArchivo,string_length(bloques), SEEK_SET);


		    bitarray_set_bit(bitmap, numeroBloque);

		  list_add(lista_archivos, archNuevo);
		  free(tamanio);
		  free(bloques);

   	  }
   	  else
   	  {

			int encontrar_Arch(t_archivosFileSystem* archivo) {
				return string_starts_with(pathAbsoluto, archivo->path);
			}

		  t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_Arch);
		  list_remove_by_condition(lista_archivos,(void*) encontrar_Arch);
   		  FILE * pFileReabrir;
   		  pFileReabrir = freopen(pathAbsoluto,flag,archBuscado->referenciaArchivo); //le cambio el tipo de apertura
   		  archBuscado->referenciaArchivo = pFileReabrir;
   		  list_add(lista_archivos, archBuscado);
   	  }

	  free(pathAbsoluto);
}

void actualizarArchivoCreado(t_metadataArch* regArchivo, t_archivosFileSystem* arch)
{
	char* tamanio = string_new();
	char* bloques = string_new();
	string_append(&tamanio, "TamanioDeArchivo=");
	string_append(&tamanio, string_itoa((regArchivo->tamanio)));
	string_append(&tamanio, "\n");
	string_append(&bloques, "Bloques=[");
	int i = 0;
	while(regArchivo->bloquesEscritos->elements_count !=i)
	{
		i++;
		string_append(&bloques, string_itoa((int)list_get(regArchivo->bloquesEscritos, i)));
		string_append(&bloques, ", ");

	}

	string_append(&bloques, "]");

    fputs(tamanio, (FILE*)arch->referenciaArchivo);
    fseek((FILE*)arch->referenciaArchivo, 0, 0);
    fputs(bloques, (FILE*)arch->referenciaArchivo);
    fseek((FILE*)arch->referenciaArchivo,string_length(bloques), SEEK_SET);

	free(tamanio);
	free(bloques);
}

void obtener_datos(char* directorio, int size, char* l, int offset) {

		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");

		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);

        char* mensaje = string_new();

			int encontrar_sem(t_archivosFileSystem* archivo) {
				return string_starts_with(pathAbsoluto, archivo->path);
			}

			t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

			if (offset != -1)
			{
				t_metadataArch* regMetaArchBuscado =leerMetadataDeArchivoCreado((FILE*)archBuscado->referenciaArchivo);
				if(offset > (regMetaArchBuscado->tamanio * regMetaArchBuscado->bloquesEscritos->elements_count))
				{

				}
				fseek(archBuscado->referenciaArchivo, offset, SEEK_SET);
			}
	        fgets(pathAbsoluto, size, archBuscado->referenciaArchivo );

	    	enviarMensaje(&socketKernel, mensaje);
			free(pathAbsoluto);
			free(directorioAux);
	        return ;

	    } // End if file


void guardar_datos(char* directorio, int size, char* buffer, int offset){

		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");

		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);


		int encontrar_sem(t_archivosFileSystem* archivo) {
			return string_starts_with(pathAbsoluto, archivo->path);
		}

		t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

		if (offset != -1) //esto no esta bien. El offset tiene que estar guardado.
		{
			t_metadataArch* regMetaArchBuscado = leerMetadataDeArchivoCreado((FILE*)archBuscado->referenciaArchivo);
			if(offset > (regMetaArchBuscado->tamanio * regMetaArchBuscado->bloquesEscritos->elements_count))
			{
				if(!((offset+size) > (int)regMetaArchBuscado->tamanio * (int)regMetaArchBuscado->bloquesEscritos->elements_count)) //entra parte en los bloque que tiene, parte tiene que pedir
				{
					int bloquesApedir = (offset / regMetaArchBuscado->tamanio) - regMetaArchBuscado->bloquesEscritos->elements_count;
					if(bloquesApedir < 1) //si es un cachito, escribo uno
					{
						int unBloque = buscarPrimerBloqueLibre();
						if(unBloque != -1)
						{
							char* pathArchivoBloque = string_new();
							string_append(&pathArchivoBloque, montaje);
							string_append(&pathArchivoBloque, "Bloques/");
							string_append(&pathArchivoBloque, string_itoa(unBloque));
							string_append(&pathArchivoBloque, ".bin");
							FILE* archBloque = fopen(pathArchivoBloque,"w");


							fputs(buffer, archBloque); //grabo;
							fseek(archBloque, (offset - ((int)regMetaArchBuscado->tamanio * (int)regMetaArchBuscado->bloquesEscritos->elements_count)), SEEK_SET);

							regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + size;
							list_add(regMetaArchBuscado->bloquesEscritos, unBloque);


						    actualizarArchivoCreado(regMetaArchBuscado, archBuscado);

							bitarray_set_bit(bitmap, unBloque);

						}
						else
						{
							//dico lleno
						}
				   }
				  else
				  {
					  int cantidadDeBloquesApedir = (offset / regMetaArchBuscado->tamanio) - regMetaArchBuscado->bloquesEscritos->elements_count;
						int unBloque = buscarPrimerBloqueLibre();
				  }
				}
				else
				{

				}
			}
			else
			{

			}

		}
		else
		{
			//escribo dentro de los bloques que ya tiene
		}


		free(pathAbsoluto);
		free(directorioAux);
        //return mensaje;

    } // End if file

void borrarArchivo(char* directorio){

		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");
		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);

		int encontrar_sem(t_archivosFileSystem* archivo) {

			return (int)string_contains(pathAbsoluto, (char*)archivo->path);

		}

		t_archivosFileSystem* archDestroy = list_find(lista_archivos, (void *) encontrar_sem);

		printf("la lista tiene %s", (char*)archDestroy->path);
		//printf("la lista tiene %s", (char*)archDestroy->referenciaArchivo);
		int result = remove((char*)archDestroy->path);
		if(result == 0)
		{
			puts("El archivo ha sido eliminado con exito");
		}
		else
		{
			puts("El archivo no pudo eliminarse, revise el path");

		}
		fclose((FILE*)archDestroy->referenciaArchivo);
		list_remove_by_condition(lista_archivos, (void *) encontrar_sem);
		free(pathAbsoluto);
		free(directorioAux);
        //return mensaje;


    } // End if file
