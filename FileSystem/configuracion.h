#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
	uint puerto; /*PUERTO*/
	char* punto_montaje; /*PUNTO_MONTAJE*/
} FileSystem_Config;

void imprimir_config(FileSystem_Config* kernel);
FileSystem_Config* cargar_config(char* path);



#endif /* CONFIGURACION_H_ */
