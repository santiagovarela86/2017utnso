#include "configuracion.h"

Memoria_Config * leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);
	Memoria_Config* memoria = malloc(sizeof(Memoria_Config));

	memoria->cache_x_proc = config_get_int_value(metadata, "CACHE_X_PROC");
	memoria->entradas_cache = config_get_int_value(metadata, "ENTRADAS_CACHE");
	memoria->marco_size = config_get_int_value(metadata, "MARCO_SIZE");
	memoria->marcos = config_get_int_value(metadata, "MARCOS");
	memoria->puerto = config_get_int_value(metadata, "PUERTO");
	memoria->reemplazo_cache = config_get_string_value(metadata, "REEMPLAZO_CACHE");
	memoria->retardo_memoria = config_get_int_value(metadata, "RETARDO_MEMORIA");

	return memoria;
}

void imprimirConfiguracion(Memoria_Config* config) {
	printf("CACHE_X_PROC: %d \n", config->cache_x_proc);
	printf("ENTRADAS_CACHE: %d\n", config->entradas_cache);
	printf("MARCO_SIZE: %d\n", config->marco_size);
	printf("PUERTO: %d\n", config->puerto);
	printf("REEMPLAZO_CACHE: %s\n", config->reemplazo_cache);
	printf("RETARDO_MEMORIA: %d\n", config->retardo_memoria);
}
