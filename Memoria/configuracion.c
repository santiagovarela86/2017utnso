#include "configuracion.h"

Memoria_Config * leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);

	if (metadata == NULL){
		exit(errno);
	}

	bool config_erroneo = archivo_config_erroneo(metadata);

	if (config_erroneo == false){
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
	else {
		exit(errno);
	}
}

bool archivo_config_erroneo(t_config* config){
	bool archivo_erroneo = false;
	if (config_has_property(config, "CACHE_X_PROC") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "ENTRADAS_CACHE") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "MARCO_SIZE") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "MARCOS") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "PUERTO") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "REEMPLAZO_CACHE") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}
	if (config_has_property(config, "RETARDO_MEMORIA") == false){
		archivo_erroneo = true;
		return archivo_erroneo;
	}

	return archivo_erroneo;
}

void imprimirConfiguracion(Memoria_Config* config) {
	printf("CACHE_X_PROC: %d \n", config->cache_x_proc);
	printf("ENTRADAS_CACHE: %d\n", config->entradas_cache);
	printf("MARCO_SIZE: %d\n", config->marco_size);
	printf("PUERTO: %d\n", config->puerto);
	printf("REEMPLAZO_CACHE: %s\n", config->reemplazo_cache);
	printf("RETARDO_MEMORIA: %d\n", config->retardo_memoria);
}
