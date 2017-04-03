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
