/*
 * consola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

typedef struct{
	int pid;
	int inicio;
	int fin;
	int duracion;
	int mensajes;
	int socket_kernel;
} programa;

void iniciar_programa();
void terminar_proceso();
void desconectar_consola();
void limpiar_mensajes();
void gestionar_programa(void* p);

void * handlerConsola(void * args);
void * handlerKernel(void * args);
void * escuchar_Kernel(void * args);


#endif /* CONSOLA_H_ */
