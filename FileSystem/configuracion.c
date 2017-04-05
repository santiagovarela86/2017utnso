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

	string_append(&directorio,"Metadata/Metadata");

	printf("el directorio es: %s\n", directorio);

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
