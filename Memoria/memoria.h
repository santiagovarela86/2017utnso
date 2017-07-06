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
#include "configuracion.h"
#include "helperFunctions.h"


#define OFFSET_VAR 4
#define ASIGNACION_MEMORIA_OK 0
#define ASIGNACION_MEMORIA_ERROR 1
#define STACK_OVERFLOW 2108

typedef struct {
	int socket_consola;
} estructura_socket;

typedef struct {
	int nro_marco;
	int pid;
	int nro_pagina;
	int offset;
} t_pagina_invertida;

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
	int pagina;
} t_pagina_proceso;

typedef struct {
	uint32_t size;
	_Bool isFree;
} heapMetadata;

void* inicializar_consola(void*);
void* handler_conexion(void *socket_desc);
void inicializar_estructuras_administrativas(Memoria_Config* config);
void log_cache_in_disk(t_list*);
void log_estructuras_memoria_in_disk();
void log_contenido_memoria_in_disk();
void log_contenido_memoria_in_disk_for_pid(int pid);
void log_contenido_memoria_cache();
void * hilo_conexiones_kernel();
void * hilo_conexiones_cpu();
void * handler_conexiones_cpu(void * args);
void iniciarPrograma(int pid, char * codigo_programa);
void agregar_registro_dump(t_pagina_invertida*);
char* leer_memoria(int posicion_de_la_Variable, int off);
void grabar_valor(int direccion, int valor);
void inicializarEstructuras(char * pathConfig);
void liberarEstructuras();
void enviarInstACPU(int * socketCliente, char ** mensajeDesdeCPU);
void inicializar_tabla_paginas(Memoria_Config* config);
t_list * grabar_en_bloque(int pid, char * codigo);
t_pagina_proceso* obtenerUltimaPaginaStack(int pid);
t_pagina_proceso* crear_nuevo_manejo_programa(int pid, int pagina);
void liberar_pagina(t_pagina_invertida* pagina);
int marco_libre_para_variables();
t_pagina_invertida *leer_pagina_de_bloque(char *base, int offset, int size);
t_pagina_invertida* list_encontrar_pag_variables(t_list* lista);
int f_hash_nene_malloc(int pid, int pagina);
t_pagina_invertida* buscar_pagina_para_insertar(int pid, int pagina);
t_pagina_invertida* buscar_pagina_para_consulta(int pid, int pagina);
void retardo_acceso_memoria(void);
void pruebas_f_hash(void);
void grabar_codigo_programa(int* j, t_pagina_invertida* pagina, char* codigo);
void * solicitar_datos_de_pagina(int pid, int pagina, int offset, int tamanio);
t_Stack* crear_entrada_stack(char variable, t_pagina_invertida* pagina);
char* serializar_entrada_indice_stack(t_Stack* indice_stack);
void enviarInstACPU(int * socketCliente, char ** mensajeDesdeCPU);
void crearPaginaHeap(int pid, int numeroDePagina, int bytes);
int obtener_inicio_pagina(t_pagina_invertida* pagina);
int obtener_offset_pagina(t_pagina_invertida* pagina);
bool pagina_llena(t_pagina_invertida* pagina);
int calcular_tamanio_proceso(int pid);
void subconsola_contenido_memoria(void);
bool almacenar_pagina_en_cache_para_pid(int pid, t_pagina_invertida* pagina);
t_entrada_cache* obtener_entrada_cache(int pid, int pagina);
void grabar_valor_en_cache(int direccion, char* valor);
t_entrada_cache* obtener_entrada_reemplazo_cache(int pid);
void reorganizar_indice_cache_y_ordenar(void);
void finalizar_programa(int pid);
void usarPaginaHeap(int pid, int paginaExistente, int direccion, int bytesPedidos);
void obtenerPosicionVariable(int pid, int pagina, int offset, int sock);
void obtenerValorDeVariable(char** mensajeDesdeCPU, int sock);
void asignarVariable(char** mensajeDesdeCPU, int sock);
void definirVariable(char nombreVariable, int pid, int pagina, int sock);
void definirPrimeraVariable(char nombreVariable, int pid, int paginaParaVariables, int sock);
void definirVariableEnPagina(char nombreVariable, t_pagina_invertida* pag_encontrada, int sock);
void definirVariableEnNuevaPagina(char nombreVariable, int pid, int pagina, int sock);
void eliminarMemoriaHeap(int pid, int pagina, int direccion);
int reordenarPaginaHeap(int indicePaginaHeap);
void reordenarMetadata(int pid, int paginaConDosBloques, int direccionMeta1, int direccionMeta2, int bytesAUnir);
char * decimalABinarioUnsigned(int valor);
void almacenarBytesEnPagina(int pid, int pagina, int offset, int size, void * buffer);
int obtener_nuevo_indice_cache();

//#define VARIABLE_EN_CACHE 99


#endif /* MEMORIA_H_ */
