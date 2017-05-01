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
	t_list* indiceEtiquetas;
	t_list* indiceStack;
	int inicio_codigo;
	int tabla_archivos;
	int pos_stack;
	int* socket_cpu;
	int exit_code;
	int quantum;
} t_pcb;

typedef struct {
	int start;
	int offset;
} elementoIndiceCodigo;

void* manejo_memoria();
void* manejo_kernel();
char * solicitoInstruccion(t_pcb* pcb);
void imprimoInfoPCB(t_pcb * pcb);
t_pcb * reciboPCB(int * socketKernel);
t_pcb * deserializar_pcb(char * message);

#endif /* CPU_H_ */
