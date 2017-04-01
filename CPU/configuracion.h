#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
	char* ip_kernel; /*IP_KERNEL*/
	uint puerto_kernel; /*PUERTO_KERNEL*/
	char* ip_memoria; /*IP_MEMORIA*/
	uint puerto_memoria; /*PUERTO_MEMORIA*/
} CPU_Config;

void imprimir_config(CPU_Config* kernel);
CPU_Config* cargar_config(char* path);

#endif /* CONFIGURACION_H_ */
