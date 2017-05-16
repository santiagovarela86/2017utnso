/*
 * filesystem.h
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

void * hilo_conexiones_kernel(void * args);
void atender_peticiones(int socket);
int validar_archivo(char* directorio);
char* abrir_archivo(char* directorio, char permiso);

typedef struct {
	int fileDescriptor;
	char* flags;
	int global_fd;
} t_fileProceso;

typedef struct {
	int fileDescriptor;
	char* path;
	int cantidadDeAperturas;
} t_fileGlobal;


#endif /* FILESYSTEM_H_ */
