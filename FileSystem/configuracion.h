#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct{
	uint puerto; /*PUERTO*/
	char* punto_montaje; /*PUNTO_MONTAJE*/
} FileSystem_Config;

typedef struct{
	int tamanio_bloques;
	int cantidad_bloques;
	char* magic_number;
} metadata_Config;

FileSystem_Config* leerConfiguracion(char* path);
void imprimirConfiguracion(FileSystem_Config* fileSystem);

metadata_Config* leerMetaData(char* mnt);
void imprimirMetadata(metadata_Config* meta);

t_bitarray* crearBitmap(char* mnt, size_t tamanio);

#endif /* CONFIGURACION_H_ */
