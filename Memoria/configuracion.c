#include "configuracion.h"

void imprimir_configuracion(Memoria_Config* config) {
	printf("CACHE_X_PROC: %d \n", config->cache_x_proc);
	printf("ENTRADAS_CACHE: %d\n", config->entradas_cache);
	printf("MARCO_SIZE: %d\n", config->marco_size);
	printf("PUERTO: %d\n", config->puerto);
	printf("REEMPLAZO_CACHE: %s\n", config->reemplazo_cache);
	printf("RETARDO_MEMORIA: %d\n", config->retardo_memoria);
}

Memoria_Config * leer_configuracion(char* directorio){
	char* path = string_new();

    string_append(&path,directorio);

	t_config* config_memoria = config_create(path);

	Memoria_Config* config = malloc(sizeof(Memoria_Config));

	config->cache_x_proc = config_get_int_value(config_memoria, "CACHE_X_PROC");
	config->entradas_cache = config_get_int_value(config_memoria, "ENTRADAS_CACHE");
	config->marco_size = config_get_int_value(config_memoria, "MARCO_SIZE");
	config->marcos = config_get_int_value(config_memoria, "MARCOS");
	config->puerto = config_get_int_value(config_memoria, "PUERTO");
	config->reemplazo_cache = config_get_string_value(config_memoria, "REEMPLAZO_CACHE");
	config->retardo_memoria = config_get_int_value(config_memoria, "RETARDO_MEMORIA");

	return config;
}
