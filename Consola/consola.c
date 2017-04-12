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
#include "configuracion.h"
#include <pthread.h>
#include "helperFunctions.h"l
#include "consola.h"

Consola_Config* configuracion;

/*
int proceso_a_terminar = -1;
int estado_consola = 1;
int programas_ejecutando = 0;
*/

typedef struct InfoConsola {
	int proceso_a_terminar;
	int estado_consola;
	int programas_ejecutando;
	t_list * threads;
	t_list * sockets;
} InfoConsola ;

void inicializarEstado(InfoConsola * infoConsola){
	infoConsola->proceso_a_terminar = -1;
	infoConsola->estado_consola = 1;
	infoConsola->programas_ejecutando = 0;
	infoConsola->threads = list_create();
	infoConsola->sockets = list_create();
}

/*
void threadDestroyer(void * thread){
	pthread_cancel(thread);
}
*/

void socketDestroyer(void * socket){
	shutdown(socket, 0);
	close(socket);
}

void destruirEstado(InfoConsola * infoConsola){
	list_destroy(infoConsola->threads);
	//list_destroy_and_destroy_elements(infoConsola->threads, threadDestroyer);
	list_destroy_and_destroy_elements(infoConsola->sockets, socketDestroyer);
}

InfoConsola infoConsola;

int main(int argc , char **argv)
{

	if (argc != 2)
    {
    	printf("Error. Parametros incorrectos.\n");
    	return EXIT_FAILURE;
    }

	//programas_ejecutando = 0;
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

	//while(1){}

	destruirEstado(&infoConsola);
	free(configuracion);

    return EXIT_SUCCESS;
}

void * handlerConsola(void * args){

	int * socketKernel = (int *) args;

	int numero = 0;

	int intentos_fallidos = 0;

	//while(intentos_fallidos < 10 && estado_consola == 1){
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

	//Loop para seguir comunicado con el servidor
	//while (estado_consola == 1) {
	while (infoConsola.estado_consola == 1) {
	}

	//Lo hace el destructor
	//shutdown(socketKernel, 0);
	//close(socketKernel);

	return EXIT_SUCCESS;
}

void terminar_proceso(){

	puts(" ");
	puts("Ingrese PID del proceso a terminar");

	int pid_ingresado;

	scanf("%d", pid_ingresado);

	//proceso_a_terminar = pid_ingresado;
	infoConsola.proceso_a_terminar = pid_ingresado;

	return;
}

void desconectar_consola(){
	//estado_consola = 0;
	infoConsola.estado_consola = 0;

	/* SEGUN EL TP, EL DESCONECTAR CONSOLA DEBE SER ABORTIVO
	//while(programas_ejecutando != 0){
	while(infoConsola.programas_ejecutando != 0){

	}
	*/

	return;
}

void limpiar_mensajes(){
	system("clear");
	return;
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

	int result = recv(*socket_kernel, buffer, sizeof(buffer), 0);

	if (result > 0){
		char** respuesta_kernel = string_split(buffer, ",");

		programa* program = malloc(sizeof(program));

		if(atoi(respuesta_kernel[0]) == 103){
			program->pid = atoi(respuesta_kernel[1]);
			program->duracion = 0;
			program->fin = 0;
			program->inicio = 0;
			program->mensajes = 0;
			program->socket_kernel = *socket_kernel;
			//COMENTE ESTO PARA PODER TERMINAR BIEN LA COMUNICACION ENTRE MEMORIA KERNEL CONSOLA Y CPU
			//ESTE THREAD INTERRUMPE LA CONSOLA
			//creoThread(&thread_id_programa, gestionar_programa, (void*)program);
		}else{
			printf("El programa no puedo iniciarse\n");
			return;
		}
	} else {
		printf("Error al recibir datos del Kernel");
	}

	return;
}

void gestionar_programa(void* p){
	//programas_ejecutando++;
	infoConsola.programas_ejecutando++;

	programa* program = malloc(sizeof(programa));
	program = (programa*) p;

	char buffer[1000];

	//while(program->pid != proceso_a_terminar && estado_consola == 1){
	while(program->pid != infoConsola.proceso_a_terminar && infoConsola.estado_consola == 1){

		recv(program->socket_kernel, buffer, sizeof(buffer), 0);

		printf("El proceso %d envio el mensaje: %s \n", program->pid, buffer);
	}

	puts("aca no deberia llegar");

	//programas_ejecutando = programas_ejecutando - 1;
	infoConsola.programas_ejecutando = infoConsola.programas_ejecutando - 1;
	free(program);
}
