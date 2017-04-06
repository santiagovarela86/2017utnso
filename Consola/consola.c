//CODIGOS
//300 CON A KER - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE
//103 KER A CON - PID DE PROGRAMA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include "configuracion.h"
#include <pthread.h>
#include "helperFunctions.h"

typedef struct{
	int pid;
	int inicio;
	int fin;
	int duracion;
	int mensajes;
} programa;

void iniciar_programa();
void terminar_proceso();
void desconectar_consola();
void limpiar_mensajes();
void gestionar_programa(void* p);

void * handlerConsola(void * args);
void * handlerKernel(void * args);

Consola_Config* configuracion;
int proceso_a_terminar = -1;

int main(int argc , char **argv)
{

	if (argc != 2)
    {
    	printf("Error. Parametros incorrectos.\n");
    	return EXIT_FAILURE;
    }

	configuracion = leerConfiguracion(argv[1]);
    imprimirConfiguracion(configuracion);

	int socketKernel;
	struct sockaddr_in direccionKernel;

	creoSocket(&socketKernel, &direccionKernel, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

    pthread_t threadConsola;
    pthread_t threadKernel;

    creoThread(&threadConsola, handlerConsola, &socketKernel);
    creoThread(&threadKernel, handlerKernel, &socketKernel);

	pthread_join(threadConsola, NULL);
	pthread_join(threadKernel, NULL);

	//while(1){}

	free(configuracion);

    return EXIT_SUCCESS;
}

void * handlerConsola(void * args){

	int * socketKernel = (int *) args;

	int numero = 0;
	int numero_correcto = 0;
	int intentos_fallidos = 0;

	while(intentos_fallidos < 10){

		puts("");
		puts("***********************************************************");
		puts("Ingrese el numero de la acción a realizar");
		puts("1) Iniciar programa");
		puts("2) Finalizar programa");
		puts("3) Desconectar");
		puts("4) Limpar");
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
	while (1) {
	}

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;

}

void terminar_proceso(){
	return;
}

void desconectar_consola(){
	return;
}

void limpiar_mensajes(){
	return;
}

void iniciar_programa(int* socket_kernel){

	pthread_t thread_id_programa;
	puts("");
	puts("Ingrese nombre del programa");

	char directorio[1000];

	scanf("%s", directorio);

	FILE *ptr_fich1 = fopen(directorio, "r");

	int num;
	char buffer[1000 + 1];

	while(!feof(ptr_fich1)){

		num = fread(buffer,sizeof(char), 1000 + 1, ptr_fich1);

		buffer[num*sizeof(char)] = '\0';
	}

	enviarMensaje(socket_kernel, buffer);

	recv(socket_kernel, buffer, sizeof(buffer), 0);

	while(1){

	}
	char* codigo;
	codigo = strtok(buffer, ";");

	programa* program = malloc(sizeof(program));

	if(atoi(codigo) == 103){
		program->pid = atoi(strtok(NULL, ";"));
		program->duracion = 0;
		program->fin = 0;
		program->inicio = 0;
		program->mensajes = 0;
		creoThread(&thread_id_programa, gestionar_programa, (void*)program);
	}else{
		printf("El programa no puedo iniciarse\n");
		return;
	}

	return;
}

void gestionar_programa(void* p){
	programa* program = malloc(sizeof(programa));
	program = (programa*) p;


}
