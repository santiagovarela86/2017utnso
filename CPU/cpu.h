/*
 * cpu.h
 *
 *  Created on: 30/4/2017
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

//Se accede de esta manera -> indice[inicio][offset]
typedef int ** t_indice_codigo;

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

typedef struct {
	int start;
	int offset;
} elementoIndiceCodigo;

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
	int pid;
	char variable;
	int direcion;
	int nro_variable;
} variables;

void* manejo_memoria();
void* manejo_kernel();
char * solicitoInstruccion(t_pcb* pcb);
void imprimoInfoPCB(t_pcb * pcb);
t_pcb * reciboPCB(int * socketKernel);
t_pcb * deserializar_pcb(char * message);
char* serializar_pcb(t_pcb* pcb);

#endif /* CPU_H_ */
