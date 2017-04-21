/*
 * kernel.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <commons/collections/queue.h>


typedef struct {
	int pagina;
	int offset;
	int size;
} t_Direccion;


typedef struct {
	t_Direccion direccion;
	char nombre_variable;
} t_Stack;

typedef struct {
	int pid;
	int program_counter;
	int tabla_archivos;
	int pos_stack;
	//t_list pos_stack;
	int* socket_cpu;
	int inicio_lectura_bloque;
	int offset;
	int exit_code;
} t_pcb;

typedef struct{
	int socket;
	int pid_asignado;
} estruct_cpu;

typedef struct{
	char* id;
	int valor;
} t_var_global;

typedef struct {
	char* id;
	int valor;
} t_semaforo;

typedef struct {
	uint32_t size;
	bool isFree;
} t_metadata_heap;

typedef struct {
	t_metadata_heap metadata;
	char* data;
} t_bloque_heap;

typedef struct {
	t_bloque_heap** bloques;
	int pid;
	int nro_pagina;
	int tamanio_disponible;
} t_pagina_heap;

void *hilo_conexiones_cpu(void* args);
void *hilo_conexiones_consola(void* args);
void *handler_conexion_consola(void * args);
void *handler_conexion_cpu(void * args);
void* manejo_memoria(void* args);
void* manejo_filesystem(void* args);
void* inicializar_consola(void* args);
void log_console_in_disk(char*);
void eliminar_pcb(t_pcb*);
void flush_cola_pcb(t_queue*);
void planificar();

t_queue* crear_cola_pcb();
t_pcb* nuevo_pcb(int, int, int*, int, int*, int);
char* serializar_pcb(t_pcb* pcb);
void inicializar_variables_globales();
void inicializar_semaforos();
void liberar_estructuras();


#endif /* KERNEL_H_ */
