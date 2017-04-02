//CODIGOS
//100 KER A MEM - HANDSHAKE
//100 KER A FIL - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE DE CONSOLA
//102 KER A CPU - RESPUESTA HANDSHAKE DE CPU
//199 KER A OTR - RESPUESTA A CONEXION INCORRECTA
//300 CON A KER - HANDSHAKE DE LA CONSOLA
//401 FIL A KER - RESPUESTA HANDSHAKE DE FS
//500 CPU A KER - HANDSHAKE DEL CPU

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "configuracion.h"
#include "kernel.h"
#include "socketHelper.h"

int conexionesCPU;
int conexionesConsola;
Kernel_Config* configuracion;

void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Error. Parametros incorrectos. \n");
		return EXIT_FAILURE;
	}

	char* pathConfiguracion;
	pthread_t thread_id_filesystem;
	pthread_t thread_id_memoria;
	pthread_t thread_proceso_consola;
	pthread_t thread_proceso_cpu;

	pathConfiguracion = argv[1];
	configuracion = cargar_config(pathConfiguracion);
	imprimir_config(configuracion);

	creoThread(&thread_id_filesystem, manejo_filesystem, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);
	creoThread(&thread_proceso_consola, hilo_conexiones_consola, NULL);
	creoThread(&thread_proceso_cpu, hilo_conexiones_cpu, NULL);

	pthread_join(thread_id_filesystem, NULL);
	pthread_join(thread_id_memoria, NULL);
	pthread_join(thread_proceso_consola, NULL);
	pthread_join(thread_proceso_cpu, NULL);

	free(configuracion);

	return EXIT_SUCCESS;
}

void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args) {
	int resultado = pthread_create(threadID, NULL, threadHandler, args);

	if (resultado < 0) {
		perror("Error al crear el Hilo");
		exit(errno);
	}
}

void* manejo_filesystem(void *args) {
	char message[1000] = "";
	char* codigo;

	int socketFS;
	struct sockaddr_in direccionFS;

	creoSocket(&socketFS, &direccionFS, inet_addr(configuracion->ip_fs), configuracion->puerto_fs);

	puts("Socket de conexion a FileSystem creado correctamente\n");

	conectarSocket(&socketFS, &direccionFS);

	puts("Conectado al File System\n");

	message[0] = '1';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

	enviarMensaje(&socketFS, message);

	while ((recv(socketFS, message, sizeof(message), 0)) > 0) {

		codigo = strtok(message, ";");

		if (atoi(codigo) == 401) {
			printf("El File System acepto la conexion \n");
			printf("\n");
		} else {
			printf("El File System rechazo la conexion \n");
			return EXIT_FAILURE;
		}

	}

	//Loop para seguir comunicado con el servidor
	while (1) {
	}

	close(socketFS);
	return EXIT_SUCCESS;
}

void* manejo_memoria(void *args) {

	char message[1000] = "";
	char* codigo;

	int socketMemoria;
	struct sockaddr_in direccionMemoria;

	creoSocket(&socketMemoria, &direccionMemoria, inet_addr(configuracion->ip_memoria), configuracion->puerto_memoria);

	puts("Socket de conexion a Memoria creado correctamente\n");

	conectarSocket(&socketMemoria, &direccionMemoria);

	puts("Conectado a la Memoria\n");

	message[0] = '1';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

	enviarMensaje(&socketMemoria, message);

	while ((recv(socketMemoria, message, sizeof(message), 0)) > 0) {

		codigo = strtok(message, ";");

		if (atoi(codigo) == 201) {
			printf("La Memoria acepto la conexion \n");
			printf("\n");
		} else {
			printf("La Memoria rechazo la conexion \n");
			return EXIT_FAILURE;
		}

	}

	//Loop para seguir comunicado con el servidor
	while (1) {
	}

	close(socketMemoria);
	return EXIT_SUCCESS;
}

void* hilo_conexiones_consola(void *args) {

	int socketKernelConsola;
	struct sockaddr_in direccionKernel;
	int socketClienteConsola;
	struct sockaddr_in direccionConsola;
	int length = 0;

	creoSocket(&socketKernelConsola, &direccionKernel, INADDR_ANY, configuracion->puerto_programa);
	bindSocket(&socketKernelConsola, &direccionKernel);
	listen(socketKernelConsola, 3);

	while ((socketClienteConsola = accept(socketKernelConsola, (struct sockaddr *) &direccionConsola, (socklen_t*) &length))) {
		pthread_t thread_proceso_consola;

		creoThread(&thread_proceso_consola, handler_conexion_consola, (void *) socketClienteConsola);

		if (socketClienteConsola < 0) {
			perror("Fallo en el manejo del hilo Consola");
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}
}

void * handler_conexion_consola(void * sock) {

	char consola_message[1000] = "";
	char* codigo;

	while ((recv((int) sock, consola_message, sizeof(consola_message), 0)) > 0) {
		codigo = strtok(consola_message, ";");

		if (atoi(codigo) == 300) {
			printf("Se acepto una consola \n");
			conexionesConsola++;
			printf("Tengo %d Consolas conectadas \n", conexionesConsola);

			consola_message[0] = '1';
			consola_message[1] = '0';
			consola_message[2] = '1';
			consola_message[3] = ';';

			if (send((int) sock, consola_message, strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		} else {
			printf("Se rechazo una conexion incorrecta \n");

			consola_message[0] = '1';
			consola_message[1] = '9';
			consola_message[2] = '9';
			consola_message[3] = ';';

			if (send((int) sock, consola_message, strlen(consola_message), 0) < 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		}

	}

	while (1) {}

	return EXIT_SUCCESS;
}

void* hilo_conexiones_cpu(void *args) {

	int socketKernelCPU;
	struct sockaddr_in direccionKernel;
	int socketClienteCPU;
	struct sockaddr_in direccionCPU;
	int length = 0;

	creoSocket(&socketKernelCPU, &direccionKernel, INADDR_ANY, configuracion->puerto_cpu);
	bindSocket(&socketKernelCPU, &direccionKernel);
	listen(socketKernelCPU, 3);

	int creado_correctamente = 0;

	while (creado_correctamente == 0) {
		socketClienteCPU = accept(socketKernelCPU, (struct sockaddr *) &direccionCPU, (socklen_t*) &length);

		if (socketClienteCPU != -1) {
			creado_correctamente = 1;
		}
	}

	while (socketClienteCPU) {
		pthread_t thread_proceso_cpu;

		creoThread(&thread_proceso_cpu, handler_conexion_cpu, (void *) socketClienteCPU);
	}

	if (socketClienteCPU < 0) {
		perror("Fallo en el manejo del hilo CPU");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void* handler_conexion_cpu(void * sock) {

	char cpu_message[1000] = "";
	char* codigo;

	while ((recv((int) sock, cpu_message, sizeof(cpu_message), 0)) > 0) {
		//puts("Se acepto un CPU");
		codigo = strtok(cpu_message, ";");

		if (atoi(codigo) == 500) {
			printf("Se acepto una CPU \n");
			conexionesCPU++;
			printf("Tengo %d CPU conectados \n", conexionesCPU);

			cpu_message[0] = '1';
			cpu_message[1] = '0';
			cpu_message[2] = '2';
			cpu_message[3] = ';';

			if (send((int) sock, cpu_message, strlen(cpu_message), 0)
					< 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		} else {
			printf("Se rechazo una conexion incorrecta \n");

			cpu_message[0] = '1';
			cpu_message[1] = '9';
			cpu_message[2] = '9';
			cpu_message[3] = ';';

			if (send((int) sock, cpu_message, strlen(cpu_message), 0)
					< 0) {
				puts("Fallo el envio al servidor");
				return EXIT_FAILURE;
			}
		}

	}

	while (1) {}

	return EXIT_SUCCESS;
}
