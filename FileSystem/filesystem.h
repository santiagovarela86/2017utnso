/*
 * filesystem.h
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

void * hilo_conexiones_kernel(void * args);


typedef struct {
	int fileDescriptor;
	char* flags;
	int global_fd;
} t_fileProceso;

typedef struct {
	char* path;
	FILE* referenciaArchivo;
} t_archivosFileSystem;

void * handler_conexion_kernel(void * sock);

void inicializarEstructuras(char * pathConfig);
void liberarEstructuras();
void obtener_datos(char* directorio, int offset, int size);
int validar_archivo(char* directorio);
void crear_archivo(char* directorio, char* flag);
void borrarArchivo(char* directorio);
void guardar_datos(char* directorio, int offset, int size, char* buffer);

#endif /* FILESYSTEM_H_ */
