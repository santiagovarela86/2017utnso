#include "configuracion.h"
#include <sys/mman.h>
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
FileSystem_Config* leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);

	FileSystem_Config* fileSystem = malloc(sizeof(FileSystem_Config));

	fileSystem->puerto = config_get_int_value(metadata, "PUERTO");
	fileSystem->punto_montaje = config_get_string_value(metadata, "PUNTO_MONTAJE");

	//imprimir_config(fs_config);

	free(metadata);

	return fileSystem;
}

void imprimirConfiguracion(FileSystem_Config* config) {
	printf("PUERTO: %d\n", config->puerto);
	printf("PUNTO_MONTAJE: %s\n", config->punto_montaje);
}


metadata_Config* leerMetaData(char* mnt){
	if (mnt == NULL) {
		exit(errno);
	}

	char* directorio = string_new();
	directorio = string_substring(mnt,0,string_length(mnt));

	string_append(&directorio,"Metadata/Metadata.bin");

	printf("DIR: %s\n", directorio);

	t_config* metadata = config_create(directorio);

	metadata_Config* fileSystem_metadata = malloc(sizeof(metadata_Config));
	fileSystem_metadata->tamanio_bloques = config_get_int_value(metadata, "TAMANIO_BLOQUES");
	fileSystem_metadata->cantidad_bloques = config_get_int_value(metadata, "CANTIDAD_BLOQUES");
	fileSystem_metadata->magic_number = config_get_string_value(metadata, "MAGIC_NUMBER");

	free(directorio);
	free(metadata);
	return fileSystem_metadata;

	/*char* pathAbsolutoMetadata = string_new();
	string_append(&pathAbsolutoMetadata, montaje);
	string_append(&pathAbsolutoMetadata, "Metadata/Metadata.bin");

	metadataSadica = fopen(pathAbsolutoMetadata, "w");
	char* tamanioBloques = string_new();
	char* cantidadBloques = string_new();
	char* magicNumber = string_new();
	string_append(&tamanioBloques, "TamañoDeBloques=64");
	string_append(&cantidadBloques, "CantidadDeBloques=30");
	string_append(&magicNumber, "MagicNumber=SADICA");

	fseek(metadataSadica, 0, SEEK_SET);
    fputs(tamanioBloques, metadataSadica);
	fseek(metadataSadica, string_length(tamanioBloques), SEEK_SET);
    fputs(cantidadBloques, metadataSadica);
	fseek(metadataSadica, string_length(tamanioBloques)+string_length(magicNumber), SEEK_SET);
    fputs(magicNumber, metadataSadica);*/
}

void imprimirMetadata(metadata_Config* meta) {
	puts("");
	puts("La Informacion del File System es:");
	printf("TAMANIO BLOQUES: %d\n", meta->tamanio_bloques);
	printf("CANTIDAD BLOQUES: %d\n", meta->cantidad_bloques);
	printf("MAGIC NUMBER: %s\n", meta->magic_number);
}

t_bitarray* crearBitmap(char* mnt, size_t tamanio_bitmap){
	/*char* directorio = string_new();
	directorio = string_substring(mnt,1,string_length(mnt));

	string_append(&directorio,"Metadata/Bitmap.bin");

	FILE *ptr_fich1 = fopen(directorio, "r");

	int num;
	char buffer[1000 + 1];

	while(!feof(ptr_fich1)){

		num = fread(buffer,sizeof(char), 1000 + 1, ptr_fich1);

		buffer[num*sizeof(char)] = '\0';
	}*/
	char* buffer = string_new();

	char* directorio = string_new();
	directorio = string_substring(mnt,0,string_length(mnt));

	string_append(&directorio,"Metadata/Bitmap.bin");
	FILE * bitmapArchivo = fopen(directorio, "w");


	t_bitarray* bitmap = bitarray_create_with_mode(buffer, tamanio_bitmap, LSB_FIRST);

    fputs((char*)bitmap->bitarray, bitmapArchivo);

    //fseek((FILE*)bitmapArchivo, string_length((char*)bitmap->bitarray), 0);

	fwrite(bitmap, sizeof(t_bitarray), 1, bitmapArchivo);
	//lfseek(bitmapArchivo,tamanio_bitmap, SEEK_SET);

	/*int fd_script = open(pathAbsolutoBitmap, O_RDWR);
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);
	t_bitarray* bitmapeo = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);*/
	//fclose(ptr_fich1);
	free(directorio);

	return bitmap;
}
void crearBloques(char* mnt, int cantidad)
{
	int i = 1;
	while(i != cantidad+1)
	{
		char* bloque = string_new();
		bloque = string_substring(mnt,0,string_length(mnt));
		string_append(&bloque,"Bloques/");
		string_append(&bloque,string_itoa(i));
		string_append(&bloque,".bin");
		FILE * bitmapArchivo = fopen(bloque, "w");
	    fputs(" ", bitmapArchivo);
	    fseek(bitmapArchivo, string_length(""), 0);
		free(bloque);
		i++;
	}
}


void imprimirBitmap(t_bitarray* bitmap){

	int j = 0, i = 0;
	int octavos = 0;

	i = bitmap->size;

	puts(" ");
	puts("EL BITMAP ES:");

	while(j <= i){
		octavos++;
		printf("%c", bitmap->bitarray[j]);
		j++;
		if(octavos == 16){
			printf("\n");
			octavos = 0;
		}
	}
}
