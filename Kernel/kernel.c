//CODIGOS
//100 KER A MEM - HANDSHAKE
//100 KER A FIL - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE DE CONSOLA
//102 KER A CPU - RESPUESTA HANDSHAKE DE CPU
//198 KER A CON - LIMITE GRADO MULTIPROGRAMACION
//199 KER A OTR - RESPUESTA A CONEXION INCORRECTA
//250 KER A MEM - ENVIO PROGRAMA A MEMORIA
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
//300 CON A KER - HANDSHAKE DE LA CONSOLA
//401 FIL A KER - RESPUESTA HANDSHAKE DE FS
//500 CPU A KER - HANDSHAKE DEL CPU
//600 CPU A KER - RESERVAR MEMORIA HEAP
//612 KER A MEM - ENVIO DE CANT MAXIMA DE PAGINAS DE STACK POR PROCESO
//615 CPU A KER - NO SE PUEDEN ASIGNAR MAS PAGINAS A UN PROCESO

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
#include "helperFunctions.h"
#include <parser/metadata_program.h>
#include <semaphore.h>

#define MAXCON 10
#define MAXCPU 10

int conexionesCPU = 0;
int conexionesConsola = 0;
Kernel_Config* configuracion;
int grado_multiprogramacion;
pthread_mutex_t mutex_grado_multiprog;
pthread_mutex_t mutex_numerador_pcb;
pthread_mutex_t mtx_ejecucion;
pthread_mutex_t mtx_listos;
pthread_mutex_t mtx_bloqueados;
pthread_mutex_t mtx_terminados;
pthread_mutex_t mtx_nuevos;
pthread_mutex_t mtx_cpu;
pthread_mutex_t mtx_globales;
pthread_mutex_t mtx_semaforos;
t_queue* cola_listos;
t_queue* cola_bloqueados;
t_queue* cola_ejecucion;
t_queue* cola_terminados;
t_queue* cola_nuevos;
t_queue* cola_cpu;
t_list* lista_variables_globales;
t_list* lista_semaforos;
t_list* lista_File_global;
t_list* lista_File_proceso;
t_list* lista_procesos;
t_list* registro_bloqueados;
int numerador_pcb = 1000;
int skt_memoria;
int skt_filesystem;
int plan;
t_list * heap;
sem_t semaforoMemoria;
sem_t semaforoFileSystem;

enum exit_codes {
	FIN_OK = 0,
	FIN_ERROR_RESERVA_RECURSOS = -1,
	FIN_ERROR_ACCESO_ARCHIVO_INEXISTENTE = -2,
	FIN_ERROR_LEER_ARCHIVO_SIN_PERMISOS = -3,
	FIN_ERROR_ESCRIBIR_ARCHIVO_SIN_PERMISOS = -4,
	FIN_ERROR_EXCEPCION_MEMORIA = -5,
	FIN_POR_DESCONEXION_CONSOLA = -6,
	FIN_POR_CONSOLA = -7,
	FIN_ERROR_RESERVA_MEMORIA_MAYOR_A_PAGINA = -8,
	FIN_ERROR_SUPERO_MAXIMO_PAGINAS = -9,
	FIN_ERROR_SIN_DEFINICION = -20
};

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
	pthread_t thread_planificador;
	pthread_t thread_multiprogramacion;

	char * pathConfig = argv[1];
	inicializarEstructuras(pathConfig);

	imprimirConfiguracion(configuracion);

	creoThread(&thread_id_filesystem, manejo_filesystem, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);
	creoThread(&thread_proceso_consola, hilo_conexiones_consola, NULL);
	creoThread(&thread_proceso_cpu, hilo_conexiones_cpu, NULL);
	creoThread(&thread_consola_kernel, inicializar_consola, NULL);
	creoThread(&thread_planificador, planificar, NULL);
	creoThread(&thread_multiprogramacion, multiprogramar, NULL);

	pthread_join(thread_id_filesystem, NULL);
	pthread_join(thread_id_memoria, NULL);
	pthread_join(thread_proceso_consola, NULL);
	pthread_join(thread_proceso_cpu, NULL);
	pthread_join(thread_consola_kernel, NULL);
	pthread_join(thread_planificador, NULL);

	liberarEstructuras();

	return EXIT_SUCCESS;
}

void inicializarEstructuras(char * pathConfig){
	pthread_mutex_init(&mutex_grado_multiprog, NULL);
	pthread_mutex_init(&mtx_bloqueados, NULL);
	pthread_mutex_init(&mtx_ejecucion, NULL);
	pthread_mutex_init(&mtx_listos, NULL);
	pthread_mutex_init(&mtx_nuevos, NULL);
	pthread_mutex_init(&mtx_terminados, NULL);
	pthread_mutex_init(&mtx_cpu, NULL);
	pthread_mutex_init(&mtx_globales, NULL);
	pthread_mutex_init(&mtx_semaforos, NULL);

	sem_init(&semaforoMemoria, 0, 0);
	sem_init(&semaforoFileSystem, 0, 0);

	heap = list_create();

	lista_semaforos = list_create();
	lista_variables_globales = list_create();
	lista_File_global = list_create();
	lista_File_proceso = list_create();
	lista_procesos = list_create();
	registro_bloqueados = list_create();

	cola_listos = crear_cola_pcb();
	cola_bloqueados = crear_cola_pcb();
	cola_ejecucion = crear_cola_pcb();
	cola_terminados = crear_cola_pcb();
	cola_nuevos = crear_cola_pcb();

	cola_cpu = crear_cola_pcb();

	configuracion = leerConfiguracion(pathConfig);
	if(configuracion->algoritmo[0] != 'R'){
		configuracion->quantum = 999;
	}

	grado_multiprogramacion = configuracion->grado_multiprogramacion;

	int w = 0;
	plan = 0;
	while(configuracion->sem_ids[w] != NULL){
		t_globales* sem_aux = malloc(sizeof(t_globales));
		sem_aux->nombre = string_new();
		sem_aux->nombre = configuracion->sem_ids[w];
		sem_aux->valor = atoi(configuracion->sem_init[w]);

		list_add(lista_semaforos, sem_aux);
		w++;
	}

	w = 0;

	while(configuracion->shared_vars[w] != NULL){
		t_globales* global_aux = malloc(sizeof(t_globales));
		global_aux->nombre = string_new();
		global_aux->nombre = configuracion->shared_vars[w];
		global_aux->valor = 0;

		list_add(lista_variables_globales, global_aux);
		w++;
	}

	//inicializacion de variables globales y semaforos
	inicializar_variables_globales();
	inicializar_semaforos();
}

void liberarEstructuras(){
	pthread_mutex_destroy(&mutex_grado_multiprog);
	pthread_mutex_destroy(&mtx_bloqueados);
	pthread_mutex_destroy(&mtx_ejecucion);
	pthread_mutex_destroy(&mtx_listos);
	pthread_mutex_destroy(&mtx_nuevos);
	pthread_mutex_destroy(&mtx_terminados);
	pthread_mutex_destroy(&mtx_cpu);
	pthread_mutex_destroy(&mtx_globales);
	pthread_mutex_destroy(&mtx_semaforos);

	list_destroy_and_destroy_elements(heap, heapElementDestroyer);

	queue_destroy_and_destroy_elements(cola_cpu, eliminar_pcb);
	free(configuracion);
	list_destroy_and_destroy_elements(lista_semaforos, free);
	list_destroy_and_destroy_elements(lista_variables_globales, free);

	list_destroy_and_destroy_elements(lista_File_global, free);
	list_destroy_and_destroy_elements(lista_File_proceso, free);
	list_destroy_and_destroy_elements(lista_procesos, eliminar_pcb);

	queue_destroy_and_destroy_elements(cola_listos, eliminar_pcb);
	queue_destroy_and_destroy_elements(cola_bloqueados, eliminar_pcb);
	queue_destroy_and_destroy_elements(cola_ejecucion, eliminar_pcb);
	queue_destroy_and_destroy_elements(cola_terminados, eliminar_pcb);

	sem_destroy(&semaforoMemoria);
	sem_destroy(&semaforoFileSystem);
}

void * inicializar_consola(void* args){

	while(1){
		puts("");
		puts("***********************************************************");
		puts("CONSOLA KERNEL - ACCIONES");
		puts("1)  Listado de procesos del sistema");
		puts("2)  Cantidad de rafagas ejecutadas");
		puts("3)  Cantidad de operaciones privilegiadas");
		puts("4)  Tabla de archivos abiertos por proceso");
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

		while (accion_correcta == 0){

			char * mensaje = string_new();
			scanf("%d", &accion);

			switch(accion){
			case 1:
				accion_correcta = 1;
				string_append(&mensaje, "Listado de procesos del sistema");
				log_console_in_disk(mensaje);
				abrir_subconsola_procesos();
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
				string_append(&mensaje, "821");

				string_append(&mensaje, ";");
				string_append(&mensaje, string_itoa(pid_buscado));
				string_append(&mensaje, ";");
				enviarMensaje(&skt_filesystem, mensaje);


				char message[MAXBUF];
				recv(skt_filesystem, message, sizeof(message), 0);

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

				puts("Se busca la Tabla global de archivos");

				char* tablaGlobalArchivos = string_new();

				int j= 0;

				for (j = 0; j < lista_File_global->elements_count; j++){

					t_fileGlobal * elem = malloc(sizeof(elem));
					elem = list_get(lista_File_global, j);

					string_append(&tablaGlobalArchivos, string_itoa(elem->cantidadDeAperturas));
					string_append(&tablaGlobalArchivos, ";");

					string_append(&tablaGlobalArchivos, string_itoa(elem->fdGlobal));
					string_append(&tablaGlobalArchivos, ";");

					string_append(&tablaGlobalArchivos, elem->path);
				    string_append(&tablaGlobalArchivos, ";");
				}

				log_console_in_disk(tablaGlobalArchivos);
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
				matarProceso(pid_buscado);
				string_append(&mensaje, "Se finalizo el proceso con PID ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 10:
				accion_correcta = 1;
				if(plan == 0){
					plan = 1;
					string_append(&mensaje, "Se finalizo la planificacion de los procesos");
					log_console_in_disk(mensaje);
				}else{
					plan = 0;
					string_append(&mensaje, "Se reinicio la planificacion de los procesos");
					log_console_in_disk(mensaje);
				}

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

			free(mensaje);
		}
	}
}

void abrir_subconsola_procesos(){


		puts("");
		puts("***********************************************************");
		puts("SUB CONSOLA KERNEL - LISTADOS DE PROCESOS DEL SISTEMA");
		puts("1)  Procesos en cola de Listos");
		puts("2)  Procesos en cola de Ejecucion");
		puts("3)  Procesos en cola de Bloqueados");
		puts("4)  Procesos en cola de Terminados");
		puts("***********************************************************");

		int accion = 0;
		int accion_correcta = 0;

		while (accion_correcta == 0){

			scanf("%d", &accion);

			switch(accion){
			case 1:
				accion_correcta = 1;
				listar_listos();
				break;
			case 2:
				accion_correcta = 1;
				listar_ejecucion();
				break;
			case 3:
				accion_correcta = 1;
				listar_bloqueados();
				break;
			case 4:
				accion_correcta = 1;
				listar_terminados();
				break;

			default:
				accion_correcta = 0;
				puts("Comando invalido. A continuacion se detallan las acciones:");
				puts("");
				puts("***********************************************************");
				puts("SUB CONSOLA KERNEL - LISTADOS DE PROCESOS DEL SISTEMA");
				puts("1)  Procesos en cola de Listos");
				puts("2)  Procesos en cola de Ejecucion");
				puts("3)  Procesos en cola de Bloqueados");
				puts("4)  Procesos en cola de Terminados");
				puts("5)  Procesos en cola de Nuevos");
				puts("***********************************************************");
				break;
			}



	}
}

void matarProceso(int pidAMatar){

		int tempPid;
		t_pcb * temporalP = malloc(sizeof(t_pcb));
		int largoColaListada;

		largoColaListada = queue_size(cola_listos);
		while(largoColaListada != 0){
			pthread_mutex_lock(&mtx_globales);
			temporalP = (t_pcb*) queue_pop(cola_listos);
			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){
				queue_push(cola_listos, temporalP);
			}else{
				queue_push(cola_terminados, temporalP);
			}
			pthread_mutex_unlock(&mtx_globales);
			largoColaListada--;
		}

		largoColaListada = queue_size(cola_bloqueados);
		while(largoColaListada != 0){
			pthread_mutex_lock(&mtx_globales);
			temporalP = (t_pcb*) queue_pop(cola_bloqueados);
			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){
				queue_push(cola_bloqueados, temporalP);
			}else{
				queue_push(cola_terminados, temporalP);
			}
			pthread_mutex_unlock(&mtx_globales);
			largoColaListada--;
		}

		largoColaListada = queue_size(cola_ejecucion);
		while(largoColaListada != 0){
			pthread_mutex_lock(&mtx_globales);
			temporalP = (t_pcb*) queue_pop(cola_ejecucion);
			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){
				queue_push(cola_ejecucion, temporalP);
			}else{
				queue_push(cola_terminados, temporalP);
			}
			pthread_mutex_unlock(&mtx_globales);
			largoColaListada--;
		}

		largoColaListada = queue_size(cola_nuevos);
		while(largoColaListada != 0){
			pthread_mutex_lock(&mtx_globales);
			temporalP = (t_pcb*) queue_pop(cola_nuevos);
			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){
				queue_push(cola_nuevos, temporalP);
			}else{
				queue_push(cola_terminados, temporalP);
			}
			pthread_mutex_unlock(&mtx_globales);
			largoColaListada--;
		}

}

void listarCola(t_queue * cola){

	t_pcb * temporalP = malloc(sizeof(t_pcb));
	int tempPid;
	int largoColaListada = queue_size(cola);
	while(largoColaListada != 0){
		pthread_mutex_lock(&mtx_globales);
		temporalP = (t_pcb*) queue_pop(cola);
		tempPid = temporalP->pid;
		printf("Programa %d \n", tempPid);
		puts("");
		if(tempPid > 0){
		queue_push(cola, temporalP);
		}
		pthread_mutex_unlock(&mtx_globales);
		largoColaListada--;
	}

}


void inicializar_variables_globales(){
	lista_variables_globales = list_create();
	int i = 0;
	while (configuracion->shared_vars[i] != NULL){
		t_var_global* var_global = malloc(sizeof(t_var_global));
		var_global->id = configuracion->shared_vars[i];
		var_global->valor = 0;
		list_add(lista_variables_globales, var_global);
		i++;
	}
}

void inicializar_semaforos(){
	lista_semaforos = list_create();
	int i = 0;
	while(configuracion->sem_ids[i] != NULL){
		t_semaforo* semaforo = malloc(sizeof(t_semaforo));
		semaforo->id = configuracion->sem_ids[i];
		semaforo->valor = atoi(configuracion->sem_init[i]);
		list_add(lista_semaforos, semaforo);
		i++;
	}
}

void log_console_in_disk(char* mensaje) {
	t_log* logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);
    log_info(logger, mensaje, "INFO");
    log_destroy(logger);
}

void* manejo_filesystem(void *args) {
	int socketFS;
	struct sockaddr_in direccionFS;

	creoSocket(&socketFS, &direccionFS, inet_addr(configuracion->ip_fs), configuracion->puerto_fs);
	puts("Socket de conexion a FileSystem creado correctamente\n");

	conectarSocket(&socketFS, &direccionFS);
	puts("Conectado al File System\n");

	skt_filesystem = socketFS;

	handShakeSend(&socketFS, "100", "401", "File System");

	//Cuando se cierre el Kernel, hay que señalizar estos semáforos para que
	//se cierren los sockets
	sem_wait(&semaforoFileSystem);

	shutdown(socketFS, 0);
	close(socketFS);
	return EXIT_SUCCESS;
}

void* manejo_memoria(void *args) {
	int socketMemoria;
	struct sockaddr_in direccionMemoria;

	creoSocket(&socketMemoria, &direccionMemoria, inet_addr(configuracion->ip_memoria), configuracion->puerto_memoria);
	puts("Socket de conexion a Memoria creado correctamente\n");

	conectarSocket(&socketMemoria, &direccionMemoria);
	puts("Conectado a la Memoria\n");

	skt_memoria = socketMemoria;

	handShakeSend(&socketMemoria, "100", "201", "Memoria");

	asignarCantidadMaximaStackPorProceso();

	//Cuando se cierre el Kernel, hay que señalizar estos semáforos para que
	//se cierren los sockets
	sem_wait(&semaforoMemoria);

	shutdown(socketMemoria, 0);
	close(socketMemoria);
	return EXIT_SUCCESS;
}

void * hilo_conexiones_consola(void *args) {

	struct sockaddr_in direccionKernel;
	int master_socket , addrlen , new_socket , client_socket[MAXCON] , max_clients = MAXCON , activity, i , valread , socketConsola;
	int max_sd;
	struct sockaddr_in address;

	char buffer[MAXBUF];

	//set of socket descriptors
	fd_set readfds;

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++) {
		client_socket[i] = 0;
	}

	//create a master socket
	creoSocket(&master_socket, &direccionKernel, INADDR_ANY, configuracion->puerto_programa);

	//bind the socket to localhost port 8888
	bindSocket(&master_socket, &direccionKernel);

	listen(master_socket, MAXCON);

	//accept the incoming connection
	addrlen = sizeof(address);

	while(1) {
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++) {
			//socket descriptor
			socketConsola = client_socket[i];

			//if valid socket descriptor then add to read list
			if(socketConsola > 0){
				FD_SET( socketConsola , &readfds);
			}

			//highest file descriptor number, need it for the select function
			if(socketConsola > max_sd){
				max_sd = socketConsola;
			}
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

		if ((activity < 0) && (errno!=EINTR)){
			printf("select error");
		}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds)){
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				exit(EXIT_FAILURE);
			}

			printf("%s:%d conectado\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			//send new connection greeting message
			handShakeListen(&new_socket, "300", "101", "199", "Consola");

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++){
				//if position is empty
				if( client_socket[i] == 0 ){
					client_socket[i] = new_socket;
					break;
				}
			}
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++){
			socketConsola = client_socket[i];

			if (FD_ISSET( socketConsola , &readfds)){
				//Check if it was for closing , and also read the incoming message
				if ((valread = read( socketConsola , buffer, MAXBUF)) == 0){
					//Somebody disconnected
					puts("Se desconectó una consola");
					//Close the socket and mark as 0 in list for reuse
					close( socketConsola );
					client_socket[i] = 0;
				}else{
					//set the string terminating NULL byte on the end of the data read
					buffer[valread] = '\0';
					char** respuesta_a_kernel = string_split(buffer, ";");
					int operacion = atoi(respuesta_a_kernel[0]);

					switch(operacion){
						case 303:
							; //https://goo.gl/y7nI85
							char * codigo = respuesta_a_kernel[1];
							iniciarPrograma(codigo, socketConsola, numerador_pcb);
							break;

						case 398:
							; //https://goo.gl/y7nI85
							int pidACerrar = atoi(respuesta_a_kernel[1]);
							finalizarPrograma(pidACerrar);
							break;

						case 399:
							cerrarConsola(socketConsola);
							break;
					}

					free(respuesta_a_kernel);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}

char* serializar_pcb(t_pcb* pcb){

	char* mensajeACPU = string_new();
	string_append(&mensajeACPU, string_itoa(pcb->pid));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->program_counter));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->cantidadPaginas));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->inicio_codigo));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->tabla_archivos));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->pos_stack));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->exit_code));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->indiceCodigo->elements_count));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->etiquetas_size));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->cantidadEtiquetas));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->indiceStack->elements_count));
	string_append(&mensajeACPU, ";");

	string_append(&mensajeACPU, string_itoa(pcb->quantum));
	string_append(&mensajeACPU, ";");

	int i;
	for (i = 0; i < pcb->indiceCodigo->elements_count; i++){
		elementoIndiceCodigo * elem = malloc(sizeof(elem));
		elem = list_get(pcb->indiceCodigo, i);

		string_append(&mensajeACPU, string_itoa(elem->start));
		string_append(&mensajeACPU, ";");

		string_append(&mensajeACPU, string_itoa(elem->offset));
		string_append(&mensajeACPU, ";");
	}

	//printf("Etiquetas Serializacion: \n");

	for (i = 0; i < pcb->etiquetas_size; i++){
		string_append(&mensajeACPU, string_itoa(pcb->etiquetas[i]));
		string_append(&mensajeACPU, ";");
		//printf("[%d]", pcb->etiquetas[i]);
	}

	//printf("\n");

	for (i = 0; i < pcb->indiceStack->elements_count; i++){
		t_Stack *sta = malloc(sizeof(t_Stack));
		sta =  list_get(pcb->indiceStack, i);
		string_append(&mensajeACPU, string_itoa(sta->direccion.offset));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->direccion.pagina));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->direccion.size));
		string_append(&mensajeACPU, ";");
		if (sta->nombre_funcion != NULL){
			string_append(&mensajeACPU, string_itoa(sta->nombre_funcion));
		}
		else {
			string_append(&mensajeACPU, string_itoa(CONST_SIN_NOMBRE_FUNCION));
		}
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->nombre_variable));
		string_append(&mensajeACPU, ";");
	}


	return mensajeACPU;

}

void * hilo_conexiones_cpu(void *args) {

	int socketKernelCPU;
	struct sockaddr_in direccionKernel;
	int socketClienteCPU;
	struct sockaddr_in direccionCPU;
	socklen_t length = sizeof direccionCPU;

	creoSocket(&socketKernelCPU, &direccionKernel, INADDR_ANY, configuracion->puerto_cpu);
	bindSocket(&socketKernelCPU, &direccionKernel);
	listen(socketKernelCPU, MAXCPU);

	while ((socketClienteCPU = accept(socketKernelCPU, (struct sockaddr *) &direccionCPU, (socklen_t*) &length))) {

		pthread_t thread_proceso_cpu;

		printf("%s:%d conectado\n", inet_ntoa(direccionCPU.sin_addr), ntohs(direccionCPU.sin_port));
		creoThread(&thread_proceso_cpu, handler_conexion_cpu, (void *) socketClienteCPU);

	}

	if (socketClienteCPU <= 0){
		printf("Error al aceptar conexiones de CPU\n");
		exit(errno);
	}

	shutdown(socketClienteCPU, 0);
	close(socketClienteCPU);
	return EXIT_SUCCESS;
}

void * handler_conexion_cpu(void * sock) {
	char message[MAXBUF];

	handShakeListen((int *) &sock, "500", "102", "199", "CPU");

	estruct_cpu cpus;
	cpus.socket = *((int *) &sock);
	cpus.pid_asignado = -1;

	pthread_mutex_lock(&mtx_cpu);
	queue_push(cola_cpu, &cpus);
	pthread_mutex_unlock(&mtx_cpu);

	int * socketCliente = (int *) &sock;

	int result = recv(* socketCliente, message, sizeof(message), 0);

	while (result > 0) {

		char* mensajeFileSystem = string_new();
		char**mensajeDesdeCPU = string_split(message, ";");
		int operacion = atoi(mensajeDesdeCPU[0]);
		int pid_mensaje;
		int fd;
		int tamanio;
		char flag;
		char* direccion;
		char* infofile;
		int pid_recibido;
		switch (operacion){
			case 530:
				finDeQuantum(socketCliente);
				break;

			case 531:
				pid_recibido = atoi(mensajeDesdeCPU[1]);
				finDePrograma(pid_recibido);
				break;

			case 532:
				pid_recibido = atoi(mensajeDesdeCPU[1]);
				bloqueoDePrograma(pid_recibido);
				break;

			case 570:
				;
				char* semaforo_buscado = string_new();
				semaforo_buscado = mensajeDesdeCPU[1];
				waitSemaforo(socketCliente, semaforo_buscado);
				break;
			case 804: //escribir archivo


			     fd = atoi(mensajeDesdeCPU[1]);
				 pid_mensaje = atoi(mensajeDesdeCPU[2]);
				 infofile = mensajeDesdeCPU[2];
				 tamanio = atoi(mensajeDesdeCPU[3]);

				escribirArchivo(fd, pid_mensaje, infofile, tamanio);

				break;

			case 803: //de CPU a File system (abrir)

				//validar();
				 pid_mensaje = atoi(mensajeDesdeCPU[1]);
				 direccion = mensajeDesdeCPU[2];
				 flag = mensajeDesdeCPU[3];

				abrirArchivo(pid_mensaje, direccion, flag);

				enviarMensaje(&skt_filesystem, mensajeFileSystem);
				break;

			case 802:  //de CPU a File system (borrar)

				 infofile = string_new();
			     pid_mensaje = atoi(mensajeDesdeCPU[1]);
				 fd = atoi(mensajeDesdeCPU[2]);

				 //lista_File_global

				//borrarArchivo(pid_mensaje, direccion, flag);

				enviarMensaje(&skt_filesystem, mensajeFileSystem);
				break;

			case 801://de CPU a File system (cerrar)

				string_append(&mensajeFileSystem, string_itoa(operacion));
				string_append(&mensajeFileSystem, ";");

				enviarMensaje(&skt_filesystem, mensajeFileSystem);
				break;

			case 800://de CPU a File system (cerrar)

				string_append(&mensajeFileSystem, string_itoa(operacion));
				string_append(&mensajeFileSystem, ";");

				enviarMensaje(&skt_filesystem, mensajeFileSystem);
				break;

			case 571:
				;
				char* otro_semaforo_buscado = string_new();
				otro_semaforo_buscado = mensajeDesdeCPU[1];
				signalSemaforo(socketCliente, otro_semaforo_buscado);
				break;

			case 515:
				;
				char * variable = mensajeDesdeCPU[1];
				int valor = atoi(mensajeDesdeCPU[2]);
				asignarValorCompartida(variable, valor);
				break;

			case 514:
				;
				char * otra_variable = mensajeDesdeCPU[1];
				obtenerValorCompartida(otra_variable, socketCliente);
				break;

			case 600:
				;
				int un_pid = atoi(mensajeDesdeCPU[1]);
				int bytes = atoi(mensajeDesdeCPU[2]);
				t_pcb * pcb = pcbFromPid(un_pid);
				reservarMemoriaHeap(pcb, bytes, * socketCliente);
				break;

			case 615:
				;
				int pid_msg = atoi(mensajeDesdeCPU[1]);
				finalizarPrograma(pid_msg);
				break;

		}

		result = recv(* socketCliente, message, sizeof(message), 0);
	}

	if (result <= 0) {
		int corte, i, encontrado;

		pthread_mutex_lock(&mtx_cpu);
		corte = queue_size(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		i = 0;
		encontrado = 0;

		estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

		while (i <= corte && encontrado == 0){

			pthread_mutex_lock(&mtx_cpu);
			temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
			pthread_mutex_unlock(&mtx_cpu);

			if(temporalCpu->socket == *socketCliente){
				encontrado = 1;
			}else{

				pthread_mutex_lock(&mtx_cpu);
				queue_push(cola_cpu, temporalCpu);
				pthread_mutex_unlock(&mtx_cpu);

				i++;
			}
		}
		printf("Se desconecto un CPU\n");
	}

	return EXIT_SUCCESS;
}

t_pcb *nuevo_pcb(int pid, int* socket_consola){
	t_pcb* new = malloc(sizeof(t_pcb));

	new->pid = pid;
	new->program_counter = 0;
	new->cantidadPaginas = 0;
	new->indiceCodigo = list_create();
	//new->indiceEtiquetas = list_create();
	new->indiceStack = list_create();
	new->inicio_codigo = 0;
	new->tabla_archivos = 0;
	new->pos_stack = 0;
	new->socket_consola = socket_consola;
	new->exit_code = 0;
	new->quantum = configuracion->quantum;

	pthread_mutex_lock(&mutex_numerador_pcb);
	numerador_pcb++;
	pthread_mutex_unlock(&mutex_numerador_pcb);
	return new;
}

t_queue* crear_cola_pcb(){
	t_queue* cola = queue_create();
	return cola;
}

void flush_cola_pcb(t_queue* queue){
	 queue_destroy_and_destroy_elements(queue, (void*) eliminar_pcb);
}

//void eliminar_pcb(t_pcb *self){
void eliminar_pcb(void * voidSelf){
	t_pcb * self = (t_pcb *) voidSelf;
	free(self->tabla_archivos);
	free(self);
}

void switchear_colas(t_queue* origen, t_queue* fin, t_pcb* element){
	queue_pop(origen);
	queue_push(fin, element);
}

void logExitCode(int code)
{
	char* errorLog = string_new();
	switch(code){
	case FIN_OK:
		errorLog = "El programa finalizó correctamente";
		break;
	case FIN_ERROR_RESERVA_RECURSOS:
		errorLog = "No se pudieron reservar recursos para ejecutar el programa";
		break;
	case FIN_ERROR_ACCESO_ARCHIVO_INEXISTENTE:
		errorLog = "El programa intentó acceder a un archivo que no existe.";
		break;
	case FIN_ERROR_LEER_ARCHIVO_SIN_PERMISOS:
		errorLog = "El programa intentó leer un archivo sin permisos.";
		break;
	case FIN_ERROR_ESCRIBIR_ARCHIVO_SIN_PERMISOS:
		errorLog = "El programa intentó escribir un archivo sin permisos.";
		break;
	case FIN_ERROR_EXCEPCION_MEMORIA:
		errorLog = "Excepción de memoria";
		break;
	case FIN_POR_DESCONEXION_CONSOLA:
		errorLog = "Finalizado a través de desconexión de consola";
		break;
	case FIN_POR_CONSOLA:
		errorLog = "Finalizado a través de comando Finalizar Programa de la consola";
		break;
	case FIN_ERROR_RESERVA_MEMORIA_MAYOR_A_PAGINA:
		errorLog = "Se intento reservar mas memoria que el tamaño de una página";
		break;
	case FIN_ERROR_SUPERO_MAXIMO_PAGINAS:
		errorLog = "No se pueden asignar mas páginas al proceso";
		break;
	case FIN_ERROR_SIN_DEFINICION:
		errorLog = "Error sin definición";
		break;
	}
	t_log* logCode = log_create("kernelExist.log", "kernel", true, LOG_LEVEL_ERROR );
	log_error(logCode, errorLog, "EXITCODE");
    log_destroy(logCode);

}

void * planificar(){
	int corte, i, encontrado;

	while (1){

		if(plan == 0){
		usleep(configuracion->quantum_sleep);

		pthread_mutex_lock(&mtx_cpu);
		corte = queue_size(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		i = 0;
		encontrado = 0;

		if(!(queue_is_empty(cola_cpu))){
			estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

			while (i <= corte && encontrado == 0){

				pthread_mutex_lock(&mtx_cpu);
				temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
				pthread_mutex_unlock(&mtx_cpu);

				if(temporalCpu->pid_asignado == -1){

					encontrado = 1;
				}else{
					i++;
				}

				pthread_mutex_lock(&mtx_cpu);
				queue_push(cola_cpu, temporalCpu);
				pthread_mutex_unlock(&mtx_cpu);
			}

			if(encontrado == 1){
				if(!(queue_is_empty(cola_listos))){

					t_pcb* pcbtemporalListos = malloc(sizeof(t_pcb));

					pthread_mutex_lock(&mtx_listos);
					pcbtemporalListos = (t_pcb*) queue_pop(cola_listos);
					pthread_mutex_unlock(&mtx_listos);

					temporalCpu->pid_asignado = pcbtemporalListos->pid;
					pcbtemporalListos->socket_cpu = &temporalCpu->socket;

					char* mensajeACPUPlan = serializar_pcb(pcbtemporalListos);
					enviarMensaje(pcbtemporalListos->socket_cpu, mensajeACPUPlan);

					pthread_mutex_lock(&mtx_ejecucion);
					queue_push(cola_ejecucion, pcbtemporalListos);
					pthread_mutex_unlock(&mtx_ejecucion);

					free(mensajeACPUPlan);
				}
			}
		}
		}
	}
}


bool esComentario(char* linea){
	//return string_starts_with(linea, TEXT_COMMENT); //me pincha porque esta entre comillas simples?
	return string_starts_with(linea, "#");
}

bool esNewLine(char* linea){
	return string_starts_with(linea, "\n");
}

char * limpioCodigo(char * codigo){
		char * curLine = codigo;
		char * codigoLimpio = string_new();

		   while(curLine)
		   {
		      char * nextLine = strchr(curLine, '\n');
		      if (nextLine){
		    	  *nextLine = '\0';
		      }

		      if (!esComentario(curLine) && !esNewLine(curLine)){
		    	  	 string_append(&codigoLimpio, curLine);
		    	  	 string_append(&codigoLimpio, "\n");
		      }

		      if (nextLine){
		    	  *nextLine = '\n';
		      }

		      curLine = nextLine ? (nextLine+1) : NULL;
		   }

		   return codigoLimpio;
	}

void cargoIndicesPCB(t_pcb * pcb, char * codigo){
	t_metadata_program * metadataProgram;

	metadataProgram = metadata_desde_literal(codigo);

	int i;
	for (i = 0; i < metadataProgram->instrucciones_size; i++){

		elementoIndiceCodigo * elem = malloc(sizeof(elementoIndiceCodigo));
		elem->start = metadataProgram->instrucciones_serializado[i].start;
		elem->offset = metadataProgram->instrucciones_serializado[i].offset;
		list_add(pcb->indiceCodigo, elem);

	}

	i = 0;
	pcb->etiquetas = malloc(metadataProgram->etiquetas_size);
	while (i < metadataProgram->etiquetas_size){
		pcb->etiquetas[i] = metadataProgram->etiquetas[i];
		i++;
	}
	pcb->etiquetas_size = metadataProgram->etiquetas_size;
	pcb->cantidadEtiquetas = metadataProgram->cantidad_de_etiquetas;
	pcb->cantidadEtiquetas = metadataProgram->cantidad_de_funciones; //ACA NO FALTA UN cantidadFunciones?

	metadata_destruir(metadataProgram);

}

t_pcb * deserializar_pcb(char * mensajeRecibido){

	t_pcb * pcb = malloc(sizeof(t_pcb));
	int cantIndiceCodigo, cantIndiceStack;
	char ** message = string_split(mensajeRecibido, ";");
	pcb->indiceCodigo = list_create();
	//pcb->indiceEtiquetas = list_create();
	pcb->indiceStack = list_create();

	pcb->pid = atoi(message[0]);
	pcb->program_counter = atoi(message[1]);
	pcb->cantidadPaginas = atoi(message[2]);
	pcb->inicio_codigo = atoi(message[3]);
	pcb->tabla_archivos = atoi(message[4]);
	pcb->pos_stack = atoi(message[5]);
	pcb->exit_code = atoi(message[6]);
	cantIndiceCodigo = atoi(message[7]);
	pcb->etiquetas_size = atoi(message[8]);
	pcb->cantidadEtiquetas = atoi(message[9]);
	cantIndiceStack = atoi(message[10]);
	pcb->quantum = atoi(message[11]);

	int i = 12;

	while (i < 12 + cantIndiceCodigo * 2){
		elementoIndiceCodigo * elem = malloc(sizeof(elem));
		elem->start = atoi(message[i]);
		i++;
		elem->offset = atoi(message[i]);
		i++;
		list_add(pcb->indiceCodigo, elem);
	}

	int j = i;

	int e = 0;

	//printf("Etiquetas Deserializacion: \n");

	if (pcb->etiquetas_size > 0){
		pcb->etiquetas = malloc(pcb->etiquetas_size);
			while (i < j + pcb->etiquetas_size){
				int ascii = atoi(message[i]);

				if (ascii >= 32 && ascii <= 126){
					pcb->etiquetas[e] = (char) ascii;
				} else {
					pcb->etiquetas[e] = atoi(message[i]);
				}

				//printf("[%d]", pcb->etiquetas[z]);
				i++;
				e++;
			}
	}

	/*
	printf("\n");

	printf("Etiquetas DE NUEVO EN LA DESERIALIZACION:\n");

	z = 0;

	while (z < pcb->etiquetas_size){
		printf("[%d]", pcb->etiquetas[z]);
		z++;
	}

	printf("\n");
	*/

	while(cantIndiceStack > 0) {
		t_Stack *sta = malloc(sizeof(t_Stack));

		sta->direccion.offset = atoi(message[i]);
		i++;
		sta->direccion.pagina = atoi(message[i]);
		i++;
		sta->direccion.size = atoi(message[i]);
		i++;
		int nro_funcion = atoi(message[i]);
		if (nro_funcion != CONST_SIN_NOMBRE_FUNCION){
			char nombre_funcion = nro_funcion;
			sta->nombre_funcion = nombre_funcion;
			i++;
		} else {
			i++;
		}
		char nombre_variable = atoi(message[i]);
		sta->nombre_variable = nombre_variable;

		list_add(pcb->indiceStack, sta);
		i++;
		cantIndiceStack--;
	}

	return pcb;
}

void * multiprogramar() {
	//validacion de nivel de multiprogramacion
	while (1) {
		if ((queue_size(cola_listos) + (queue_size(cola_bloqueados) + (queue_size(cola_ejecucion)))) < grado_multiprogramacion) {
			//Se crea programa nuevo

			if (queue_size(cola_nuevos) > 0) {
				t_nuevo* nue = queue_pop(cola_nuevos);
				t_pcb * new_pcb = nuevo_pcb(numerador_pcb, &(nue->skt));
				envioProgramaAMemoria(new_pcb, nue);
				free(nue);
			}
		}
	}
}

void envioProgramaAMemoria(t_pcb * new_pcb, t_nuevo * nue){
	char* mensajeInicioPrograma = string_new();
	string_append(&mensajeInicioPrograma, string_itoa(new_pcb->pid));
	string_append(&mensajeInicioPrograma, ";");
	string_append(&mensajeInicioPrograma, nue->codigo);
	enviarMensaje(&skt_memoria, mensajeInicioPrograma);

	char message[MAXBUF];

	int result = recv(skt_memoria, message, sizeof(message), 0);

	if (result > 0){
		//Recepcion de respuesta de la Memoria sobre validacion de espacio para almacenar script
		char** respuesta_Memoria = string_split(message, ";");

		int operacion = atoi(respuesta_Memoria[0]);
		int socketConsola = nue->skt;

		switch (operacion) {
		case 298:

			rechazoFaltaMemoria(socketConsola);
			break;

		case 203:
			;
			int inicio_codigo = atoi(respuesta_Memoria[1]);
			int cantidadPaginas = atoi(respuesta_Memoria[2]);
			creoPrograma(new_pcb, nue->codigo, inicio_codigo, cantidadPaginas);
			informoAConsola(socketConsola, new_pcb->pid);
			break;
		}

		free(respuesta_Memoria);
		free(mensajeInicioPrograma);
	} else {
		printf("Error al enviar Programa a Memoria\n");
	}
}

void rechazoFaltaMemoria(int socketConsola){
	puts("Se rechaza programa por falta de espacio en memoria\n");
	//Decremento el numero de pid global del kernel
	pthread_mutex_lock(&mutex_numerador_pcb);
	numerador_pcb--;
	pthread_mutex_unlock(&mutex_numerador_pcb);
	enviarMensaje(&socketConsola, "197;");
}

void creoPrograma(t_pcb * new_pcb, char * codigo, int inicio_codigo, int cantidadPaginas){
	//Si hay espacio suficiente en la memoria
	//Agrego el programa a la cola de listos
	printf("Se creo el programa %d \n", new_pcb->pid);
	puts("");

	new_pcb->inicio_codigo = inicio_codigo;
	new_pcb->cantidadPaginas = cantidadPaginas;
	cargoIndicesPCB(new_pcb, codigo);

	pthread_mutex_lock(&mtx_listos);
	queue_push(cola_listos, new_pcb);
	pthread_mutex_unlock(&mtx_listos);
}

void informoAConsola(int socketConsola, int pid){
	// INFORMO A CONSOLA EL RESULTADO DE LA CREACION DEL PROCESO
	char* respuestaAConsola = string_new();
	string_append(&respuestaAConsola, "103");
	string_append(&respuestaAConsola, ";");
	string_append(&respuestaAConsola, string_itoa(pid));
	string_append(&respuestaAConsola, ";");
	enviarMensaje(&socketConsola, respuestaAConsola);

	free(respuestaAConsola);
}

t_pcb * pcbFromPid(int pid){
	int mismoPid (t_pcb * pcb){
		return pcb->pid == pid;
	}

	return list_find(lista_procesos, mismoPid);
}

void asignarCantidadMaximaStackPorProceso(){
	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "612");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(configuracion->stack_size));
	string_append(&mensajeAMemoria, ";");
	enviarMensaje(&skt_memoria, mensajeAMemoria);

	free(mensajeAMemoria);
}

void reservarMemoriaHeap(t_pcb * pcb, int bytes, int socketCPU){
	int paginaActual = pcb->cantidadPaginas;

	/*
	string_append(&solicitud, "600");
	string_append(&solicitud, ";");
	string_append(&solicitud, string_itoa(pcb->pid));
	string_append(&solicitud, ";");
	string_append(&solicitud, string_itoa(paginaActual));
	string_append(&solicitud, ";");
	string_append(&solicitud, string_itoa(bytes));
	string_append(&solicitud, ";");
	*/
	enviarMensaje(&skt_memoria, serializarMensaje(4, 600, pcb->pid, paginaActual, bytes));

	char * message = string_new();

	int result = recv(skt_memoria, message, MAXBUF, 0);

	if (result > 0){
		heapElement * heapElem = malloc(sizeof(heapElement));

		char ** respuesta = string_split(message, ";");

		heapElem->pid = atoi(respuesta[0]);
		heapElem->nro_pagina = atoi(respuesta[1]);
		heapElem->tamanio_disponible = atoi(respuesta[2]);

		list_add(heap, heapElem);

		printf("Se agregó una página al Heap\n");

		printf("PID: %d\n", heapElem->pid);
		printf("Nro Pagina: %d\n", heapElem->nro_pagina);
		printf("Free Space: %d\n", heapElem->tamanio_disponible);

		enviarMensaje(&socketCPU, serializarMensaje(3, heapElem->pid, heapElem->nro_pagina, heapElem->tamanio_disponible));

	} else {
		perror("Error reservando Memoria de Heap\n");
	}

	free(message);
}

void liberarMemoriaHeap(){

}

void heapElementDestroyer(void * heapElement){
	free(heapElement);
}

void iniciarPrograma(char * codigo, int socketCliente, int pid) {
	//validacion de nivel de multiprogramacion
	if ((queue_size(cola_listos) + (queue_size(cola_bloqueados) + (queue_size(cola_ejecucion)))) == grado_multiprogramacion) {
		t_nuevo* nue = malloc(sizeof(t_nuevo));
		nue->codigo = string_new();
		nue->codigo = limpioCodigo(codigo);
		nue->skt = socketCliente;

		pthread_mutex_lock(&mtx_nuevos);
		queue_push(cola_nuevos, nue);
		pthread_mutex_unlock(&mtx_nuevos);
	} else {
		//Se crea programa nuevo
		t_pcb * new_pcb = nuevo_pcb(pid, &socketCliente);

		char * mensajeInicioPrograma = string_new();
		char * codigoLimpio = limpioCodigo(codigo);
		string_append(&mensajeInicioPrograma, "250");
		string_append(&mensajeInicioPrograma, ";");
		string_append(&mensajeInicioPrograma, string_itoa(new_pcb->pid));
		string_append(&mensajeInicioPrograma, ";");
		string_append(&mensajeInicioPrograma, codigoLimpio);
		string_append(&mensajeInicioPrograma, ";");
		enviarMensaje(&skt_memoria, mensajeInicioPrograma);

		char message[MAXBUF];
		recv(skt_memoria, message, sizeof(message), 0);

		//Recepcion de respuesta de la Memoria sobre validacion de espacio para almacenar script
		char** respuesta_Memoria = string_split(message, ";");

		if (atoi(respuesta_Memoria[0]) == 298) {
			puts("Se rechaza programa por falta de espacio en memoria");

			//Decremento el numero de pid global del kernel
			pthread_mutex_lock(&mutex_numerador_pcb);
			numerador_pcb--;
			pthread_mutex_unlock(&mutex_numerador_pcb);

			char message2[MAXBUF];
			strcpy(message2, "197;");
			enviarMensaje(&socketCliente, message2);
		} else {
			//Si hay espacio suficiente en la memoria
			//Agrego el programa a la cola de listos
			printf("Se creo el programa %d \n", new_pcb->pid);

			puts("");
			//printf("%s", buffer);
			printf("%s", codigo);
			puts("");

			new_pcb->inicio_codigo = atoi(respuesta_Memoria[1]);
			new_pcb->cantidadPaginas = atoi(respuesta_Memoria[2]);
			cargoIndicesPCB(new_pcb, codigoLimpio);

			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, new_pcb);
			list_add(lista_procesos, new_pcb);
			pthread_mutex_unlock(&mtx_listos);

			// INFORMO A CONSOLA EL RESULTADO DE LA CREACION DEL PROCESO
			char* info_pid = string_new();
			char* respuestaAConsola = string_new();
			string_append(&info_pid, "103");
			string_append(&info_pid, ";");
			string_append(&info_pid, string_itoa(new_pcb->pid));
			string_append(&respuestaAConsola, info_pid);
			enviarMensaje(&socketCliente, respuestaAConsola);

			free(info_pid);
			free(respuestaAConsola);
		}

		free(respuesta_Memoria);
		free(mensajeInicioPrograma);
	}
}

void finalizarPrograma(int pidACerrar) {
	t_pcb * temporalN;

	int encontre = 0;

	int largoCola = queue_size(cola_listos);

	int mismoPid (t_pcb * pcb){
		return pcb->pid == pidACerrar;
	}

	//busco pid en cola listos
	while (encontre == 0 && largoCola != 0) {

		pthread_mutex_lock(&mtx_listos);
		temporalN = (t_pcb*) queue_pop(cola_listos);
		pthread_mutex_unlock(&mtx_listos);
		largoCola--;

		if (temporalN->pid == pidACerrar) {

			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
			encontre = 1;
			printf("Se termino el proceso: %d\n", temporalN->pid);

		} else {

			//puts("Me llego el 398 y el proceso no existe");
			printf("Se intento cerrar un programa que no existe\n");
			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, temporalN);
			pthread_mutex_unlock(&mtx_listos);

		}
	}

	int encontreBloq = 0;
	int largoColaBloq = queue_size(cola_bloqueados);

	//busco pid en cola bloqueados
	while (encontreBloq == 0 && largoColaBloq != 0) {

		pthread_mutex_lock(&mtx_bloqueados);
		temporalN = (t_pcb*) queue_pop(cola_bloqueados);
		pthread_mutex_unlock(&mtx_bloqueados);
		largoColaBloq--;

		if (temporalN->pid == pidACerrar) {

			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
			encontreBloq = 1;
			printf("Se termino el proceso: %d\n", temporalN->pid);

		} else {

			pthread_mutex_lock(&mtx_bloqueados);
			queue_push(cola_bloqueados, temporalN);
			pthread_mutex_unlock(&mtx_bloqueados);
			printf("Se intento cerrar un programa que no existe\n");

		}
	}

	int encontreEjec = 0;
	int largoColaEjec = queue_size(cola_ejecucion);

	//busco pid en cola bloqueados
	while (encontreEjec == 0 && largoColaEjec != 0) {

		temporalN = (t_pcb*) queue_pop(cola_ejecucion);
		largoColaEjec--;

		if (temporalN->pid == pidACerrar) {

			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
			encontreEjec = 1;
			printf("Se termino el proceso: %d", temporalN->pid);

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, temporalN);
			pthread_mutex_unlock(&mtx_ejecucion);
			printf("Se intento cerrar un programa que no existe\n");

		}
	}

}

void cerrarConsola(int socketCliente) {
	// Buscar en las colas de listos, bloqueadosa y en ejecucion a todos los programas
	//cuyo socket_consola sea igual al que envio este mensaje y matarlos.

	t_pcb * temporalN;

	int largoColaListos = queue_size(cola_listos);

	//busco pid en cola listos

	while (largoColaListos != 0) {
		pthread_mutex_lock(&mtx_listos);
		temporalN = (t_pcb*) queue_pop(cola_listos);
		printf("El skt es %d \n", *(temporalN->socket_consola));
		printf("El skt buscado es %d \n", socketCliente);
		pthread_mutex_unlock(&mtx_listos);
		largoColaListos--;
		if (*(temporalN->socket_consola) == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
		} else {
			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, temporalN);
			pthread_mutex_unlock(&mtx_listos);
		}
	}

	int largoColaBloq = queue_size(cola_bloqueados);

	//busco pid en cola bloqueados

	while (largoColaBloq != 0) {
		pthread_mutex_lock(&mtx_bloqueados);
		temporalN = (t_pcb*) queue_pop(cola_bloqueados);
		pthread_mutex_unlock(&mtx_bloqueados);
		largoColaBloq--;
		if (*(temporalN->socket_consola) == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
		} else {
			pthread_mutex_lock(&mtx_bloqueados);
			queue_push(cola_bloqueados, temporalN);
			pthread_mutex_unlock(&mtx_bloqueados);
		}
	}

	int largoColaEjec = queue_size(cola_ejecucion);

	//busco pid en cola bloqueados

	while (largoColaEjec != 0) {
		pthread_mutex_lock(&mtx_ejecucion);
		temporalN = (t_pcb*) queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);
		largoColaEjec--;
		if (*(temporalN->socket_consola) == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);
			pthread_mutex_unlock(&mtx_terminados);
		} else {
			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, temporalN);
			pthread_mutex_unlock(&mtx_ejecucion);
		}
	}
}

void finDeQuantum(int * socketCliente){
	char message[MAXBUF];
	recv(* socketCliente, &message, sizeof(message), 0);

	t_pcb* pcb_deserializado = malloc(sizeof(t_pcb));
	pcb_deserializado = deserializar_pcb(message);

	int pid_a_buscar = pcb_deserializado->pid;

	int encontrado = 0;

	int size = queue_size(cola_ejecucion);

	int iter = 0;

	while (encontrado == 0 && iter < size) {

		pthread_mutex_lock(&mtx_ejecucion);
		t_pcb* pcb_a_cambiar = queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);

		if (pcb_a_cambiar->pid == pid_a_buscar) {

			pcb_a_cambiar = pcb_deserializado;

			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, pcb_a_cambiar);
			pthread_mutex_unlock(&mtx_listos);

			encontrado = 1;

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, pcb_a_cambiar);
			pthread_mutex_unlock(&mtx_ejecucion);

		}

		iter++;
	}

	encontrado = 0;

	estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

	while (encontrado == 0) { //Libero la CPU que estaba ejecutando al programa

		pthread_mutex_lock(&mtx_cpu);
		temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if (temporalCpu->pid_asignado == pid_a_buscar) {
			encontrado = 1;
		}

		temporalCpu->pid_asignado = -1;

		pthread_mutex_lock(&mtx_cpu);
		queue_push(cola_cpu, temporalCpu);
		pthread_mutex_unlock(&mtx_cpu);

	}
}

void listar_terminados(){
	int fin = queue_size((cola_terminados));

	if(fin == 0){
		puts("No hay programas terminados");
	}

	while(fin > 0){
		pthread_mutex_lock(&mtx_terminados);
		t_pcb* aux = queue_pop(cola_terminados);
		pthread_mutex_unlock(&mtx_terminados);

		printf("Programa: %d \n", aux->pid);

		pthread_mutex_lock(&mtx_terminados);
		queue_push(cola_terminados, aux);
		pthread_mutex_unlock(&mtx_terminados);

		fin--;
	}

	return;
}

void listar_listos(){
	int fin = queue_size((cola_listos));

	if(fin == 0){
		puts("No hay programas listos");
	}

	while(fin > 0){
		pthread_mutex_lock(&mtx_listos);
		t_pcb* aux = queue_pop(cola_listos);
		pthread_mutex_unlock(&mtx_listos);

		printf("Programa: %d \n", aux->pid);

		pthread_mutex_lock(&mtx_listos);
		queue_push(cola_listos, aux);
		pthread_mutex_unlock(&mtx_listos);

		fin--;
	}

	return;
}

void listar_bloqueados(){
	int fin = queue_size((cola_bloqueados));

	if(fin == 0){
		puts("No hay programas bloqueados");
	}

	while(fin > 0){
		pthread_mutex_lock(&mtx_bloqueados);
		t_pcb* aux = queue_pop(cola_bloqueados);
		pthread_mutex_unlock(&mtx_bloqueados);

		printf("Programa: %d \n", aux->pid);

		pthread_mutex_lock(&mtx_bloqueados);
		queue_push(cola_bloqueados, aux);
		pthread_mutex_unlock(&mtx_bloqueados);

		fin--;
	}

	return;
}

void listar_ejecucion(){
	int fin = queue_size((cola_ejecucion));

	if(fin == 0){
		puts("No hay programas terminados");
	}

	while(fin > 0){
		pthread_mutex_lock(&mtx_ejecucion);
		t_pcb* aux = queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);

		puts("Programas Terminados:");
		printf("%d \n", aux->pid);

		pthread_mutex_lock(&mtx_ejecucion);
		queue_push(cola_ejecucion, aux);
		pthread_mutex_unlock(&mtx_ejecucion);

		fin--;
	}

	return;
}

void bloqueoDePrograma(int pid_a_buscar){
	int encontrado = 0;

	int size = queue_size(cola_ejecucion);

	int iter = 0;

	t_pcb* pcb_a_cambiar;

	while (encontrado == 0 && iter < size) {

		pthread_mutex_lock(&mtx_ejecucion);
		pcb_a_cambiar = queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);

		if (pcb_a_cambiar->pid == pid_a_buscar) {

			encontrado = 1;

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, pcb_a_cambiar);
			pthread_mutex_unlock(&mtx_ejecucion);

		}

		iter++;
	}

	if(encontrado == 1){
		pthread_mutex_lock(&mtx_bloqueados);
		queue_push(cola_bloqueados, pcb_a_cambiar);
		pthread_mutex_unlock(&mtx_bloqueados);
	}

	encontrado = 0;

	estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

	while (encontrado == 0) { //Libero la CPU que estaba ejecutando al programa

		pthread_mutex_lock(&mtx_cpu);
		temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if (temporalCpu->pid_asignado == pid_a_buscar) {
			encontrado = 1;
		}

		temporalCpu->pid_asignado = -1;

		pthread_mutex_lock(&mtx_cpu);
		queue_push(cola_cpu, temporalCpu);
		pthread_mutex_unlock(&mtx_cpu);
	}

}

void finDePrograma(int pid_a_buscar){
	int encontrado = 0;

	int size = queue_size(cola_ejecucion);

	int iter = 0;

	t_pcb* pcb_a_cambiar;

	while (encontrado == 0 && iter < size) {

		pthread_mutex_lock(&mtx_ejecucion);
		pcb_a_cambiar = queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);

		if (pcb_a_cambiar->pid == pid_a_buscar) {

			encontrado = 1;

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, pcb_a_cambiar);
			pthread_mutex_unlock(&mtx_ejecucion);

		}

		iter++;
	}

	if(encontrado == 1){
		pthread_mutex_lock(&mtx_terminados);
		queue_push(cola_terminados, pcb_a_cambiar);
		pthread_mutex_unlock(&mtx_terminados);
	}

	encontrado = 0;

	estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

	while (encontrado == 0) { //Libero la CPU que estaba ejecutando al programa

		pthread_mutex_lock(&mtx_cpu);
		temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if (temporalCpu->pid_asignado == pid_a_buscar) {
			encontrado = 1;
		}

		temporalCpu->pid_asignado = -1;

		pthread_mutex_lock(&mtx_cpu);
		queue_push(cola_cpu, temporalCpu);
		pthread_mutex_unlock(&mtx_cpu);
	}

}

void waitSemaforo(int * socketCliente, char * semaforo_buscado){
	int logrado = 0;

	while (logrado == 0) {

		int encontrar_sem(t_globales* glo) {
			return string_starts_with(semaforo_buscado, glo->nombre);
		}

		t_globales* sem = list_find(lista_semaforos, encontrar_sem);

		if(sem->valor == 0){

			int fin = queue_size(cola_ejecucion);
			int encontrado = 0;
			t_pcb* p;

			while(fin > 0 && encontrado == 0){

				pthread_mutex_lock(&mtx_ejecucion);
				p = queue_pop(cola_ejecucion);
				pthread_mutex_unlock(&mtx_ejecucion);

				if(*(p->socket_cpu) == *(socketCliente)){
					t_bloqueo* bloq = malloc(sizeof(t_bloqueo));
					bloq->pid = p->pid;
					bloq->sem = string_new();
					bloq->sem = sem->nombre;
					puts("entre");
					list_add(registro_bloqueados, bloq);

					encontrado = 1;
				}

				pthread_mutex_lock(&mtx_ejecucion);
				queue_push(cola_ejecucion, p);
				pthread_mutex_unlock(&mtx_ejecucion);

				fin--;
			}

			char* mensajeACPU = string_new();
			string_append(&mensajeACPU, "577");
			string_append(&mensajeACPU, ";");

			enviarMensaje(socketCliente, mensajeACPU);
			logrado = 1;

			free(mensajeACPU);
		}

		if (sem->valor > 0) {
			pthread_mutex_lock(&mtx_semaforos);
			sem->valor--;
			pthread_mutex_unlock(&mtx_semaforos);

			char* mensajeACPU = string_new();
			string_append(&mensajeACPU, "570");
			string_append(&mensajeACPU, ";");

			enviarMensaje(socketCliente, mensajeACPU);
			logrado = 1;

			free(mensajeACPU);
		}
	}

	free(semaforo_buscado);
}

void signalSemaforo(int * socketCliente, char * semaforo_buscado){

	int encontrar_sem(t_globales* glo) {
		return string_starts_with(semaforo_buscado, glo->nombre);
	}

	t_globales* sem = list_find(lista_semaforos, (void*) encontrar_sem);

	int encontrar_bloqueado(t_bloqueo* bloq){

		return string_starts_with(sem->nombre,bloq->sem);
	}

	t_bloqueo* bloq = list_find(registro_bloqueados, (void *) encontrar_bloqueado);

	if(bloq != NULL){

		int fin = queue_size(cola_bloqueados);
		int encontrado = 0;
		t_pcb* p;

		while(fin > 0 && encontrado == 0){

		pthread_mutex_lock(&mtx_bloqueados);
		p = queue_pop(cola_bloqueados);
		pthread_mutex_unlock(&mtx_bloqueados);

		if(p->pid == bloq->pid){

			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, p);
			pthread_mutex_unlock(&mtx_listos);

			int indice = 0;
			while(encontrado == 0){
				t_bloqueo* b = list_get(registro_bloqueados, indice);
				if(b->pid == bloq->pid){
					list_remove(registro_bloqueados, indice);
					free(b);
					encontrado = 1;
				}else{
					indice++;
				}
			}

		}else{
			pthread_mutex_lock(&mtx_bloqueados);
			queue_push(cola_bloqueados, p);
			pthread_mutex_unlock(&mtx_bloqueados);
		}

		fin --;

		}
	}

	pthread_mutex_lock(&mtx_semaforos);
	sem->valor++;
	pthread_mutex_unlock(&mtx_semaforos);

	char* mensajeACPU = string_new();
	string_append(&mensajeACPU, "571");
	string_append(&mensajeACPU, ";");

	enviarMensaje(socketCliente, mensajeACPU);

	free(mensajeACPU);

}

void asignarValorCompartida(char * variable, int valor){

	char* var_comp = string_new();
	string_append(&var_comp, "!");
	string_append(&var_comp, variable);

	int encontrar_sem(t_globales* glo) {
		return string_starts_with(var_comp, glo->nombre);
	}

	t_globales *var_glo = list_find(lista_variables_globales,
			(void *) encontrar_sem);

	pthread_mutex_lock(&mtx_globales);
	var_glo->valor = valor;
	pthread_mutex_unlock(&mtx_globales);

}

void obtenerValorCompartida(char * variable, int * socketCliente){
	char* var_comp = string_new();
	string_append(&var_comp, "!");
	string_append(&var_comp, variable);

	int encontrar_sem(t_globales* glo) {
		return string_starts_with(var_comp, glo->nombre);
	}

	t_globales* var_glo = list_find(lista_variables_globales,
			(void *) encontrar_sem);

	char* mensajeACPU = string_new();
	string_append(&mensajeACPU, string_itoa(var_glo->valor));
	string_append(&mensajeACPU, ";");

	enviarMensaje(socketCliente, mensajeACPU);

	free(mensajeACPU);
}

void escribirArchivo(int fd, int pid_mensaje, char * info, int tamanio){
	/*
	char* info = string_new();
	int fd = atoi(mensajeDesdeCPU[1]);
	info = mensajeDesdeCPU[3];
	int pid_mensaje = atoi(mensajeDesdeCPU[2]);
	*/

	if (fd == 1) {
		t_pcb* temporalN;
		int encontreEjec = 0;
		int largoColaEjec = queue_size(cola_ejecucion);

		//busco pid en cola bloqueados
		while (encontreEjec == 0 && largoColaEjec != 0) {
			temporalN = (t_pcb*) queue_pop(cola_ejecucion);
			largoColaEjec--;
			if (temporalN->pid == pid_mensaje) {
				pthread_mutex_lock(&mtx_terminados);
				queue_push(cola_terminados, temporalN);
				pthread_mutex_unlock(&mtx_terminados);
				encontreEjec = 1;
			} else {
				pthread_mutex_lock(&mtx_ejecucion);
				queue_push(cola_ejecucion, temporalN);
				pthread_mutex_unlock(&mtx_ejecucion);
			}
		}

		char* mensajeAConso = string_new();
		string_append(&mensajeAConso, "802");
		string_append(&mensajeAConso, ";");
		string_append(&mensajeAConso, string_itoa(pid_mensaje));
		string_append(&mensajeAConso, ";");
		string_append(&mensajeAConso, info);
		string_append(&mensajeAConso, ";");

		enviarMensaje(temporalN->socket_consola, mensajeAConso);

		free(mensajeAConso);

	} //TODO EN EL ELSE HAY QUE GRABAR EN UN ARCHIVO DEL FS. PARA ESO SE DEBE BUSCAR EN LA TABLA DE ARCHIVOS AL FD QUE SE RECIBIO EN EL MENSAJE
	  //Y CUANDO SE LO ENCUENTRA TOMAR EL NOMBRE DEL ARCHIVO. CON ESE NOMBRE IR AL FS Y GRABAR.
}

void abrirArchivo(int pid_mensaje, char* direccion, char flag)
{

}
void borrarArchivo(int pid_mensaje, char* direccion, char flag)
{
}
