#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
	uint puerto_programa; /*PUERTO_PROG*/
	uint puerto_cpu; /*PUERTO_CPU*/
	char* ip_memoria; /*IP_MEMORIA*/
	uint puerto_memoria; /*PUERTO_MEMORIA*/
	char* ip_fs; /*IP_FS*/
	uint puerto_fs; /*PUERTO_FS*/
	int quantum; /*QUANTUM*/
	int quantum_sleep; /*QUANTUM_SLEEP*/
	char* algoritmo; /*ALGORITMO*/
	int grado_multiprogramacion; /*GRADO_MULTIPROG*/
	char** sem_ids; /*SEM_IDS*/
	char** sem_init; /*SEM_INIT*/
	char** shared_vars; /*SHARED_VARS*/
	int stack_size; /*STACK_SIZE*/
} Kernel;

void imprimir_config(Kernel* kernel);
Kernel* cargar_config(char* path);

#endif /* CONFIGURACION_H_ */
