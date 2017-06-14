/*
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
	t_list* args;
	int retPost;
	int retVar;
	t_list* variables;
	int stack_pointer;
} t_Stack;


typedef struct {
	int pid;
	char nombre_variable;
	int pagina;
	int offset;
	int size;
	int nro_variable;
	char nombre_funcion;
} t_variables;

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
	int socket_consola;
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
	int pid;
	int nro_pagina;
	t_list * alocaciones;
	int tamanio_disponible;
} admPaginaHeap;

typedef struct {
	int size;
	int direccion;
} admReservaHeap;

typedef struct {
	int pid;
	t_list* tablaProceso;
} t_lista_fileProcesos;

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
	int fd;
	int offset;
}t_offsetArch;


typedef struct {
	int pid;
	char* sem;
}t_bloqueo;

typedef struct {
	int pid;
	int cant_oper_privilegiadas;
	int cant_syscalls;
	int cant_alocar;
	int tama_alocar;
	int cant_liberar;
	int tama_liberar;
}t_estadistica;

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
void multiprogramar();

void iniciarPrograma(char * codigo, int socket, int pid);
void finalizarPrograma(int pidACerrar);
void cerrarConsola(int socketCliente);

void finDeQuantum(int * socketCliente);
void finDePrograma(int * socketCliente);
void waitSemaforo(int * socketCliente, char * semaforo_buscado);
void signalSemaforo(int * socketCliente, char * otro_semaforo_buscado);
void asignarValorCompartida(char * variable, int valor);
void obtenerValorCompartida(char * otra_variable, int * socketCliente);


void escribirArchivo( int pid_mensaje, int fd, char* infofile, int tamanio);
int abrirArchivo(int pid_mensaje, char* direccion, char* flag);
void borrarArchivo(int pid_mensaje, int fd);
void cerrarArchivo(int pid_mensaje, int fd);
char* leerArchivo( int pid_mensaje, int fd, char* infofile, int tamanio);
t_fileGlobal* traducirFDaPath(int pid_mensaje, int fd);
t_fileProceso* existeEnElementoTablaArchivo(t_list* tablaDelProceso, int fdGlobal);
t_lista_fileProcesos* existeEnListaProcesosArchivos(int pid_mensaje);
t_fileGlobal* existeEnTablaGlobalArchivos(char* direccion);
void grabarEnTablaGlobal(int cantidadAperturas, int FdGlobal, char* direccion);
void grabarEnTablaProcesos(int pid_mensaje, int fd, int globalFD, char* flag);
void grabarEnTablaProcesosUnProcesoTabla(t_list* listaProcesoDeLaTablaProesos, int fd, int globalFD, char* flag);

void envioProgramaAMemoria(t_pcb * new_pcb, t_nuevo * nue);
void rechazoFaltaMemoria(int socketConsola);
void creoPrograma(t_pcb * new_pcb, char * codigo, int inicio_codigo, int cantidadPaginas);
void informoAConsola(int socketConsola, int pid);
void reservarMemoriaHeap(t_pcb * pcb, int bytes, int * socketCPU);
void liberarMemoriaHeap(t_pcb * pcb, int direccion, int * socketCPU);
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
t_estadistica* encontrar_estadistica(int p);
void finalizarProgramaEnMemoria(int pid);
int obtener_pid_de_cpu(int* skt);
char* serializar_codigo_por_instrucciones(char* codigo);
void pedirPaginaHeapNueva(t_pcb * pcb, int bytes, int * socketCPU);
void eliminarMemoriaHeap(t_pcb * pcb, int direccion, int * socketCliente);

#define CONST_SIN_NOMBRE_FUNCION -1

#endif /* KERNEL_H_ */

