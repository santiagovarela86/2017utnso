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
	char nombre_funcion;
} t_Stack;

typedef struct {
	int start;
	int offset;
} elementoIndiceCodigo;

typedef struct {
	int pid;
	int program_counter;
	int cantidadPaginas;
	t_list* indiceCodigo;
	char * etiquetas;
	int etiquetas_size;
	int cantidadEtiquetas;
	t_list* indiceStack;
	int inicio_codigo;
	int tabla_archivos;
	int pos_stack;
	int* socket_cpu;
	int exit_code;
	int quantum;
	int* socket_consola;
} t_pcb;

typedef struct{
	char* nombre;
	int valor;
} t_globales;

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
	//t_bloque_heap** bloques;
	int pid;
	int nro_pagina;
	int tamanio_disponible;
} heapElement;

typedef struct {
	int fileDescriptor;
	char* flags;
	int global_fd;
} t_fileProceso;

typedef struct {
	int fileDescriptor;
	char* path;
	int cantidadDeAperturas;
} t_fileGlobal;

typedef struct {
	char* codigo;
	int skt;
}t_nuevo;

void *hilo_conexiones_cpu(void* args);
void *hilo_conexiones_consola(void* args);
void *handler_conexion_consola(void * args);
void *handler_conexion_cpu(void * args);
void* manejo_memoria(void* args);
void* manejo_filesystem(void* args);
void* inicializar_consola(void* args);
void log_console_in_disk(char*);
//void eliminar_pcb(t_pcb*);
void eliminar_pcb(void * voidPCB);
void flush_cola_pcb(t_queue*);
void * planificar();
void inicializarEstructuras(char * pathConfig);

t_queue* crear_cola_pcb();
t_pcb* nuevo_pcb(int, int *);
char* serializar_pcb(t_pcb* pcb);
void inicializar_variables_globales();
void inicializar_semaforos();
void liberarEstructuras();
void heapElementDestroyer(void * heapElement);

bool esComentario(char* linea);
bool esNewLine(char* linea);
char * limpioCodigo(char * codigo);
void cargoIndicesPCB(t_pcb * pcb, char * codigo);
t_pcb * deserializar_pcb(char * mensajeRecibido);
void * multiprogramar();

void iniciarPrograma(char * codigo, int socket, int pid);
void finalizarPrograma(int pidACerrar);
void cerrarConsola(int socketCliente);

void finDeQuantum(int * socketCliente);
void finDePrograma(int pid);
void waitSemaforo(int * socketCliente, char * semaforo_buscado);
void signalSemaforo(int * socketCliente, char * otro_semaforo_buscado);
void asignarValorCompartida(char * variable, int valor);
void obtenerValorCompartida(char * otra_variable, int * socketCliente);
void escribir(int fd, int pid_mensaje, char * info);

void envioProgramaAMemoria(t_pcb * new_pcb, t_nuevo * nue);
void rechazoFaltaMemoria(int socketConsola);
void creoPrograma(t_pcb * new_pcb, char * codigo, int inicio_codigo, int cantidadPaginas);
void informoAConsola(int socketConsola, int pid);
void reservarMemoriaHeap(t_pcb * pcb, int bytes);
t_pcb * pcbFromPid(int pid);
void asignarCantidadMaximaStackPorProceso();

#define CONST_SIN_NOMBRE_FUNCION -1

#endif /* KERNEL_H_ */
