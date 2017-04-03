/*
 * configuracion.h
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
	char* ip_kernel;
	int puerto_kernel;
} Consola_Config ;

Consola_Config* leerConfiguracion(char* path);
void imprimirConfiguracion(Consola_Config* configuracion);

#endif /* CONFIGURACION_H_ */
