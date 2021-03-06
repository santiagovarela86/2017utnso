#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helperFunctions.h"
#include <pthread.h>
#include <ctype.h>
#include <stdarg.h>
#include <commons/collections/list.h>

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto) {

	*sock = socket(AF_INET, SOCK_STREAM, 0);

	if (*sock < 0) {
		perror("Error al Crear el Socket");
		exit(errno);
	}

	memset(direccion, 0, sizeof(*direccion));

	direccion->sin_family = AF_INET;
	direccion->sin_addr.s_addr = ip;
	direccion->sin_port = htons(puerto);
}

void bindSocket(int * sock, struct sockaddr_in * direccion) {

	int resultado = bind(*sock, (struct sockaddr*)direccion,  (socklen_t) sizeof(struct sockaddr_in));

	if (resultado != 0) {
		perror("Error en hacer Bind al Socket");
		exit(errno);
	}
}

void escuchoSocket(int * sock) {

	int resultado = listen(*sock, MAXLIST);

	if (resultado != 0) {
		perror("Error al escuchar Socket");
		exit(errno);
	}
}

void conectarSocket(int * sock, struct sockaddr_in * direccion){

	int resultado = connect(* sock, (struct sockaddr *) direccion, sizeof(* direccion));

	if (resultado < 0) {
		perror("Error al conectarse al Socket");
		exit(errno);
	}
}

void enviarMensaje(int * sock, char * message){
	int resultado = send(* sock, message, strlen(message), 0);

	if (resultado < 0) {
		puts("Error al enviar mensaje");
		exit(errno);
	}
}

void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args) {
	int resultado = pthread_create(threadID, NULL, threadHandler, args);

	if (resultado < 0) {
		perror("Error al crear el Hilo");
		exit(errno);
	}
}

void handShakeListen(int * socketCliente, char * codigoEsperado, char * codigoAceptado, char * codigoRechazado, char * proceso){
	char message[MAXBUF];
	char * codigo;
	char * separador = ";";

	int result = recv(* socketCliente, message, sizeof(message), 0);

	if (result > 0) {
		codigo = strtok(message, separador);

		if(strcmp(codigo, codigoEsperado) == 0){
			printf("Se acepto la conexion del proceso %s \n", proceso);
			strcpy(message, codigoAceptado);
			strcat(message, separador);
			strcat(message, codigoAceptado);
			strcat(message, separador);
			enviarMensaje(socketCliente, message);
		}else{
			strcpy(message, codigoRechazado);
			strcat(message, separador);
			printf("Se rechazo la conexion del proceso %s \n", proceso);
			enviarMensaje(socketCliente, message);
		}
	} else {
		printf("Error al recibir datos del %s", proceso);
	}

}

void handShakeSend(int * socketServer, char * codigoEnvio, char * codigoEsperado, char * proceso){
	char message[MAXBUF];
	char * codigo;
	char * separador = ";";

	strcpy(message, codigoEnvio);
	strcat(message, separador);
	enviarMensaje(socketServer, message);

	int result = recv(* socketServer, message, sizeof(message), 0);

	if (result > 0) {
		codigo = strtok(message, separador);

		if (strcmp(codigo, codigoEsperado) == 0) {
			printf("El proceso %s acepto la conexion \n", proceso);
			printf("\n");
		} else {
			printf("El proceso %s rechazo la conexion \n", proceso);
			exit(errno);
		}
	} else {
		printf("Error al recibir datos del %s", proceso);
	}

}
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s));
}

//mensaje = serializarMensaje(cantArgumentos, arg1, arg2, ... )
char * serializarMensaje(int cant, ... ){
	va_list valist;
	va_start(valist, cant);

	int i;
	char * message = string_new();

	for (i = 0; i < cant; i++){
		int valor = va_arg(valist, int);
		string_append(&message, string_itoa(valor));
		string_append(&message, ";");
	}

	return message;
}

int substr_count (char* string,char *string2){
	int i,h,j = 1;
	for (i = 0,h = 0;i < strlen(string) && string2[h];i++)
		if (string[i] == *string2 && string[i] != string[i-strlen(string)]) j++;
	return j-1;
}

int encontrarPosicionEnListaDeBloques (int idBloque, t_list* lista_bloques)
{
int indexBloque =0;
int i = 0;

	void encontrar_bloque(int bloque) {
		if (idBloque == bloque)
			indexBloque = i;
		else
			i++;
	}
	 list_iterate(lista_bloques, (void*)encontrar_bloque);

	return indexBloque;
}
