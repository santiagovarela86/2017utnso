/*
 * cache.h
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <commons/collections/queue.h>

typedef struct {
	int pid;
	int nro_pagina;
	char* contenido_pagina;
} t_entrada_cache;


t_entrada_cache *crear_entrada_cache(char*, int , int);
void destruir_entrada_cache(t_entrada_cache*);
t_queue* crear_cola_cache();
void flush_cola_cache(t_queue* queue);

#endif /* CACHE_H_ */
