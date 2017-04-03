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
	uint puerto; /*PUERTO*/
	char* punto_montaje; /*PUNTO_MONTAJE*/
} FileSystem_Config;

FileSystem_Config* leerConfiguracion(char* path);
void imprimirConfiguracion(FileSystem_Config* fileSystem);

#endif /* CONFIGURACION_H_ */
