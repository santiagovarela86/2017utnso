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

	/*

	ip_memoria = configuracion->ip_memoria;
	puerto_memoria = configuracion->puerto_memoria;
	ip_kernel = configuracion->ip_kernel;
	puerto_kernel = configuracion->puerto_kernel;

	*/

	creoThread(&thread_id_kernel, manejo_kernel, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);

	/*
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
	*/

	pthread_join(thread_id_kernel, NULL);
	pthread_join(thread_id_memoria, NULL);

	//while(1){}

	free(configuracion);

    return EXIT_SUCCESS;
}

void * manejo_kernel(void * args){
    char* codigo;
	int sock;
	struct sockaddr_in server;
	char message[1000] = "";
	char server_reply[2000] = "";

	creoSocket(&sock, &server, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion a Kernel creado correctamente\n");

	/*
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

	*/

	printf("PUERTO KERNEL: %d\n", server.sin_port);

	conectarSocket(&sock, &server);

	/*

	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Fallo el intento de conexion al servidor\n");
	    return 0;
	}
	*/

	puts("Conectado al servidor\n");

	strcpy(message, "500;");

	enviarMensaje(&sock, message);

	/*
	message[0] = '5';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

    if(send(sock , message , strlen(message) , 0) < 0)
    {
        puts("Fallo el envio al servidor");
        return EXIT_FAILURE;
    }
    */

	while((recv(sock, message, sizeof(message), 0)) > 0)
	{

		codigo = strtok(message, ";");

		if(atoi(codigo) == 102){
			printf("El Kernel acepto la conexion \n");
			printf("\n");
		}else{
			printf("Conexion rechazada \n");
			return EXIT_FAILURE;
		}

	}

	//Loop para seguir comunicado con el servidor
	while(1){}

	close(sock);
	return 0;
}

void* manejo_memoria(void * args){
	char* codigo;
	int sock;
	struct sockaddr_in server;
	char message[1000] = "";
	char server_reply[2000] = "";

	creoSocket(&sock, &server, inet_addr(configuracion->ip_memoria), configuracion->puerto_memoria);
	puts("Socket de conexion a Memoria creado correctamente\n");

	/*

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

	*/

	conectarSocket(&sock, &server);

	/*

	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)	    {
		perror("Fallo el intento de conexion al servidor\n");
		return 0;
	}

	*/

	puts("Conectado al servidor\n");
	strcpy(message, "500;");

	/*

	message[0] = '5';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

	*/

	enviarMensaje(&sock, message);

	/*
    if(send(sock , message , strlen(message) , 0) < 0)
    {
        puts("Fallo el envio al servidor");
        return EXIT_FAILURE;
    }
    */

	while((recv(sock, message, sizeof(message), 0)) > 0)
	{

		codigo = strtok(message, ";");

		if(atoi(codigo) == 202){
			printf("La Memoria acepto la conexion \n");
			printf("\n");
		}else{
			printf("Conexion rechazada \n");
			return EXIT_FAILURE;
		}

	}

	//Loop para seguir comunicado con el servidor
	while(1){}

	close(sock);
	return 0;
}
