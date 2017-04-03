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
int grado_multiprogramacion;
pthread_mutex_t mutex_grado_multiprog = PTHREAD_MUTEX_INITIALIZER;

void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args);

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos. \n");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_filesystem;
	pthread_t thread_id_memoria;
	pthread_t thread_proceso_consola;
	pthread_t thread_proceso_cpu;
	pthread_t thread_consola_kernel;

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	grado_multiprogramacion = configuracion->grado_multiprogramacion;

	creoThread(&thread_id_filesystem, manejo_filesystem, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);
	creoThread(&thread_proceso_consola, hilo_conexiones_consola, NULL);
	creoThread(&thread_proceso_cpu, hilo_conexiones_cpu, NULL);
	creoThread(&thread_consola_kernel, inicializar_consola, NULL);

	pthread_join(thread_id_filesystem, NULL);
	pthread_join(thread_id_memoria, NULL);
	pthread_join(thread_proceso_consola, NULL);
	pthread_join(thread_proceso_cpu, NULL);
	pthread_join(thread_consola_kernel, NULL);

	free(configuracion);

	return EXIT_SUCCESS;
}

void * inicializar_consola(void* args){

	while(1){
		puts("");
		puts("***********************************************************");
		puts("CONSOLA KERNEL - ACCIONES");
		puts("1)  Listado de procesos del sistema");
		puts("2)  Cantidad de rafagas ejecutadas");
		puts("3)  Cantidad de operaciones privilegiadas");
		puts("4)  Tabla de archivos abiertos");
		puts("5)  Cantidad de paginas de heap utilizadas");
		puts("6)  Cantidad de syscalls ejecutadas");
		puts("7)  Tabla global de archivos");
		puts("8)  Modificar el grado de multiprogramacion del sistema");
		puts("9)  Finalizar Proceso");
		puts("10) Detener la Planificacion");
		puts("***********************************************************");

		int accion = 0;
		int accion_correcta = 0;
		int nuevo_grado_multiprog = 0;
		int pid_buscado;
		char* mensaje = string_new();

		while (accion_correcta == 0){

			mensaje = string_new();
			scanf("%d", &accion);

			switch(accion){
			case 1:
				accion_correcta = 1;
				string_append(&mensaje, "Listado de procesos del sistema");
				log_console_in_disk(mensaje);
				break;
			case 2:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Cantidad de rafagas ejecutadas por el proceso ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 3:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Cantidad de operaciones privilegiadas ejecutadas por el proceso ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 4:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Tabla de archivos abiertos por el proceso ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 5:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Cantidad de paginas de heap utilizadas por el proceso ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 6:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Cantidad de syscalls ejecutadas por el proceso ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 7:
				accion_correcta = 1;
				string_append(&mensaje, "Tabla global de archivos");
				log_console_in_disk(mensaje);
				break;
			case 8:
				accion_correcta = 1;
				printf("Ingrese nuevo grado de multiprogramacion: ");
				scanf("%d", &nuevo_grado_multiprog);
				pthread_mutex_lock(&mutex_grado_multiprog);
				grado_multiprogramacion = nuevo_grado_multiprog;
				pthread_mutex_unlock(&mutex_grado_multiprog);
				string_append(&mensaje, "Ahora el grado de multiprogramacion es ");
				string_append(&mensaje, string_itoa(nuevo_grado_multiprog));
				log_console_in_disk(mensaje);
				break;
			case 9:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				string_append(&mensaje, "Se finalizo el proceso con PID ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 10:
				accion_correcta = 1;
				string_append(&mensaje, "Se finalizo la planificacion de los procesos");
				log_console_in_disk(mensaje);
				break;
			default:
				accion_correcta = 0;
				puts("Comando invalido. A continuacion se detallan las acciones:");
				puts("1)  Listado de procesos del sistema");
				puts("2)  Cantidad de rafagas ejecutadas");
				puts("3)  Cantidad de operaciones privilegiadas");
				puts("4)  Tabla de archivos abiertos");
				puts("5)  Cantidad de paginas de heap utilizadas");
				puts("6)  Cantidad de syscalls ejecutadas");
				puts("7)  Tabla global de archivos");
				puts("8)  Modificar el grado de multiprogramacion del sistema");
				puts("9)  Finalizar Proceso");
				puts("10) Detener la Planificacion");
				break;
			}
		}
	}
}

void log_console_in_disk(char* mensaje) {
	t_log* logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);
    log_info(logger, mensaje, "INFO");
    log_destroy(logger);
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
	}

	return EXIT_SUCCESS;
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
