//CODIGOS
//300 CON A KER - HANDSHAKE
//399 CON A KER - ADIOS CONSOLA
//398 CON A KER - ENVIO DE PID DEL PROGRAMA A FINALIZAR
//101 KER A CON - RESPUESTA HANDSHAKE
//103 KER A CON - PID DE PROGRAMA

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
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

t_queue* cola_programas;

Consola_Config* configuracion;

InfoConsola infoConsola;

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

	int socketKernel;
	struct sockaddr_in direccionKernel;
	list_add(infoConsola.sockets, &socketKernel);

	creoSocket(&socketKernel, &direccionKernel, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

    pthread_t threadConsola;
    pthread_t threadKernel;

    list_add(infoConsola.threads, &threadConsola);
    list_add(infoConsola.threads, &threadKernel);

    creoThread(&threadConsola, handlerConsola, &socketKernel);
    creoThread(&threadKernel, handlerKernel, &socketKernel);

	pthread_join(threadConsola, NULL);
	pthread_join(threadKernel, NULL);

	destruirEstado(&infoConsola);
	free(configuracion);

    return EXIT_SUCCESS;
}

void * handlerConsola(void * args){

	int * socketKernel = (int *) args;

	int numero = 0;

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

		scanf("%d",&numero);

		if(numero == 1){
			iniciar_programa(socketKernel);
		}else if(numero == 2){
			terminar_proceso(socketKernel);
		}else if(numero == 3){
			desconectar_consola(socketKernel);
		}else if(numero == 4){
			limpiar_mensajes();
		}else{
			intentos_fallidos++;
		}
	}

	if(intentos_fallidos == 10){
		return 0;
	}
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

	char pid_ingresado = 0;

	scanf("%s", &pid_ingresado);

	char message[MAXBUF];

	strcpy(message, "398;");
	strcat(message, &pid_ingresado);

	enviarMensaje(socket_kernel, message);

	return;
}

void desconectar_consola(int* socket_kernel){
	infoConsola.estado_consola = 0;

	char message[MAXBUF];
	strcpy(message, "399;");

	enviarMensaje(socket_kernel, message);

	exit(-1);

	return;
}

void limpiar_mensajes(){
	system("clear");
	return;
}

void * escuchar_Kernel(void * args){
	char buffer[1000 + 1];

	int * socketKernel = (int *) args;

	while(infoConsola.estado_consola == 1){

		int result = recv(*socketKernel, buffer, sizeof(buffer), 0);

		if (result > 0){
			char** respuesta_kernel = string_split(buffer, ",");

			programa* program = malloc(sizeof(program));

			if(atoi(respuesta_kernel[0]) == 103){
				program->pid = atoi(respuesta_kernel[1]);
				program->duracion = 0;
				program->fin = 0;
				program->inicio = 0;
				program->mensajes = 0;
				program->socket_kernel = *socketKernel;

				queue_push(cola_programas, program);

			}else if(atoi(respuesta_kernel[0]) == 197){
				printf("El programa no pudo iniciarse por falta de memoria\n");
			}else if(atoi(respuesta_kernel[0]) == 198){
				printf("El programa no pudo iniciarse por limite de multiprogramacion\n");
			}else{
				printf("Mensaje de programa: %s\n", *respuesta_kernel);
			}
		}
	}

	return 0;
}

void iniciar_programa(int* socket_kernel){

	puts("");
	puts("Ingrese nombre del programa");

	char directorio[1000];

	scanf("%s", directorio);

	int fd_script = open(directorio, O_RDWR);
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);
	char* pmap_script = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);

	char* respuestaConsola = string_new();
	string_append(&respuestaConsola, "303");
	string_append(&respuestaConsola, pmap_script);

	enviarMensaje(socket_kernel, respuestaConsola);

	close(fd_script);
	munmap(pmap_script,scriptFileStat.st_size);

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

}

void destruirEstado(InfoConsola * infoConsola){
	list_destroy(infoConsola->threads);
	list_destroy_and_destroy_elements(infoConsola->sockets, socketDestroyer);
}
