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
				case 804:
					guardar_datos(mensajeAFileSystem[1], atoi(mensajeAFileSystem[2]), mensajeAFileSystem[3], atoi(mensajeAFileSystem[4]));
				break;

				case 803:
					crear_archivo(mensajeAFileSystem[1], mensajeAFileSystem[2]);
			    break;

				case 802:
					borrarArchivo(mensajeAFileSystem[1]);
				break;

				case 800:
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
		    fseek((FILE*)archNuevo->referenciaArchivo, string_length(tamanio), 0);
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

FILE* abrirUnArchivoBloque(int idBloque)
{
	char* pathArchivoBloque = string_new();
	string_append(&pathArchivoBloque, montaje);
	string_append(&pathArchivoBloque, "Bloques/");
	string_append(&pathArchivoBloque, string_itoa(idBloque));
	string_append(&pathArchivoBloque, ".bin");

	FILE* archBloque = fopen(pathArchivoBloque,"w");

	return archBloque;
}

void grabarUnArchivoBloque(FILE* archBloque, int idBloque, char* buffer, int size)
{
	bitarray_set_bit(bitmap, idBloque);
	fputs(buffer, archBloque); //grabo;
	fseek(archBloque, size, SEEK_SET);
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

void obtener_datos(char* directorio, int size, char* buffer, int offset) {

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

			int bloquePosicion = offset / ((int)metadataSadica->tamanio_bloques);

			int idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion); //bloque donde comienza lo que quiero leer

			FILE* archBloqueAleer= abrirUnArchivoBloque(idbloqueALeer);

			if(size > metadataSadica->tamanio_bloques)
			{
				int cantidadDeBloquesALeer = size / metadataSadica->tamanio_bloques;
				if((size % metadataSadica->tamanio_bloques) != 0)
				{
					cantidadDeBloquesALeer++;
				}
				while(cantidadDeBloquesALeer != 0)
				{
					if(cantidadDeBloquesALeer != 1)
					{
						char* valorLeido = fgets(pathAbsoluto, metadataSadica->tamanio_bloques, archBuscado->referenciaArchivo);
						fseek(archBloqueAleer, metadataSadica->tamanio_bloques, SEEK_SET);
						string_append(buffer, (char*)valorLeido);
						idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion+1); //bloque donde comienza lo que quiero leer
						archBloqueAleer= abrirUnArchivoBloque(idbloqueALeer);
					}
					else
					{
						char* valorLeido = fgets(pathAbsoluto, metadataSadica->tamanio_bloques, archBuscado->referenciaArchivo);
						fseek(archBloqueAleer,(size % metadataSadica->tamanio_bloques), SEEK_SET);
						string_append(buffer, (char*)valorLeido);

					}
					cantidadDeBloquesALeer--;
				}
			}
			else
			{
				fseek(archBloqueAleer, size, SEEK_SET);
				fgets(pathAbsoluto, size, archBuscado->referenciaArchivo );
			}

		}
		enviarMensaje(&socketKernel, mensaje);
		free(pathAbsoluto);
		free(directorioAux);
		return ;

	} // End if file


void pidoBloquesEnBlancoYgrabo(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size )
{
       if((offset -(regMetaArchBuscado->bloquesEscritos->elements_count * regMetaArchBuscado->tamanio) / metadataSadica->tamanio_bloques) > 1) // hay saltos de bloques?
	   {
		  int cantBloquesEnBlanco =  (offset -(regMetaArchBuscado->bloquesEscritos->elements_count * regMetaArchBuscado->tamanio) / metadataSadica->tamanio_bloques);
		  while(cantBloquesEnBlanco != 0)  //meto bloques en blanco exceptuando el que voy a escribir
		  {

			int unBloqueEnBlanco = buscarPrimerBloqueLibre();
			if(unBloqueEnBlanco != -1)
			{
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				list_add(regMetaArchBuscado->bloquesEscritos, unBloqueEnBlanco);
				bitarray_set_bit(bitmap, unBloqueEnBlanco);
			}
			else
			{
				//disco lleno
			}
		  }
		}
		int cantidadDeBloquesApedir = (size) / metadataSadica->tamanio_bloques; // el "/" hace división entera no mas, sin resto
		if(((size) % metadataSadica->tamanio_bloques) != 0) //pregunto si hay resto, o sea un cachito de bloque mas
		{
			  cantidadDeBloquesApedir++;
		}
		while(cantidadDeBloquesApedir != 0)
		{
		 int unBloque = buscarPrimerBloqueLibre();
		 if(unBloque != -1)
		 {
			FILE* archBloque = abrirUnArchivoBloque(unBloque);

			if(cantidadDeBloquesApedir != 1) //porque el utlimo no lo va a grabar entero
			{
				char* bufferaux = string_substring(buffer,0,(metadataSadica->tamanio_bloques));
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				list_add(regMetaArchBuscado->bloquesEscritos, unBloque);
				grabarUnArchivoBloque(archBloque, unBloque, bufferaux, metadataSadica->tamanio_bloques); //meto todo el bloque

			}
			else
			{
				char* bufferaux = string_substring(buffer,0, ((size) % metadataSadica->tamanio_bloques)); //lo que me falta grabar
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + ((size) % metadataSadica->tamanio_bloques);
				list_add(regMetaArchBuscado->bloquesEscritos, unBloque);
				grabarUnArchivoBloque(archBloque, unBloque, bufferaux, ((size) % metadataSadica->tamanio_bloques));
			}

			cantidadDeBloquesApedir--;
		 }
	else
	{
		//disco lleno
	}
   }
 }

void grabarParteEnbloquesYparteEnNuevos(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size )
{
	  int bloquePosicion = offset / ((int)metadataSadica->tamanio_bloques);
	  int idbloqueAGrabar = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion +1); //bloque donde comienza lo que quiero grabar

	  FILE* archGrabar= abrirUnArchivoBloque(idbloqueAGrabar);

	  char* bufferaux = string_substring(buffer,0, ( metadataSadica->tamanio_bloques -  (offset % ((int)metadataSadica->tamanio_bloques)))); //lo que me falta grabar


	  regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + string_length(bufferaux);
	  grabarUnArchivoBloque(archGrabar, idbloqueAGrabar, bufferaux, string_length(bufferaux));

	  char* bufferNuevo = string_substring_from(buffer,string_length(bufferaux));
	  pidoBloquesEnBlancoYgrabo(offset - string_length(bufferaux),regMetaArchBuscado,buffer,size - string_length(bufferaux));
}


void graboEnLosBloquesQueYaTiene(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size )
{
	int bloquePosicion = offset / ((int)metadataSadica->tamanio_bloques);

    if(size > metadataSadica->tamanio_bloques)
	{
		bloquePosicion = 1;
	}
	int idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion); //bloque donde comienza lo que quiero leer

	FILE* archBloqueAGrabar= abrirUnArchivoBloque(idbloqueALeer);

	if(size > metadataSadica->tamanio_bloques)
	{
	int cantidadDeBloquesALeer = size / metadataSadica->tamanio_bloques;
		if((size % metadataSadica->tamanio_bloques) != 0)
		{
			cantidadDeBloquesALeer++;
		}
		while(cantidadDeBloquesALeer != 0)
		{
			if(cantidadDeBloquesALeer != 1)
			{
				char* bufferaux = string_substring(buffer,0,(metadataSadica->tamanio_bloques));
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, bufferaux, metadataSadica->tamanio_bloques); //meto todo el bloque
				idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion+1); //bloque donde comienza lo que quiero leer
				archBloqueAGrabar= abrirUnArchivoBloque(idbloqueALeer);
			}
			else
			{
				char* bufferaux = string_substring(buffer,0, ((size) % metadataSadica->tamanio_bloques)); //lo que me falta grabar
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + ((size) % metadataSadica->tamanio_bloques);
				grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, bufferaux, ((size) % metadataSadica->tamanio_bloques));
			}
			cantidadDeBloquesALeer--;
		}
	}
	else
	{
		grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, buffer, size); //meto todo el bloque
	}
}



void guardar_datos(char* directorio, int size, char* buffer, int offset)
{
		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");

		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);

		int encontrar_sem(t_archivosFileSystem* archivo) {
			return string_starts_with(pathAbsoluto, archivo->path);
		}

		t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

			t_metadataArch* regMetaArchBuscado = leerMetadataDeArchivoCreado((FILE*)archBuscado->referenciaArchivo);
			int posicionesParaGuardar = (int)regMetaArchBuscado->bloquesEscritos->elements_count * (int)metadataSadica->tamanio_bloques;
			if(offset+size < posicionesParaGuardar) //entra todo en los arch/bloques que ya tiene
			{
         		graboEnLosBloquesQueYaTiene(offset, regMetaArchBuscado, buffer, size);
			}
			else
			{
			  if((offset < ((int)regMetaArchBuscado->tamanio)) && ((offset+size) > ((int)regMetaArchBuscado->tamanio))) //entra parte en los bloque que tiene, parte tiene que pedir
			  {
				  grabarParteEnbloquesYparteEnNuevos(offset, regMetaArchBuscado, buffer, size );
		      }
			  else //no entra nada, pido bloques y grabo todo en ellos
			  {
				  pidoBloquesEnBlancoYgrabo(offset,regMetaArchBuscado,buffer,size);
			  }
		}
   		actualizarArchivoCreado(regMetaArchBuscado, archBuscado);
   	 free(pathAbsoluto);
   	 free(directorioAux);
 }
