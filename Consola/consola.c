//CODIGOS
//300 CON A KER - HANDSHAKE
//399 CON A KER - ADIOS CONSOLA
//398 CON A KER - ENVIO DE PID DEL PROGRAMA A FINALIZAR
//101 KER A CON - RESPUESTA HANDSHAKE
//103 KER A CON - PID DE PROGRAMA
//666 KER A CON - PID DE PROGRAMA MUERTO

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include "configuracion.h"
#include <pthread.h>
#include "helperFunctions.h"
#include "consola.h"
#include <signal.h>
#include <semaphore.h>
#define BST (-3)


   struct tm *info;


t_queue* cola_programas;

Consola_Config* configuracion;

InfoConsola infoConsola;

pthread_t threadKernel;
pthread_t threadConsola;

t_list* lista_semaforos;

sem_t sem_procesamiento_mensaje;

char buffer[MAXBUF];

int socketKernel;

int main(int argc , char **argv)
{

	if (argc != 2)
    {
    	printf("Error. Parametros incorrectos.\n");
    	return EXIT_FAILURE;
    }

	cola_programas = crear_cola_programas();

	inicializarEstado(&infoConsola);

	configuracion = leerConfiguracion(argv[1]);
    imprimirConfiguracion(configuracion);

    lista_semaforos = list_create();

    sem_init(&sem_procesamiento_mensaje, 0, 0);

	struct sockaddr_in direccionKernel;
	list_add(infoConsola.sockets, &socketKernel);

	creoSocket(&socketKernel, &direccionKernel, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

    list_add(infoConsola.threads, &threadConsola);
    list_add(infoConsola.threads, &threadKernel);

    creoThread(&threadConsola, handlerConsola, &socketKernel);
    creoThread(&threadKernel, handlerKernel, &socketKernel);

	pthread_join(threadConsola, NULL);
	pthread_join(threadKernel, NULL);

	destruirEstado(&infoConsola);
	free(configuracion);
	free(cola_programas);

    return EXIT_SUCCESS;
}

void * handlerConsola(void * args){

	int * socketKernel = (int *) args;

	char* valorIngresado = string_new();

	int intentos_fallidos = 0;

	while(intentos_fallidos < 10 && infoConsola.estado_consola == 1){

		puts("");
		puts("***********************************************************");
		puts("Ingrese el numero de la acción a realizar");
		puts("1) Iniciar programa");
		puts("2) Finalizar programa");
		puts("3) Desconectar");
		puts("4) Limpiar");
		puts("***********************************************************");

		scanf("%s", valorIngresado);
		 int numero =	atoi(valorIngresado);
				switch(numero){
				case 1:
					intentos_fallidos = 0;
					iniciar_programa(socketKernel);
					break;
				case 2:
					intentos_fallidos = 0;
					terminar_proceso(socketKernel);
					break;
				case 3:
					intentos_fallidos = 0;
					desconectar_consola(socketKernel);
					break;
				case 4:
					intentos_fallidos = 0;
					limpiar_mensajes();
					break;
				default:
					intentos_fallidos++;
					break;
				}

				if(intentos_fallidos == 10){
					return 0;
				}

	}

	free(valorIngresado);
	return 0;
}

void * handlerKernel(void * args){

	int * socketKernel = (int *) args;

	handShakeSend(socketKernel, "300", "101", "Kernel");

    pthread_t threadEscuchaKernel;
    creoThread(&threadEscuchaKernel, escuchar_Kernel, socketKernel);

	pthread_join(threadEscuchaKernel, NULL);
	return EXIT_SUCCESS;
}

void terminar_proceso(int* socket_kernel){

	puts(" ");
	puts("Ingrese PID del proceso a terminar");

	char* pid_ingresado = string_new();

	scanf("%s", pid_ingresado);

	char* message = string_new();
	string_append(&message, "398");
	string_append(&message, ";");
	string_append(&message, pid_ingresado);
	string_append(&message, ";");


	enviarMensaje(socket_kernel, message);

	return;
}

void desconectar_consola(int* socket_kernel){
	infoConsola.estado_consola = 0;

	char message[MAXBUF];
	strcpy(message, "399;");

	enviarMensaje(socket_kernel, message);

	pthread_kill(threadConsola, SIGTERM);
	pthread_kill(threadKernel, SIGTERM);

	return;
}

void limpiar_mensajes(){
	system("clear");
	return;
}

void * manejoPrograma(void * args){

	int * aux = (int *) args;
	int pid_local = *aux;

	char buffer_local[MAXBUF];
	char** respuesta_kernel;

	int encontrar_pid(hilo_programa* popeye){
		return (popeye->pid_programa == pid_local);
	}

	hilo_programa* est_program = (hilo_programa* ) list_find(lista_semaforos, (void *) encontrar_pid);

	//pthread_mutex_lock(&(est_program->mtx_programa));

	while(1){

		pthread_mutex_lock(&(est_program->mtx_programa));

		int i = 0;

		while(i < MAXBUF){
			buffer_local[i] = buffer[i];
			i++;
		}

		respuesta_kernel = string_split(buffer_local, ";");

		if (atoi(respuesta_kernel[0]) == 575){
			printf("Mensaje de programa %d : %s\n", atoi(respuesta_kernel[1]), respuesta_kernel[2]);
			puts("");

			sem_post(&sem_procesamiento_mensaje);

		}else if (atoi(respuesta_kernel[0]) == 666){
			/*666 es muerte*/
			programa* p;

			int pid = atoi(respuesta_kernel[1]);
			int encontrado = 0;
			int fin = queue_size(cola_programas);

			while(fin > 0 && encontrado == 0){

				p = queue_pop(cola_programas);

				if(p->pid == pid){
					encontrado = 1;

					// sleep(3); // Sleep para probar el calculo del tiempo

					time_t tiempo = time(0);
					struct tm * morfeo = localtime(&tiempo); //TODO Aqui rompe al ejecutar un segundo programa

					p->fin = morfeo->tm_hour * 10000 + morfeo->tm_min * 100 + morfeo->tm_sec;
					p->duracion = p->fin - p->inicio;

					printf("La hora de inicio fue H: %d, M: %d, S: %d \n", (p->inicio / 10000), ((p->inicio % 10000) / 100), (p->inicio % 100));
					printf("La hora de finalizacion fue H: %d, M: %d, S: %d \n", (p->fin / 10000), ((p->fin % 10000) / 100), (p->fin % 100));
					printf("La duracion fue H: %d, M: %d, S: %d \n", (p->duracion / 10000), ((p->duracion % 10000) / 100), (p->duracion % 100));

				}

				queue_push(cola_programas, p);

				fin--;

			}


			sem_post(&sem_procesamiento_mensaje);
		}

	}

	return 0;
}

void * escuchar_Kernel(void * args){

	int * socketKernel = (int *) args;
	int pid_recibido;

	while(infoConsola.estado_consola == 1){

		int result = recv(*socketKernel, buffer, sizeof(buffer), 0);

		if (result > 0){

			char** respuesta_kernel = string_split(buffer, ";");

			if(atoi(respuesta_kernel[0]) == 103){
				pthread_t threadPrograma;
				list_add(infoConsola.threads, &threadPrograma);

				programa* program = malloc(sizeof(programa));

				printf("Se acepto el programa de pid: %d\n",  atoi(respuesta_kernel[1]));

				time_t tiempo = time(0);
				struct tm * morfeo = localtime(&tiempo);

				program->pid = atoi(respuesta_kernel[1]);
				program->inicio = morfeo->tm_hour * 10000 + morfeo->tm_min * 100 + morfeo->tm_sec;
				program->duracion = 0;
				program->mensajes = 0;
				program->socket_kernel = *socketKernel;

				queue_push(cola_programas, program);

				hilo_programa* est_program = malloc(sizeof(hilo_programa));
				est_program->pid_programa = program->pid;
				pthread_mutex_init(&(est_program->mtx_programa), NULL);
				list_add(lista_semaforos, est_program);

				creoThread(&threadPrograma, manejoPrograma, &(program->pid));

			}else if(atoi(respuesta_kernel[0]) == 197){
				printf("El programa no pudo iniciarse por falta de memoria\n");
			}else{

				pid_recibido = atoi(respuesta_kernel[1]);

				int encontrar_pid(hilo_programa* popeye){
					return (popeye->pid_programa == pid_recibido);
				}

				hilo_programa* est_program = (hilo_programa* ) list_find(lista_semaforos, (void *) encontrar_pid);

				pthread_mutex_unlock(&(est_program->mtx_programa));

				sem_wait(&sem_procesamiento_mensaje);

			}
		}
	}

	return 0;
}

void iniciar_programa(int* socket_kernel){

	puts("");
	puts("Ingrese nombre del programa");

	char directorio[MAXBUF];

	scanf("%s", directorio);

	int fd_script = open(directorio, O_RDWR);

	while (fd_script < 0) //por si ingresamos mal el nombre del archivo no cuelgue.
	{
			puts("");
			puts("No se encuentra el archivo solicitado");
			puts("");
			puts("Ingrese nombre del programa");
			puts("");
			scanf("%s", directorio);
			fd_script = open(directorio, O_RDWR);

	}
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);
	char* pmap_script = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);

	char * respuestaConsola = malloc(MAXBUF);

	string_append(&respuestaConsola, string_itoa(303));
	string_append(&respuestaConsola, ";");
	string_append(&respuestaConsola, pmap_script);
	string_append(&respuestaConsola, ";");

	enviarMensaje(socket_kernel, respuestaConsola);

	close(fd_script);
	munmap(pmap_script,scriptFileStat.st_size);

	//free(respuestaConsola);

	return;
}

t_queue* crear_cola_programas(){
	t_queue* cola = queue_create();
	return cola;
}

void eliminar_programa(programa *self){

}

void flush_cola_programas(t_queue* queue){
	 queue_destroy_and_destroy_elements(queue, (void*) eliminar_programa);
}

void inicializarEstado(InfoConsola * infoConsola){
	infoConsola->proceso_a_terminar = -1;
	infoConsola->estado_consola = 1;
	infoConsola->programas_ejecutando = 0;
	infoConsola->threads = list_create();
	infoConsola->sockets = list_create();
}

void socketDestroyer(void * socket){
	int * skt = (int *) socket;
	shutdown(*skt, 0);
	//VA ESTO?
	close(*skt);
}

void destruirEstado(InfoConsola * infoConsola){
	list_destroy(infoConsola->threads);
	list_destroy_and_destroy_elements(infoConsola->sockets, socketDestroyer);
}
