#include "configuracion.h"

void imprimir_config(FileSystem_Config* config){
	printf("PUERTO: %d\n", config->puerto);
	printf("PUNTO_MONTAJE: %s\n", config->punto_montaje);
}

FileSystem_Config* cargar_config(char* path){

	FileSystem_Config* fs_config = malloc(sizeof(FileSystem_Config));

	t_config* metadata = config_create(path);

	if(path == NULL){
		exit(EXIT_FAILURE);
	}

	fs_config->puerto = config_get_int_value(metadata, "PUERTO");
	fs_config->punto_montaje = config_get_string_value(metadata, "PUNTO_MONTAJE");

	imprimir_config(fs_config);

	return fs_config;

}
