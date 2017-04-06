#include "configuracion.h"

FileSystem_Config* leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);

	FileSystem_Config* fileSystem = malloc(sizeof(FileSystem_Config));

	fileSystem->puerto = config_get_int_value(metadata, "PUERTO");
	fileSystem->punto_montaje = config_get_string_value(metadata, "PUNTO_MONTAJE");

	//imprimir_config(fs_config);

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
	directorio = string_substring(mnt,1,string_length(mnt));

	string_append(&directorio,"Metadata/Metadata.bin");

	t_config* metadata = config_create(directorio);

	metadata_Config* fileSystem_metadata = malloc(sizeof(metadata_Config));

	fileSystem_metadata->tamanio_bloques = config_get_int_value(metadata, "TAMANIO_BLOQUES");
	fileSystem_metadata->cantidad_bloques = config_get_int_value(metadata, "CANTIDAD_BLOQUES");
	fileSystem_metadata->magic_number = config_get_string_value(metadata, "MAGIC_NUMBER");

	return fileSystem_metadata;
}

void imprimirMetadata(metadata_Config* meta) {
	puts("");
	puts("La Informacion del File System es:");
	printf("TAMANIO BLOQUES: %d\n", meta->tamanio_bloques);
	printf("CANTIDAD BLOQUES: %d\n", meta->cantidad_bloques);
	printf("MAGIC NUMBER: %s\n", meta->magic_number);
}

t_bitarray* crearBitmap(char* mnt, size_t tamanio_bitmap){
	char* directorio = string_new();
	directorio = string_substring(mnt,1,string_length(mnt));

	string_append(&directorio,"Metadata/Bitmap.bin");

	FILE *ptr_fich1 = fopen(directorio, "r");

	int num;
	char buffer[1000 + 1];

	while(!feof(ptr_fich1)){

		num = fread(buffer,sizeof(char), 1000 + 1, ptr_fich1);

		buffer[num*sizeof(char)] = '\0';
	}

	t_bitarray* bitmap = bitarray_create(buffer,tamanio_bitmap);

	return bitmap;
}

void imprimirBitmap(t_bitarray* bitmap){

	int j = 0, i = 0;
	int octavos = 0;

	//LA ASIGNACION CORRECTA ES LA DE BITMAP->SIZE PERO COMO ES MUY GRANDE Y EL ARCHIVO QUE CREE SOLO TIENE 64 CHARS
	//HARCODEO CON 64.
	//i = bitmap->size;
	i = 64;

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
