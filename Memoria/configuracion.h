#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
	int puerto;
	int marcos;
	int marco_size;
	int entradas_cache;
	int cache_x_proc;
	char* reemplazo_cache;
	int retardo_memoria;
} Memoria_Config;

typedef struct {
	int maxima_cant_paginas_procesos;
	int maxima_cant_paginas_administracion;
} t_max_cantidad_paginas;

Memoria_Config* leerConfiguracion(char* path);
void imprimirConfiguracion(Memoria_Config* memoria);
bool archivo_config_erroneo(t_config* config);
t_max_cantidad_paginas* obtenerMaximaCantidadDePaginas(Memoria_Config*, int);

#endif /* CONFIGURACION_H_ */
