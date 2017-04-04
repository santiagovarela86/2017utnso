/*
 * memoria.h
 *
 *  Created on: 2/4/2017
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "helperFunctions.h"

typedef struct {
	int socket_consola;
} estructura_socket;

typedef struct {
	int nro_marco;
	int pid;
	int nro_pagina;
} t_pagina_invertida;

typedef struct {
	int pid;
	int nro_pagina;
	char* contenido_pagina;
} t_entrada_cache;

void* inicializar_consola(void*);
void* handler_conexion(void *socket_desc);
void inicializar_estructuras_administrativas(Memoria_Config* config);
void log_cache_in_disk(t_list*);
void log_estructuras_memoria_in_disk(t_list*);
void log_contenido_memoria_in_disk(t_list*);
void limpiar_memoria_cache();

#endif /* MEMORIA_H_ */
