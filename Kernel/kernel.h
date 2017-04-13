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
	int pid;
	int program_counter;
	int tabla_archivos;
	int pos_stack;
	int* socket_cpu;
	int exit_code;
} t_pcb;

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
t_queue* crear_cola_pcb();
t_pcb* nuevo_pcb(int, int, int*, int, int*, int);
char* serializar_pcb(t_pcb* pcb);


#endif /* KERNEL_H_ */
