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

void* manejo_memoria();
void* manejo_kernel();

char* ip_memoria;
int puerto_memoria;
char* ip_kernel;
int puerto_kernel;

int main(int argc , char **argv){

	pthread_t thread_id_kernel;
	pthread_t thread_id_memoria;

	if (argc == 1){
		printf("Error. Parametros incorrectos\n");
		return EXIT_FAILURE;
	}

	char* path = argv[1];

	CPU_Config* config = cargar_config(path);
	ip_memoria = config->ip_memoria;
	puerto_memoria = config->puerto_memoria;
	ip_kernel = config->ip_kernel;
	puerto_kernel = config->puerto_kernel;

	if(pthread_create(&thread_id_kernel, NULL, manejo_kernel, NULL) < 0)
	{
		perror("Error al crear el Hilo");
		return 1;
	}

	if(pthread_create( &thread_id_memoria, NULL, manejo_memoria, NULL) < 0)
	{
		perror("Error al crear el Hilo");
		return 1;
	}

	while(1){}

    return EXIT_SUCCESS;
}

void* manejo_kernel(){
	int sock;
	struct sockaddr_in server;
	char message[1000] = "";
	char server_reply[2000] = "";

	//Creacion de Socket
	sock = socket(AF_INET , SOCK_STREAM , 0);

	if (sock == -1){
		printf("Error. No se pudo crear el socket de conexion\n");
	    return 0;
	}

	puts("Socket de conexion a Kernel creado correctamente\n");

	server.sin_addr.s_addr = inet_addr(ip_kernel);
	server.sin_family = AF_INET;
	server.sin_port = htons(puerto_kernel);

	printf("PUERTO KERNEL: %d\n", server.sin_port);


	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Fallo el intento de conexion al servidor\n");
	    return 0;
	}

	puts("Conectado al servidor\n");

	//Loop para seguir comunicado con el servidor
	while(1){}

	close(sock);
	return 0;
}

void* manejo_memoria(){
	int sock;
	struct sockaddr_in server;
	char message[1000] = "";
	char server_reply[2000] = "";

	//Creacion de Socket
	sock = socket(AF_INET , SOCK_STREAM , 0);

	if (sock == -1){
		printf("Error. No se pudo crear el socket de conexion\n");
		return 0;
	}

	puts("Socket de conexion a Memoria creado correctamente\n");

	server.sin_addr.s_addr = inet_addr(ip_memoria);
	server.sin_family = AF_INET;
	server.sin_port = htons(puerto_memoria);

	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)	    {
		perror("Fallo el intento de conexion al servidor\n");
		return 0;
	}

	puts("Conectado al servidor\n");

	//Loop para seguir comunicado con el servidor
	while(1){}

	close(sock);
	return 0;
}
