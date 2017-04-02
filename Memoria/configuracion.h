#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int puerto;
	int marcos;
	int marco_size;
	int entradas_cache;
	int cache_x_proc;
	char* reemplazo_cache;
	int retardo_memoria;
} Memoria_Config;

void imprimir_configuracion(Memoria_Config* config);
Memoria_Config * leer_configuracion(char* directorio);

#endif /* CONFIGURACION_H_ */
