//CODIGOS
//500 CPU A KER - HANDSHAKE
//500 CPU A MEM - HANDSHAKE
//102 KER A CPU - RESPUESTA HANDSHAKE DE CPU
//202 MEM A CPU - RESPUESTA HANDSHAKE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <pthread.h>
#include "configuracion.h"
#include "helperFunctions.h"
#include "helperParser.h"

void* manejo_memoria();
void* manejo_kernel();

CPU_Config* configuracion;

int main(int argc , char **argv){

	if (argc != 2){
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_kernel;
	pthread_t thread_id_memoria;

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	creoThread(&thread_id_kernel, manejo_kernel, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);

	pthread_join(thread_id_kernel, NULL);
	pthread_join(thread_id_memoria, NULL);

	//while(1){}

	free(configuracion);

    return EXIT_SUCCESS;
}

void* manejo_kernel(void *args) {
	int socketKernel;
	struct sockaddr_in direccionKernel;

	creoSocket(&socketKernel, &direccionKernel, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

	handShakeSend(&socketKernel, "500", "102", "Kernel");

	char message[MAXBUF];

	int result = recv(socketKernel, message, sizeof(message), 0);

	/*GSIMOIS: INICIO CODIGO DE DESERIALIZACION DEL PCB
	char** msg_kernel_pcb = string_split(message, ",");
	printf("BUFFER: %s\n", message);
	printf("PCB del proceso \n");
	printf("PID: %d\n", atoi(msg_kernel_pcb[0]));
	printf("PC: %d\n", atoi(msg_kernel_pcb[1]));
	//FIN CODIGO DE DESERIALIZACION DEL PCB
	*/

	while (result) {
		printf("%s", message);
		//esta es la llamada al parser
		//procesoLineas(message);
		result = recv(socketKernel, message, sizeof(message), 0);
	}

	if (result <= 0) {
		printf("Se desconecto el Kernel\n");
	}

	t_metadata_program* programa = metadata_desde_literal(message);

	//ESTOS PRINTF SON PARA VER QUE LEA BIEN EL SCRIPT - LUEGO HAY QUE QUITARLOS
	//printf("La cantidad de etiquetas son %d \n", programa->cantidad_de_etiquetas);
	//printf("La cantidad de funciones son %d \n", programa->cantidad_de_funciones);

	//Loop para seguir comunicado con el servidor
	while (1) {
	}

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;
}

void* manejo_memoria(void * args){
	int socketMemoria;
	struct sockaddr_in direccionMemoria;

	creoSocket(&socketMemoria, &direccionMemoria, inet_addr(configuracion->ip_memoria), configuracion->puerto_memoria);
	puts("Socket de conexion a la Memoria creado correctamente\n");

	conectarSocket(&socketMemoria, &direccionMemoria);
	puts("Conectado a la Memoria\n");

	handShakeSend(&socketMemoria, "500", "202", "Memoria");

	//Loop para seguir comunicado con el servidor
	while (1) {
	}

	shutdown(socketMemoria, 0);
	close(socketMemoria);
	return EXIT_SUCCESS;
}
