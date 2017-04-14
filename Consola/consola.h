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

typedef struct InfoConsola {
	int proceso_a_terminar;
	int estado_consola;
	int programas_ejecutando;
	t_list * threads;
	t_list * sockets;
} InfoConsola ;

void iniciar_programa();
void terminar_proceso();
void desconectar_consola();
void limpiar_mensajes();
void gestionar_programa(void* p);
t_queue* crear_cola_programas();
void * handlerConsola(void * args);
void * handlerKernel(void * args);
void * escuchar_Kernel(void * args);
void inicializarEstado(InfoConsola * infoConsola);
void destruirEstado(InfoConsola * infoConsola);

#endif /* CONSOLA_H_ */
