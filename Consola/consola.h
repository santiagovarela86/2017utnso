/*
 * consola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_
#define SIZE_FECHA  11



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

typedef struct{
	pthread_mutex_t mtx_programa;
	int pid_programa;
} hilo_programa;

void iniciar_programa(int* socket_kernel);
void terminar_proceso(int* socket_kernel);
void desconectar_consola(int* socket_kernel);
void limpiar_mensajes();
void gestionar_programa(void* p);
t_queue* crear_cola_programas();
void * handlerConsola(void * args);
void * handlerKernel(void * args);
void * escuchar_Kernel(void * args);
void inicializarEstado(InfoConsola * infoConsola);
void destruirEstado(InfoConsola * infoConsola);
void * manejoPrograma(void * args);

#endif /* CONSOLA_H_ */
