#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct{
	char* ip_kernel; /*IP_KERNEL*/
	uint puerto_kernel; /*PUERTO_KERNEL*/
	char* ip_memoria; /*IP_MEMORIA*/
	uint puerto_memoria; /*PUERTO_MEMORIA*/
} CPU_Config;

CPU_Config* leerConfiguracion(char* path);
void imprimirConfiguracion(CPU_Config* cpu);

#endif /* CONFIGURACION_H_ */
