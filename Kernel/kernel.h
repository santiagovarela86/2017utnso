/*
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
	int fdGlobal;
	char* path;
	int cantidadDeAperturas;
} t_fileGlobal;

typedef struct {
	char* codigo;
	int skt;
}t_nuevo;

typedef struct {
	int pid;
	char* sem;
}t_bloqueo;

enum exit_codes {
	FIN_OK = 0,
	FIN_ERROR_RESERVA_RECURSOS = -1,
	FIN_ERROR_ACCESO_ARCHIVO_INEXISTENTE = -2,
	FIN_ERROR_LEER_ARCHIVO_SIN_PERMISOS = -3,
	FIN_ERROR_ESCRIBIR_ARCHIVO_SIN_PERMISOS = -4,
	FIN_ERROR_EXCEPCION_MEMORIA = -5,
	FIN_POR_DESCONEXION_CONSOLA = -6,
	FIN_POR_CONSOLA = -7,
	FIN_ERROR_RESERVA_MEMORIA_MAYOR_A_PAGINA = -8,
	FIN_ERROR_SUPERO_MAXIMO_PAGINAS = -9,
	FIN_ERROR_SIN_DEFINICION = -20
};

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
void finDePrograma(int * socketCliente);
void waitSemaforo(int * socketCliente, char * semaforo_buscado);
void signalSemaforo(int * socketCliente, char * otro_semaforo_buscado);
void asignarValorCompartida(char * variable, int valor);
void obtenerValorCompartida(char * otra_variable, int * socketCliente);


void escribirArchivo(int fd, int pid_mensaje, char * info, int tamanio);
void abrirArchivo(int pid_mensaje, char* direccion, char* flag);
void borrarArchivo(int pid_mensaje, char* direccion);
void cerrarArchivo(int pid_mensaje, int fd);
char* leerArchivo( int pid_mensaje, int fd, char* infofile, int tamanio);

void envioProgramaAMemoria(t_pcb * new_pcb, t_nuevo * nue);
void rechazoFaltaMemoria(int socketConsola);
void creoPrograma(t_pcb * new_pcb, char * codigo, int inicio_codigo, int cantidadPaginas);
void informoAConsola(int socketConsola, int pid);
void reservarMemoriaHeap(t_pcb * pcb, int bytes, int socketCPU);
t_pcb * pcbFromPid(int pid);
void asignarCantidadMaximaStackPorProceso();
void abrir_subconsola_procesos();
void listarCola(t_queue * cola);
void listar_terminados();
void listar_listos();
void listar_bloqueados();
void listar_ejecucion();
void bloqueoDePrograma(int pid_a_buscar);
void matarProceso(int pidAMatar);
void abrir_subconsola_dos(t_pcb* p);
t_pcb* existe_proceso(int pid);

#define CONST_SIN_NOMBRE_FUNCION -1

#endif /* KERNEL_H_ */
