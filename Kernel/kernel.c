//CODIGOS
//100 KER A MEM - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE DE CONSOLA
//300 CON A KER - HANDSHAKE DE LA CONSOLA

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "configuracion.h"
#include "kernel.h"

Kernel* config;
int sumarizador_conexiones_cpu;
int sumarizador_conexiones_consola;

typedef struct{
	int socket_consola;
} estructura_socket_consola;


int main(int argc, char **argv)
{
    if (argc == 1)
    {
    	printf("Error. Parametros incorrectos \n");
    	return EXIT_FAILURE;
    }

    char* path = argv[1];

    config = malloc(sizeof(Kernel));
    config = cargar_config(path);

	pthread_t thread_id_filesystem;
	if(pthread_create(&thread_id_filesystem, NULL, manejo_filesystem, NULL) < 0)
	{
		perror("Error al crear el Hilo de FS");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_memoria;
	if(pthread_create(&thread_id_memoria, NULL, manejo_memoria, NULL) < 0)
	{
		perror("Error al crear el Hilo de Memoria");
		return EXIT_FAILURE;
	}

	pthread_t thread_proceso_consola;
	if(pthread_create(&thread_proceso_consola, NULL, hilo_conexiones_consola, NULL) < 0)
	{
		perror("Error al crear el Hilo de Consola");
		return EXIT_FAILURE;
	}

	pthread_t thread_proceso_cpu;
	if(pthread_create(&thread_proceso_cpu, NULL,  hilo_conexiones_cpu, NULL) < 0)
	{
		perror("Error al crear el Hilo de CPU");
		return EXIT_FAILURE;
	}

	pthread_join(thread_proceso_cpu, NULL);
	pthread_join(thread_proceso_consola, NULL);
	pthread_join(thread_id_filesystem, NULL);
	pthread_join(thread_id_memoria, NULL);

	free(config);

	return EXIT_SUCCESS;
}

void* handler_conexion_cpu(void *socket_desc){
	sumarizador_conexiones_cpu++;
	printf("Tengo %d CPU conectados \n", sumarizador_conexiones_cpu);

	while(1){}

	return EXIT_SUCCESS;
}

void* handler_conexion_consola(void *socket_desc){

	estructura_socket_consola* e_sc = malloc(sizeof(estructura_socket_consola));
	e_sc = (estructura_socket_consola*) socket_desc;
	char consola_message[1000] = "";
	char* codigo;

	sumarizador_conexiones_consola++;
	printf("Tengo %d Programas conectados \n", sumarizador_conexiones_consola);

	while((recv(e_sc->socket_consola, consola_message, sizeof(consola_message), 0)) > 0)
	{
		codigo = strtok(consola_message, ";");

		if(atoi(codigo) == 300){
			printf("Se acepto una consola \n");

			consola_message[0] = '1';
			consola_message[1] = '0';
			consola_message[2] = '1';
			consola_message[3] = ';';

		    if(send(e_sc->socket_consola , consola_message , strlen(consola_message) , 0) < 0)
		    {
		        puts("Fallo el envio al servidor");
		        return EXIT_FAILURE;
		    }
		}else{
			printf("Se rechazo una conexion incorrecta \n");
		}

	}

	while(1) {}

	return EXIT_SUCCESS;
}

void* hilo_conexiones_consola(void *args){

	int socket_desc_consola, socket_desc_cpu, client_sock_consola, client_sock_cpu;
	int c_consola, c_cpu;
	struct sockaddr_in server_consola, server_cpu, client_cpu, client_consola;

	socket_desc_consola = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc_consola == -1)
	{
		printf("Error. No se pudo crear el socket de conexion Consola");
	}

	server_consola.sin_family = AF_INET;
	server_consola.sin_addr.s_addr = INADDR_ANY;
	server_consola.sin_port = htons(config->puerto_programa);

	if(bind(socket_desc_consola,(struct sockaddr *)&server_consola, sizeof(server_consola)) < 0)
	{
		perror("Error en el bind Consola");
		return EXIT_FAILURE;
	}

	listen(socket_desc_consola, 3);

	while((client_sock_consola = accept(socket_desc_consola, (struct sockaddr *)&client_consola, (socklen_t*)&c_consola))){
		pthread_t thread_proceso_consola;

		estructura_socket_consola est_sc;
		est_sc.socket_consola = client_sock_consola;

		if(pthread_create(&thread_proceso_consola, NULL, handler_conexion_consola, (void*) &est_sc) < 0)
		{
			perror("Error al crear el Hilo Consola");
			return EXIT_FAILURE;
		}
	}

	if (client_sock_consola < 0)
	{
		perror("Fallo en la conexion Consola");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void* hilo_conexiones_cpu(void *args){

	int socket_desc_cpu, client_sock_cpu;
	int c_cpu;
	struct sockaddr_in server_cpu, client_cpu;

	socket_desc_cpu = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc_cpu == -1)
	{
		printf("Error. No se pudo crear el socket de conexion CPU");
	}

	server_cpu.sin_family = AF_INET;
	server_cpu.sin_addr.s_addr = INADDR_ANY;
	server_cpu.sin_port = htons(config->puerto_cpu);

	if(bind(socket_desc_cpu,(struct sockaddr *)&server_cpu, sizeof(server_cpu)) < 0)
	{
		perror("Error en el bind CPU");
		return EXIT_FAILURE;
	}

	listen(socket_desc_cpu, 3);

	while((client_sock_cpu = accept(socket_desc_cpu, (struct sockaddr *)&client_cpu, (socklen_t*)&c_cpu))){
		pthread_t thread_proceso_cpu;
		if(pthread_create(&thread_proceso_cpu, NULL, handler_conexion_cpu, (void*) &client_sock_cpu) < 0)
		{
			perror("Error al crear el Hilo CPU");
			return EXIT_FAILURE;
		}
	}

	if (client_sock_cpu < 0)
	{
		perror("Fallo en la conexion CPU");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}

void* manejo_memoria(void *args){

    char message[1000] = "";
    char* codigo;

	int sock;
	struct sockaddr_in server;

	char* ip_memoria = config->ip_memoria;
	int port_memoria = config->puerto_memoria;

	//Creacion de Socket
	sock = socket(AF_INET , SOCK_STREAM , 0);

	if (sock == -1){
		printf("Error. No se pudo crear el socket de conexion\n");
	    return 0;
	}

	puts("Socket de conexion a Memoria creado correctamente\n");

	server.sin_addr.s_addr = inet_addr(ip_memoria);
	server.sin_family = AF_INET;
	server.sin_port = htons(port_memoria);

	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Fallo el intento de conexion al servidor\n");
	    return 0;
	}

	puts("Conectado al servidor\n");

	message[0] = '1';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

    if(send(sock , message , strlen(message) , 0) < 0)
    {
        puts("Fallo el envio al servidor");
        return EXIT_FAILURE;
    }

	while((recv(sock, message, sizeof(message), 0)) > 0)
	{

		codigo = strtok(message, ";");

		if(atoi(codigo) == 201){
			printf("La memoria acepto la conexion \n");
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

void* manejo_filesystem(void *args){
	int sock;
	struct sockaddr_in server;

	char* ip_fs = config->ip_fs;
	int puerto_fs = config->puerto_fs;

	//Creacion de Socket
	sock = socket(AF_INET , SOCK_STREAM , 0);

	if (sock == -1){
		printf("Error. No se pudo crear el socket de conexion\n");
	    return 0;
	}

	puts("Socket de conexion a FileSystem creado correctamente\n");

	server.sin_addr.s_addr = inet_addr(ip_fs);
	server.sin_family = AF_INET;
	server.sin_port = htons(puerto_fs);

	//Conexion al Servidor
	if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Fallo el intento de conexion al servidor\n");
	    return 0;
	}

	puts("Conectado al servidor\n");

	//Loop para seguir comunicado con el servidor
	while(1){}

	close(sock);
	return EXIT_SUCCESS;
}
