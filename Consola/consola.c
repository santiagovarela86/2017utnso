//CODIGOS
//300 CON A KER - HANDSHAKE
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
			terminar_proceso();
		}else if(numero == 3){
			desconectar_consola();
		}else if(numero == 4){
			limpiar_mensajes();
		}else{
			intentos_fallidos++;
			puts("Ingrese una opción de 1 a 4");
		}
	}

	if(intentos_fallidos == 10){
		return EXIT_FAILURE;
	}
}

void * handlerKernel(void * args){

	int * socketKernel = (int *) args;

	handShakeSend(socketKernel, "300", "101", "Kernel");

    pthread_t threadEscuchaKernel;
    creoThread(&threadEscuchaKernel, escuchar_Kernel, socketKernel);

	return EXIT_SUCCESS;
}

void terminar_proceso(){

	puts(" ");
	puts("Ingrese PID del proceso a terminar");

	int pid_ingresado;

	scanf("%d", pid_ingresado);

	infoConsola.proceso_a_terminar = pid_ingresado;

	return;
}

void desconectar_consola(){
	infoConsola.estado_consola = 0;

	while(infoConsola.programas_ejecutando != 0){

	}

	return;
}

void limpiar_mensajes(){
	system("clear");
	return;
}

void * escuchar_Kernel(void * args){
	char buffer[1000 + 1];

	int * socketKernel = (int *) args;
	printf("llego el skt %d \n",*socketKernel);
	while(infoConsola.estado_consola == 1){

		int result = recv(*socketKernel, buffer, sizeof(buffer), 0);
		puts("Escuchando al kernel");
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
				printf("llego el pid %d \n", program->pid);
				queue_push(cola_programas, program);

			}else if(atoi(respuesta_kernel[0]) == 104){
				printf("El programa no puedo iniciarse\n");
			}else{
				printf("Mensaje de programa: %s\n", respuesta_kernel);
			}
		} else {
			printf("Error al recibir datos del Kernel");
		}
	}

	return 0;
}

void iniciar_programa(int* socket_kernel){

	pthread_t thread_id_programa;
	puts("");
	puts("Ingrese nombre del programa");

	char directorio[1000];

	scanf("%s", directorio);

	int fd_script = open(directorio, O_RDWR);
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);
	char* pmap_script = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);

	char buffer[1000 + 1];

	enviarMensaje(socket_kernel, pmap_script);

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
	shutdown(socket, 0);
	close(socket);
}

void destruirEstado(InfoConsola * infoConsola){
	list_destroy(infoConsola->threads);
	//list_destroy_and_destroy_elements(infoConsola->threads, threadDestroyer);
	list_destroy_and_destroy_elements(infoConsola->sockets, socketDestroyer);
}
