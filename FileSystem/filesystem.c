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

	char* montajeInicial = string_new();
	montajeInicial = configuracion->punto_montaje;
	char* aux = string_substring(montajeInicial,string_length(montajeInicial)-1,string_length(montajeInicial));
	printf("%s\n",aux);
	if(string_equals_ignore_case(aux,"/"))
	{
		montaje = montajeInicial;
	}
	else
	{
		montaje = montajeInicial;
		string_append(&montaje,"/");
	}
	metadataSadica = leerMetaData(montaje);
    bitmap = crearBitmap(montaje, (size_t)metadataSadica->cantidad_bloques);

    crearBloques(montaje, (int)metadataSadica->cantidad_bloques, (int)metadataSadica->tamanio_bloques);

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
	void* buffer;
	socketKernel = accept(socketFileSystem, (struct sockaddr *) &direccionKernel, &length);

	if (socketKernel > 0) {
		//printf("%s:%d conectado\n", inet_ntoa(direccionKernel.sin_addr), ntohs(direccionKernel.sin_port));
		handShakeListen(&socketKernel, "100", "401", "499", "Kernel");
		char message[MAXBUF];

		int result = recv(socketKernel, message, sizeof(message), 0);
		puts("****************************** \n");
		puts("Este proceso no tiene consola \n");
		puts("****************************** \n");
		while (result > 0) {

			//printf("%s", message);

			char**mensajeAFileSystem = string_split(message, ";");
			int codigo = atoi(mensajeAFileSystem[0]);

			switch (codigo){
				case 804:
					enviarMensaje(&socketKernel, "todo piola");
					buffer = malloc(MAXBUF);
					recv(socketKernel, buffer, MAXBUF, 0);
					guardar_datos(mensajeAFileSystem[1], atoi(mensajeAFileSystem[2]), buffer, atoi(mensajeAFileSystem[3]));
					free(buffer);
				break;

				case 803:
					crear_archivo(mensajeAFileSystem[1], mensajeAFileSystem[2]);
			    break;

				case 802:
					//puts("2 \n");
					borrarArchivo(mensajeAFileSystem[1]);
				break;

				case 800:
					enviarMensaje(&socketKernel, "todo piola");
					buffer = malloc(MAXBUF);
					recv(socketKernel, buffer, MAXBUF, 0);
					obtener_datos(mensajeAFileSystem[1], atoi(mensajeAFileSystem[2]), buffer, atoi(mensajeAFileSystem[3]));
					free(buffer);
				break;
			  }

			result = recv(socketKernel, message, sizeof(message), 0);

		    }

		if (result <= 0) {
			printf("Se desconecto el Kernel\n");
		}
		 else {
			perror("Fallo en el manejo del hilo Kernel");

		 }

   	}

	shutdown(socketKernel, 0);
	close(socketKernel);

	return EXIT_SUCCESS;
}

int validar_archivo(char* directorio, char* flag){

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
		int fd_script = open(directorio, O_RDWR);
		if(fd_script != -1)
		{
			struct stat script;
			fstat(fd_script, &script);
			char* pmap = mmap(0, script.st_size, PROT_WRITE, MAP_SHARED, fd_script, 0);

			if(string_contains(pmap, "Tamanio"))
			{
		   		 munmap(pmap,script.st_size);
			   	 close(fd_script);
			   	 //FILE * freabierto = fopen(directorio, "w");
		   		 t_archivosFileSystem* archNuevo = malloc(sizeof(t_archivosFileSystem));
		   		   archNuevo->referenciaArchivo = &fd_script;
		   		   archNuevo->path = string_new();
		   		    string_append(&archNuevo->path, directorio);

		  		  list_add(lista_archivos, archNuevo);

				return 2;
			}
			else
			{
				if(string_contains(flag, "c"))
				{
					return 1;
				}
				else
				{
					return 3;
				}

			}

		}
		if(string_contains(flag, "c"))
		{
			return 1;
		}
		else
		{
			return 3;
		}
	}
	//free(pathAbsoluto);

}
t_metadataArch* leerMetadataDeArchivoCreado(char* arch)
{
	t_metadataArch* regMetadataArch = malloc(sizeof(t_metadataArch));
	regMetadataArch->bloquesEscritos = list_create();

	int fd_script = open(arch, O_RDWR);
	struct stat script;
	fstat(fd_script, &script);
	char* pmap = mmap(0, script.st_size, PROT_WRITE, MAP_SHARED, fd_script, 0);

	char* tamanio = string_new();
	char* bloques =  string_new();
	bloques = string_duplicate(pmap);

	tamanio = strtok_r(strtok(bloques, "]"), "[",&bloques);


	strtok(tamanio, "\n");
	regMetadataArch->tamanio = atoi((string_substring(tamanio,0,17)));


	int cantBloques = substr_count(bloques, ",") + 1; //para saber cuantos bloques debo leer

	char** arrayBloques = string_split(bloques,",");

	int i = 0; //porque los registros empiezan en 0
	while( i != cantBloques)
	{

		list_add(regMetadataArch->bloquesEscritos, atoi(arrayBloques[i]));
		i++;
	}

	//printf("el valor del mmap es %s",pmap);
	 munmap(pmap,script.st_size);
	 close(fd_script);
	 return regMetadataArch;

}
void ponerVaciosAllenarEnArchivos(FILE * pFile, int cantidadEspacios)
{
	char * espacios =string_new();

	int i = 0;

	for(i=0; i < cantidadEspacios; i++)
	{
		string_append(&espacios," ");
	}

    fputs(espacios, pFile);
    fseek(pFile, string_length(espacios), 0);
}

void cerrarUnArchivoBloque(char* pmap, struct stat script)
{
	munmap(pmap,script.st_size);
}

void actualizarBitmap(int numeroBloque)
{
	char* directorio = string_new();
	directorio = string_substring(montaje,0,string_length(montaje));

	string_append(&directorio,"Metadata/Bitmap.bin");
	remove(directorio);
	FILE * bitmapArchivo = fopen(directorio, "w+");

	bitarray_set_bit(bitmap, numeroBloque);

    fputs(bitmap->bitarray, bitmapArchivo);
    fseek(bitmapArchivo, string_length(bitmap->bitarray),0);
}

void crear_archivo(char* flag, char* directorio){

	  char* directorioAux = string_new();
      directorioAux = strtok(directorio, "\n"); //porque me agrega un \n al final del archivo
      char* auxConBarra = string_substring(directorioAux,0,1);

	  if(string_equals_ignore_case(auxConBarra,"/"))
	  {
		directorioAux = string_substring(directorioAux,1,string_length(directorioAux));
	  }

	  char* pathAbsoluto = string_new();
	  string_append(&pathAbsoluto, montaje);
	  string_append(&pathAbsoluto, directorioAux);

   	  int result = validar_archivo(pathAbsoluto, flag);

   	  if (result == 1)
   	  {
			int numeroBloque = buscarPrimerBloqueLibre(); //Por defecto tengo que asignarle un bloque
			if(numeroBloque != -1)
			{
				 FILE * pFile;
				pFile = fopen (pathAbsoluto,"w+"); //por defecto lo crea

				char* tamanio = string_new();
				char* bloques = string_new();
				string_append(&tamanio, "TamanioDeArchivo=0");
				//string_append(&tamanio, string_itoa(metadataSadica->tamanio_bloques));
				string_append(&tamanio," \n");


				string_append(&bloques, "Bloques=[");
				string_append(&bloques, string_itoa(numeroBloque));
				string_append(&bloques, "]");
				string_append(&tamanio, bloques);
				int longitudAGrabar =string_length(tamanio);
				ponerVaciosAllenarEnArchivos(pFile,longitudAGrabar);

			   t_archivosFileSystem* archNuevo = malloc(sizeof(t_archivosFileSystem));
			   archNuevo->referenciaArchivo = pFile;
			   archNuevo->path = string_new();

			  // fclose(pFile);
				string_append(&archNuevo->path, pathAbsoluto);


				int archNuevoMap = open(pathAbsoluto, O_RDWR);
				struct stat scriptMap;
				fstat(archNuevoMap, &scriptMap);


				void* archMap = mmap(0,scriptMap.st_size, PROT_WRITE, MAP_SHARED, archNuevoMap, 0);


				memcpy(archMap,tamanio,longitudAGrabar);
				munmap(archMap,longitudAGrabar);
				close(archNuevoMap);

				actualizarBitmap(numeroBloque);



			  list_add(lista_archivos, archNuevo);

			 // free(tamanio);
			  //free(bloques);
				enviarMensaje(&socketKernel, "Archivo creado");
			}
			else
			{
				enviarMensaje(&socketKernel, "Error: disco lleno, no se puede crear");
			}

   	  }
   	  else if(result == 3)
   	  {
   		 enviarMensaje(&socketKernel, "Error: creacion sin permiso");
   	  }
   	  else
   	  {
   		enviarMensaje(&socketKernel, "Archivo abierto");
   	  }
   	/*  else
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
   	  //puts("sale al parecer sin problemas");
	  //free(pathAbsoluto);*/
}

t_mapeoArchivo* abrirUnArchivoBloque(int idBloque)
{

	char* pathArchivoBloque = string_new();
	string_append(&pathArchivoBloque, montaje);
	string_append(&pathArchivoBloque, "Bloques/");
	string_append(&pathArchivoBloque, string_itoa(idBloque));
	string_append(&pathArchivoBloque, ".bin");

	int fd_aleer = open(pathArchivoBloque, O_RDWR);
	struct stat scriptAleer;
	fstat(fd_aleer, &scriptAleer);

	t_mapeoArchivo* mapeo = malloc(sizeof(t_mapeoArchivo));

	mapeo->archivoMapeado = mmap(0, scriptAleer.st_size, PROT_WRITE, MAP_SHARED, fd_aleer, 0);

	mapeo->script = scriptAleer;

	//printf("el valor del mmap es %s",pmap);

	return mapeo;
}
char* concatenernaVacios(void* buffer, int size)
{

	int len = string_length(buffer);
	while( len != size)
	{
		string_append(&buffer, " ");
		len++;
	}
	//string_append(&new, buffer);
	return buffer;
}


void grabarUnArchivoBloque(t_mapeoArchivo* archBloque, int idBloque, void* buffer, int size)
{
	char* pathArchivoBloque = string_new();
	string_append(&pathArchivoBloque, montaje);
	string_append(&pathArchivoBloque, "Bloques/");
	string_append(&pathArchivoBloque, string_itoa(idBloque));
	string_append(&pathArchivoBloque, ".bin");

	//FILE* fbloque = fopen(pathArchivoBloque, "w+");

	//ponerVaciosAllenarEnArchivos(fbloque,size);

	int archNuevoMap = open(pathArchivoBloque, O_RDWR);
	struct stat scriptMap;
	fstat(archNuevoMap, &scriptMap);


	void* archMapBloque = mmap(0,scriptMap.st_size, PROT_WRITE, MAP_SHARED, archNuevoMap, 0);

	if(string_length(buffer) < size)
	{
		char * newBuffer = concatenernaVacios(buffer,size);
		memcpy(archMapBloque, newBuffer, size);
	}
	else
	{
		memcpy(archMapBloque, buffer, size);
	}

	munmap(archMapBloque, size);
	close(archNuevoMap);
	actualizarBitmap(idBloque);

}

void actualizarArchivoCreado(t_metadataArch* regArchivo, char* path)
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

		string_append(&bloques, string_itoa((int)list_get(regArchivo->bloquesEscritos, i)));
		if(regArchivo->bloquesEscritos->elements_count != (i+1))
		{
			string_append(&bloques, ",");
		}
		i++;

	}

	string_append(&bloques, "]");
	string_append(&tamanio, bloques);

	int longitudAGrabar = string_length(tamanio);

	FILE* fileActualizar = fopen(path, "w");
	ponerVaciosAllenarEnArchivos((FILE*)fileActualizar,string_length(tamanio));

	int archNuevoMap = open(path, O_RDWR);
	struct stat scriptMap;
	fstat(archNuevoMap, &scriptMap);


	void* archMap = mmap(0,scriptMap.st_size, PROT_WRITE, MAP_SHARED, archNuevoMap, 0);


	memcpy(archMap,tamanio,longitudAGrabar);
    munmap(archMap,longitudAGrabar);
    close(archNuevoMap);

//	free(tamanio);
	//free(bloques);
}

void borrarArchivo(char* directorio){
		//puts("2 \n");
		//printf("el valor del directorio es %s \n", directorio);
		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");
	   char* auxConBarra = string_substring(directorioAux,0,1);

		if(string_equals_ignore_case(auxConBarra,"/"))
		{
		  directorioAux = string_substring(directorioAux,1,string_length(directorioAux));
		}

		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);
		//puts("2 \n");

		t_metadataArch* archAborrar = leerMetadataDeArchivoCreado(pathAbsoluto);

		int cant = archAborrar->bloquesEscritos->elements_count;

		while(cant != 0)
		{

			cant--;

			int numeroBloque = list_get(archAborrar->bloquesEscritos,cant);
			char* bloque = string_new();

			bloque = string_substring(montaje,0,string_length(montaje));
			string_append(&bloque,"Bloques/");
			string_append(&bloque,string_itoa(numeroBloque));
			string_append(&bloque,".bin");
			remove(bloque);
			FILE * bitmapArchivo = fopen(bloque, "w+");
			ponerVaciosAllenarEnArchivos(bitmapArchivo,metadataSadica->tamanio_bloques);

			char* directorioBit = string_new();
			directorioBit = string_substring(montaje,0,string_length(montaje));

			string_append(&directorioBit,"Metadata/Bitmap.bin");
			remove(directorioBit);
		     bitmapArchivo = fopen(directorioBit, "w+");

			bitarray_clean_bit(bitmap, numeroBloque);

		    fputs(bitmap->bitarray, bitmapArchivo);
		    fseek(bitmapArchivo, string_length(bitmap->bitarray),0);
		}

		int result = remove((char*)pathAbsoluto);
		if(result == 0)
		{
			//puts("El archivo ha sido eliminado con exito");
		}
		else
		{
			puts("El archivo no pudo eliminarse, revise el path");

		}

		//puts("2 \n");
		//fclose((FILE*)archDestroy->referenciaArchivo);

		//list_remove_by_condition(lista_archivos, (void *) encontrar_sem);
		//free(pathAbsoluto);
		//free(directorioAux);
        //return mensaje;

    } // End if file


void* appendVoid(void* valor1, int sizeValor1, void* valorAagregar, int sizeAgregar)
{
	//reader = read(0, valorAagregar, 1);
	valor1 = realloc(valor1, sizeValor1 + sizeAgregar);
	void* aux = valor1;
	memcpy(aux + sizeValor1, valorAagregar, sizeAgregar);
	return valor1;
}

void obtener_datos(char* directorio, int size, void* buffer, int offset) {

	buffer= string_new();
	char* directorioAux = string_new();
	directorioAux = strtok(directorio, "\n");
    char* auxConBarra = string_substring(directorioAux,0,1);

	if(string_equals_ignore_case(auxConBarra,"/"))
	{
	  directorioAux = string_substring(directorioAux,1,string_length(directorioAux));
 	}
	char* pathAbsoluto = string_new();
	string_append(&pathAbsoluto, montaje);
	string_append(&pathAbsoluto, directorioAux);

		void* valorLeido = malloc(metadataSadica->tamanio_bloques);
		void* textoResult = malloc(size);
		char* textoResultChar = string_new();
		int tamanioDisco = metadataSadica->cantidad_bloques * metadataSadica->tamanio_bloques;
		if (size <= tamanioDisco)
		{
			t_metadataArch* regMetaArchBuscado =leerMetadataDeArchivoCreado(pathAbsoluto);

			if(regMetaArchBuscado->bloquesEscritos->elements_count != 0)
			{
				int bloquePosicion = offset / ((int)metadataSadica->tamanio_bloques); //numero de bloque donde comienzo a leer

							int idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion); //bloque donde comienza lo que quiero leer

							t_mapeoArchivo* archBloqueAleer = malloc(sizeof(t_mapeoArchivo));
							archBloqueAleer= abrirUnArchivoBloque(idbloqueALeer);

							if(size > metadataSadica->tamanio_bloques) //el tamaño de lo que quiero leer es mayor a un bloque?
							{
								int cantidadDeBloquesALeer = size / metadataSadica->tamanio_bloques;
								if((size % metadataSadica->tamanio_bloques) != 0)
								{
									cantidadDeBloquesALeer++;
								}
								if(cantidadDeBloquesALeer > regMetaArchBuscado->bloquesEscritos->elements_count)
								{
									textoResult = "Error: size";
								}
								else
								{
									int auxSize = 0;
									while(cantidadDeBloquesALeer != 0)
									{
										if(cantidadDeBloquesALeer != 1)
										{
											valorLeido = archBloqueAleer->archivoMapeado;
											appendVoid(buffer, auxSize, valorLeido, metadataSadica->tamanio_bloques);
											cerrarUnArchivoBloque(archBloqueAleer->archivoMapeado,archBloqueAleer->script);
											idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion+1); //bloque donde comienza lo que quiero leer
											archBloqueAleer= abrirUnArchivoBloque(idbloqueALeer);
											auxSize = auxSize + metadataSadica->tamanio_bloques;
										}
										else
										{
											valorLeido = string_substring(archBloqueAleer->archivoMapeado,0,(size % metadataSadica->tamanio_bloques));
											appendVoid(buffer, auxSize, valorLeido, metadataSadica->tamanio_bloques);
										}

										cantidadDeBloquesALeer--;
									}
									appendVoid(textoResult,0, buffer, size);
									enviarMensaje(&socketKernel, "8000");
									char messageLeerOK[MAXBUF];

									int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
									if(resultLeerOk > 0)
									{
										send(socketKernel, textoResult, size, 0);
									}
								  }

							}
							else
							{
								if(string_length(archBloqueAleer->archivoMapeado) < size)
								{
									if(string_equals_ignore_case(archBloqueAleer->archivoMapeado," "))
									{
										string_append(&textoResultChar, "Error: vacio");
										enviarMensaje(&socketKernel, "9000");
										char messageLeerOK[MAXBUF];

										int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
										if(resultLeerOk > 0)
										{
											enviarMensaje(&socketKernel, textoResultChar);
										}
									}
									else
									{
										string_append(&textoResultChar, "Error: size");
										enviarMensaje(&socketKernel, "9000");
										char messageLeerOK[MAXBUF];

										int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
										if(resultLeerOk > 0)
										{
											enviarMensaje(&socketKernel, textoResultChar);
										}
									}
								}
								else
								{
									memcpy(valorLeido,archBloqueAleer->archivoMapeado,size);
									appendVoid(buffer,0, valorLeido, size);
									appendVoid(textoResult,0, buffer, size);
									enviarMensaje(&socketKernel, "8000");
									char messageLeerOK[MAXBUF];

									int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
									if(resultLeerOk > 0)
									{
										send(socketKernel, textoResult, size, 0);
									}
								}

								cerrarUnArchivoBloque(archBloqueAleer->archivoMapeado,archBloqueAleer->script);

							}
			}
			else
			{
				string_append(&textoResultChar, "Error: vacio");
				enviarMensaje(&socketKernel, "9000");
				char messageLeerOK[MAXBUF];

				int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
				if(resultLeerOk > 0)
				{
					enviarMensaje(&socketKernel, textoResultChar);
				}
			}

		}
		else
		{
			string_append(&textoResultChar, "Error: tamaño a leer supera al almacenamiento secunadario");
			enviarMensaje(&socketKernel, "9000");
			char messageLeerOK[MAXBUF];

			int resultLeerOk = recv(socketKernel, messageLeerOK, sizeof(messageLeerOK), 0);
			if(resultLeerOk > 0)
			{
				enviarMensaje(&socketKernel, textoResultChar);
			}
		}

		//string_append(&mensaje, valorLeido);

		//free(pathAbsoluto);
		//free(directorioAux);
		return ;

	} // End if file



char* pidoBloquesEnBlancoYgrabo(int offset, t_metadataArch* regMetaArchBuscado, void* buffer, int size )
{
	char exitCode = string_new();
		int cantidadDeBloquesDelOffset = (offset / metadataSadica->tamanio_bloques);
		int cantBloquesEnBlancoASaltar = 0;
		int auxoff = 0;
		int auxsize = size;
		if(offset > metadataSadica->tamanio_bloques)
		{
			if((offset % metadataSadica->tamanio_bloques)  != 0)
			{
				cantidadDeBloquesDelOffset++;
			    int cantBloquesEnBlancoASaltar =  cantidadDeBloquesDelOffset - regMetaArchBuscado->bloquesEscritos->elements_count;
			    auxoff = (offset % metadataSadica->tamanio_bloques);
			}
		}
        if(cantBloquesEnBlancoASaltar >1) // hay saltos de bloques?
	    {

		  while(cantBloquesEnBlancoASaltar != 1)  //meto bloques en blanco exceptuando el que voy a escribir
		  {

			int unBloqueEnBlanco = buscarPrimerBloqueLibre();
			if(unBloqueEnBlanco != -1)
			{
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				t_mapeoArchivo* archBloqueAGrabarEnBlanco = abrirUnArchivoBloque(unBloqueEnBlanco);
				grabarUnArchivoBloque(archBloqueAGrabarEnBlanco,unBloqueEnBlanco," ", metadataSadica->tamanio_bloques);
				list_add(regMetaArchBuscado->bloquesEscritos, unBloqueEnBlanco);
				actualizarBitmap(unBloqueEnBlanco);
			}
			else
			{
				string_append(&exitCode, "Error: almacenamiento secundario lleno");
				break;
			}
			cantBloquesEnBlancoASaltar--;
		  }
		}
        int cantidadDeBloquesApedir = 1;
        if(size > metadataSadica->tamanio_bloques)
        {
    	    cantidadDeBloquesApedir = (size) / metadataSadica->tamanio_bloques; // el "/" hace división entera no mas, sin resto
    		if(((size) % metadataSadica->tamanio_bloques) != 0) //pregunto si hay resto, o sea un cachito de bloque mas
    		{
    			  cantidadDeBloquesApedir++;
    			  auxsize = (size) % metadataSadica->tamanio_bloques;
    		}
        }
        else
        {
        	cantidadDeBloquesApedir = 1;
        }

		int desdeDondeComenizoALeer = 0;
		while(cantidadDeBloquesApedir != 0)
		{
		 int unBloque = buscarPrimerBloqueLibre();
		 if(unBloque != -1)
		 {
			t_mapeoArchivo* archBloque = abrirUnArchivoBloque(unBloque);

			if(cantidadDeBloquesApedir != 1) //porque el utlimo no lo va a grabar entero
			{
				void*  bufferaux =  malloc((metadataSadica->tamanio_bloques));
				memcpy(bufferaux, buffer + desdeDondeComenizoALeer, (metadataSadica->tamanio_bloques));
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				grabarUnArchivoBloque(archBloque, unBloque, bufferaux, metadataSadica->tamanio_bloques); //meto el cacho de buffer en todo el bloque
				list_add(regMetaArchBuscado->bloquesEscritos, unBloque);
				desdeDondeComenizoALeer = desdeDondeComenizoALeer + (metadataSadica->tamanio_bloques);
			}
			else
			{
				void* bufferaux =  malloc(auxsize);
				memcpy(bufferaux, buffer + desdeDondeComenizoALeer, (auxsize)); //lo que me falta grabar
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + auxsize + auxoff;
				list_add(regMetaArchBuscado->bloquesEscritos, unBloque);
				grabarUnArchivoBloque(archBloque, unBloque, bufferaux, auxsize);
			}

			cantidadDeBloquesApedir--;
		 }
		else
		{
			string_append(&exitCode, "Error: almacenamiento secundario lleno");
			break;
		}
      }
		return exitCode;
 }

char* grabarParteEnbloquesYparteEnNuevos(int offset, t_metadataArch* regMetaArchBuscado, void* buffer, int size )
{

	  //int offsetRelativo = offset - (regMetaArchBuscado->bloquesEscritos->elements_count * metadataSadica->tamanio_bloques); //busco la posición a grabar en ese bloque
      int sizeRelativo = (regMetaArchBuscado->bloquesEscritos->elements_count * metadataSadica->tamanio_bloques);
	  void* bufferaux = malloc(sizeRelativo);
	  memcpy(bufferaux, buffer, sizeRelativo); //grabo lo que tengo que grabar en ese bloque

	  graboEnLosBloquesQueYaTiene(offset, regMetaArchBuscado, bufferaux, sizeRelativo);

	  void* bufferNuevo =  malloc( size - sizeRelativo);
	  memcpy(bufferNuevo, buffer + sizeRelativo, size - sizeRelativo);
	 char* exitCode = pidoBloquesEnBlancoYgrabo(offset,regMetaArchBuscado,bufferNuevo,size- sizeRelativo);
	 return exitCode;
}


void graboEnLosBloquesQueYaTiene(int offset, t_metadataArch* regMetaArchBuscado, void* buffer, int size )
{
	int auxoffset = 0;
	int auxsize = size;
	int bloquePosicion = offset / ((int)metadataSadica->tamanio_bloques); //me da el bloque al que quiero escribir (sin resto)
	if(offset > ((int)metadataSadica->tamanio_bloques))
	{
		if(offset % ((int)metadataSadica->tamanio_bloques)) //pregunto si tiene resto
		{
			auxoffset = offset % ((int)metadataSadica->tamanio_bloques);
			  bloquePosicion++; //si lo tiene significa que graba en el siguiente bloque.
		}
	}
	int idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion); //bloque donde comienza lo que quiero grabar

	t_mapeoArchivo* archBloqueAGrabar= abrirUnArchivoBloque(idbloqueALeer);

	if(size < metadataSadica->tamanio_bloques)  //pregunto si todo el buffer entra en un bloque
	{
		int sizeFinal = (offset + size);
		grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, buffer, sizeFinal); //si entra, meto todo el bloque
	}
	else
	{//sino entra todo en un bloque, pregunto en cuantos bloques entra el buffer.
	    int cantidadDeBloquesALeer = size / metadataSadica->tamanio_bloques; //me da la cantidad de bloques en la que entra el buffer (sin resto)
		if((size % metadataSadica->tamanio_bloques) != 0)
		{
			cantidadDeBloquesALeer++; //si lo tienen significa que graba en un bloque mas.
			auxsize = size < metadataSadica->tamanio_bloques;
		}
		int desdeDondeComenizoALeer = 0; // para recorrer el string del buffer y cortarlo de a cachitos
		while(cantidadDeBloquesALeer != 0)
		{
			if(cantidadDeBloquesALeer != 1)
			{ //saco un cacho de buffer del tamaño de un bloque y lo grabo y pido el siguiente bloque

				void* bufferaux = malloc(desdeDondeComenizoALeer);
				memcpy(bufferaux, (buffer + desdeDondeComenizoALeer), (metadataSadica->tamanio_bloques));
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + metadataSadica->tamanio_bloques;
				grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, bufferaux, metadataSadica->tamanio_bloques); //meto el cacho de buffer en todo el bloque
				idbloqueALeer = (int)list_get(regMetaArchBuscado->bloquesEscritos, bloquePosicion+1); // dame el siguiente bloque de la lista
				archBloqueAGrabar= abrirUnArchivoBloque(idbloqueALeer);
				desdeDondeComenizoALeer =  desdeDondeComenizoALeer + (metadataSadica->tamanio_bloques);
			}
			else
			{
				void* bufferaux = malloc(desdeDondeComenizoALeer);
				memcpy(bufferaux, buffer + desdeDondeComenizoALeer, (auxsize)); //lo que me falta grabar
				regMetaArchBuscado->tamanio = regMetaArchBuscado->tamanio + auxsize + auxoffset;
				grabarUnArchivoBloque(archBloqueAGrabar, idbloqueALeer, bufferaux, auxsize);
			}
			cantidadDeBloquesALeer--;
		}
	}
}



void guardar_datos(char* directorio, int size, void* buffer, int offset)
{
	char* exitCode = string_new();
		char* directorioAux = string_new();
		directorioAux = strtok(directorio, "\n");
		  char* auxConBarra = string_substring(directorioAux,0,1);

		  if(string_equals_ignore_case(auxConBarra,"/"))
		  {
			directorioAux = string_substring(directorioAux,1,string_length(directorioAux));
		  }

		char* pathAbsoluto = string_new();
		string_append(&pathAbsoluto, montaje);
		string_append(&pathAbsoluto, directorioAux);

		int encontrar_sem(t_archivosFileSystem* archivo) {
			return string_starts_with(pathAbsoluto, archivo->path);
		}

		//t_archivosFileSystem* archBuscado = list_find(lista_archivos, (void *) encontrar_sem);

		t_metadataArch* regMetaArchBuscado = leerMetadataDeArchivoCreado(pathAbsoluto);
		int posicionesParaGuardar = (int)regMetaArchBuscado->bloquesEscritos->elements_count * (int)metadataSadica->tamanio_bloques;
		int tamanioDisco = (int)metadataSadica->cantidad_bloques * (int)metadataSadica->tamanio_bloques;
		if(size > tamanioDisco)
		{
		    enviarMensaje(&socketKernel, "Error: tamaño a escribir supera al almacenamiento secunadario");
		}
		else
		{
			if(offset+size < posicionesParaGuardar) //entra todo en los arch/bloques que ya tiene?
			{
				graboEnLosBloquesQueYaTiene(offset, regMetaArchBuscado, buffer, size);
			}
			else
			{
			  if((offset <= posicionesParaGuardar) && (offset+size) > (posicionesParaGuardar)) //entra parte en los bloque que tiene, y parte tiene que pedir ?
			  {

				   exitCode = grabarParteEnbloquesYparteEnNuevos(offset, regMetaArchBuscado, buffer, size );
			  }
			  else //no entra nada, pido bloques y grabo todo en ellos
			  {
				   exitCode = pidoBloquesEnBlancoYgrabo(offset,regMetaArchBuscado,buffer,size);

			  }
			}
		   	 actualizarArchivoCreado(regMetaArchBuscado, pathAbsoluto);
		   	if(string_contains(exitCode, "Error"))
		   	{
		   		enviarMensaje(&socketKernel, "Error: tamaño a escribir supera al almacenamiento secunadario");
		   	}
		    enviarMensaje(&socketKernel, "Archivo Escrito Correctamente");
		}

	 //free(pathAbsoluto);
   	 //free(directorioAux);
 }
