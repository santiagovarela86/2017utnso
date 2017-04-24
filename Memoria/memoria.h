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
#include "cache.h"

#include "helperFunctions.h"


#define OFFSET_VAR 4

typedef struct {
	int pid;
	char variable;
	int numero_pagina;
} t_manejo_programa;

typedef struct {
	int socket_consola;
} estructura_socket;

typedef struct {
	int nro_marco;
	int pid;
	int nro_pagina;
	int inicio;
	int offset;
} t_pagina_invertida;

typedef struct {
	int nro_marco;
	int ocupado;
	int inicio;
	int final;
} t_marco;

void* inicializar_consola(void*);
void* handler_conexion(void *socket_desc);
void inicializar_estructuras_administrativas(Memoria_Config* config);
void log_cache_in_disk(t_queue*);
void log_estructuras_memoria_in_disk(t_list*);
void log_contenido_memoria_in_disk(t_list*);
void limpiar_memoria_cache();
void * hilo_conexiones_kernel(void * args);
void * hilo_conexiones_cpu(void * args);
void * handler_conexiones_cpu(void * args);
t_pagina_invertida* crear_nueva_pagina(int pid, int marco, int pagina, int inicio, int offset);
void iniciar_programa(int pid, char* codigo, int paginas, int skt);
char* leer_codigo_programa(int pid, int inicio_bloque, int offset);
void agregar_registro_dump(t_pagina_invertida*);
char* leer_memoria(int posicion_de_la_Variable, int off);
void grabar_valor(int direccion, int valor);
void definir_variable(int posicion_donde_guardo, char identificador_variable, int pid);

void enviarScriptACPU(int * socketCliente, char ** mensajeDesdeCPU);
void inicializar_lista_marcos(Memoria_Config* config);
t_pagina_invertida* grabar_en_bloque(int pid, int cantidad_paginas, char* codigo);
t_marco* get_marco_libre(bool esDescendente);


#endif /* MEMORIA_H_ */
