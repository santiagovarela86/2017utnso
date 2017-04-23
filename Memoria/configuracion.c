#include "configuracion.h"

Memoria_Config* leerConfiguracion(char* path) {

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
		free(metadata);
		return memoria;
	}
	else{
		free(metadata);
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
	printf("MARCOS: %d\n", config->marcos);
	printf("MARCO_SIZE: %d\n", config->marco_size);
	printf("PUERTO: %d\n", config->puerto);
	printf("REEMPLAZO_CACHE: %s\n", config->reemplazo_cache);
	printf("RETARDO_MEMORIA: %d\n", config->retardo_memoria);
}

t_max_cantidad_paginas* obtenerMaximaCantidadDePaginas(Memoria_Config* config, int tamanio_estruct_administrativa){

	//el espacio fisico es cantidad marcos * tamanio del marco = 500 * 256 = 128 KB
	//la estructura administrativa tiene nro marco, pid y nro pagina = 20 bytes
	printf("SIZEOF: %d\n", tamanio_estruct_administrativa);

	//Para estructuras administrativas serian 500 elementos de 20 bytes c/u = 10000 bytes
	int sizeEstructurasAdministrativas = config->marcos * tamanio_estruct_administrativa;
	printf("SIZE ADMIN: %d\n", sizeEstructurasAdministrativas);

	//La cantidad de marcos serian 10000 / 256 bytes = 39 marcos para estructuras administrativas
	int cantMarcosEstructurasAdministrativas = sizeEstructurasAdministrativas / config->marco_size;
	printf("CANT MARCOS ESTRUCTURAS ADMINISTRATIVAS: %d\n", cantMarcosEstructurasAdministrativas);

	//Luego los marcos restantes 500 - 39 = 461 se asignaran a los procesos
	int cantMarcosProcesos = config->marcos - cantMarcosEstructurasAdministrativas;
	printf("CANT MARCOS PROCESOS: %d\n", cantMarcosProcesos);

	t_max_cantidad_paginas* tamanio_maximo = malloc(sizeof(t_max_cantidad_paginas));
	tamanio_maximo->maxima_cant_paginas_administracion = cantMarcosEstructurasAdministrativas;
	tamanio_maximo->maxima_cant_paginas_procesos = cantMarcosProcesos;

	return tamanio_maximo;
}
