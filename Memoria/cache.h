/*
 * cache.h
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <commons/collections/list.h>

typedef struct {
	int indice;
	int pid;
	int nro_pagina;
	char* contenido_pagina;
} t_entrada_cache;


t_entrada_cache *crear_entrada_cache(int, int, int, char*);
void destruir_entrada_cache(t_entrada_cache*);
void flush_memoria_cache(t_list*);

#endif /* CACHE_H_ */
