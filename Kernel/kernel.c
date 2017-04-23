//CODIGOS
//100 KER A MEM - HANDSHAKE
//100 KER A FIL - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE DE CONSOLA
//102 KER A CPU - RESPUESTA HANDSHAKE DE CPU
//198 KER A CON - LIMITE GRADO MULTIPROGRAMACION
//199 KER A OTR - RESPUESTA A CONEXION INCORRECTA
//203 MEM A KER - ESPACIO SUFICIENTE PARA ALMACENAR PROGRAMA
//298 MER A KER - ESPACIO INSUFICIENTE PARA ALMACENAR PROGRAMA
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
#include "helperFunctions.h"

#define MAXCON 10
#define MAXCPU 10

int conexionesCPU = 0;
int conexionesConsola = 0;
Kernel_Config* configuracion;
int grado_multiprogramacion;
pthread_mutex_t mutex_grado_multiprog;
pthread_mutex_t mutex_numerador_pcb;
t_queue* cola_listos;
t_queue* cola_bloqueados;
t_queue* cola_ejecucion;
t_queue* cola_terminados;
t_queue* cola_cpu;
t_list* lista_variables_globales;
t_list* lista_semaforos;
int numerador_pcb = 1000;
int skt_memoria;
int skt_cpu;

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos. \n");
		return EXIT_FAILURE;
	}

	pthread_mutex_init(&mutex_grado_multiprog, NULL);
	pthread_mutex_init(&mutex_numerador_pcb, NULL);

	cola_cpu = crear_cola_pcb();
	pthread_t thread_id_filesystem;
	pthread_t thread_id_memoria;
	pthread_t thread_proceso_consola;
	pthread_t thread_proceso_cpu;
	pthread_t thread_consola_kernel;
	pthread_t thread_planificador;

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	grado_multiprogramacion = configuracion->grado_multiprogramacion;

	//inicializacion de variables globales y semaforos
	inicializar_variables_globales();
	inicializar_semaforos();

	cola_listos = crear_cola_pcb();
	cola_bloqueados = crear_cola_pcb();
	cola_ejecucion = crear_cola_pcb();
	cola_terminados = crear_cola_pcb();

	creoThread(&thread_id_filesystem, manejo_filesystem, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);
	creoThread(&thread_proceso_consola, hilo_conexiones_consola, NULL);
	creoThread(&thread_proceso_cpu, hilo_conexiones_cpu, NULL);
	creoThread(&thread_consola_kernel, inicializar_consola, NULL);
	creoThread(&thread_planificador, planificar, NULL);

	pthread_join(thread_id_filesystem, NULL);
	pthread_join(thread_id_memoria, NULL);
	pthread_join(thread_proceso_consola, NULL);
	pthread_join(thread_proceso_cpu, NULL);
	pthread_join(thread_consola_kernel, NULL);
	pthread_join(thread_planificador, NULL);

	liberar_estructuras();
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
		//char* mensaje = string_new();

		while (accion_correcta == 0){

			char * mensaje = string_new();
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

			free(mensaje);
		}
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

void liberar_estructuras(){
	//YA QUE HAY UN LIBERAR ESTRUCTURAS
	//PODRIAMOS ARMAR UN INICIALIZAR ESTRUCTURAS
	free(configuracion);
	free(lista_variables_globales);
	free(lista_semaforos);
	free(cola_cpu);
	free(cola_listos);
	free(cola_bloqueados);
	free(cola_ejecucion);
	free(cola_terminados);
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

	handShakeSend(&socketFS, "100", "401", "File System");

	//PRUEBO CON EL BLOQUEO EN VEZ DE LA ESPERA ACTIVA
	pause();

	/*
	//Loop para seguir comunicado con el servidor
	while (1) {
	}
	*/

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

	//PRUEBO CON EL BLOQUEO EN VEZ DE LA ESPERA ACTIVA
	pause();

	/*
	//Loop para seguir comunicado con el servidor
	while (1) {
	}
	*/

	shutdown(socketMemoria, 0);
	close(socketMemoria);
	return EXIT_SUCCESS;
}

void * hilo_conexiones_consola(void *args) {

	struct sockaddr_in direccionKernel;
	int master_socket , addrlen , new_socket , client_socket[MAXCON] , max_clients = MAXCON , activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;

	char buffer[1025];  //data buffer of 1K

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
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if(sd > 0){
				FD_SET( sd , &readfds);
			}

			//highest file descriptor number, need it for the select function
			if(sd > max_sd){
				max_sd = sd;
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
			sd = client_socket[i];

			if (FD_ISSET( sd , &readfds)){
				//Check if it was for closing , and also read the incoming message
				if ((valread = read( sd , buffer, 1024)) == 0){
					//Somebody disconnected
					puts("Se desconectó una consola");
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
				}else{
					//set the string terminating NULL byte on the end of the data read
					buffer[valread] = '\0';
					char** respuesta_a_kernel = string_split(buffer, ";");
					if(atoi(respuesta_a_kernel[0]) == 303){
						//validacion de nivel de multiprogramacion
						if((queue_size(cola_listos) + (queue_size(cola_bloqueados) + (queue_size(cola_ejecucion)))) == configuracion->grado_multiprogramacion){
							char message[MAXBUF];
							strcpy(message, "198;");
							enviarMensaje(&sd, message);
						}else{
							//Se crea programa nuevo
							t_pcb * new_pcb = nuevo_pcb(numerador_pcb, 0, NULL, NULL, &skt_cpu, 0);

							printf("%s", buffer);
							char message[MAXBUF];
							char* mensajeInicioPrograma = string_new();
							string_append(&mensajeInicioPrograma, string_itoa(new_pcb->pid));
							string_append(&mensajeInicioPrograma, ";");
							string_append(&mensajeInicioPrograma, respuesta_a_kernel[1]);
							enviarMensaje(&skt_memoria, mensajeInicioPrograma);

							recv(skt_memoria, message, sizeof(message), 0);
							//Recepcion de respuesta de la Memoria sobre validacion de espacio para almacenar script
							char** respuesta_Memoria = string_split(message, ";");

							if(atoi(respuesta_Memoria[0]) == 298){
								puts("Se rechaza programa por falta de espacio en memoria");

								//Decremento el numero de pid global del kernel
								pthread_mutex_lock(&mutex_numerador_pcb);
								numerador_pcb--;
								pthread_mutex_unlock(&mutex_numerador_pcb);

								char message2[MAXBUF];
								strcpy(message2, "197;");
								enviarMensaje(&sd, message2);
							}else{
								//Si hay espacio suficiente en la memoria
								//Agrego el programa a la cola de listos
								int indice_inicio = atoi(respuesta_Memoria[1]);
								int offset = atoi(respuesta_Memoria[2]);
								new_pcb->inicio_lectura_bloque = indice_inicio;
								new_pcb->offset = offset;

								queue_push(cola_listos, new_pcb);

								char* info_pid = string_new();
								char* respuestaAConsola = string_new();
								string_append(&info_pid, "103");
								string_append(&info_pid, ";");
								string_append(&info_pid, string_itoa(new_pcb->pid));
								string_append(&respuestaAConsola, info_pid);
								enviarMensaje(&sd, respuestaAConsola);

								free(info_pid);
								free(respuestaAConsola);

							}

							free(respuesta_Memoria);
							free(mensajeInicioPrograma);
						}
					}else if(atoi(respuesta_a_kernel[0]) == 398){
						//TODO - Leer el resto del mensaje donde se revise el pid del proceso. Buscarlo en las
						//colas de listos, bloqueados o en ejecucion y matarlo.
						puts("Se debe matar al proceso del mensaje - FALTA IMPLEMENTAR");
					}if(atoi(respuesta_a_kernel[0]) == 399){
						//TODO - Buscar en las colas de listos, bloqueadosa y en ejecucion a todos los programas
						//cuyo socket_consola sea igual al que envio este mensaje y matarlos.
						puts("Se debe matar a todos procesos de la consola que envio el mensaje - FALTA IMPLEMENTAR");
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
	string_append(&mensajeACPU, string_itoa(pcb->pos_stack));
	string_append(&mensajeACPU, ";");
	string_append(&mensajeACPU, string_itoa(*pcb->socket_cpu));
	string_append(&mensajeACPU, ";");
	string_append(&mensajeACPU, string_itoa(pcb->inicio_lectura_bloque));
	string_append(&mensajeACPU, ";");
	string_append(&mensajeACPU, string_itoa(pcb->offset));
	string_append(&mensajeACPU, ";");
	string_append(&mensajeACPU, string_itoa(pcb->exit_code));
	string_append(&mensajeACPU, ";");
	string_append(&mensajeACPU, string_itoa(configuracion->quantum));
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
		skt_cpu = socketClienteCPU;

		pthread_t thread_proceso_cpu;

		printf("%s:%d conectado\n", inet_ntoa(direccionCPU.sin_addr), ntohs(direccionCPU.sin_port));
		creoThread(&thread_proceso_cpu, handler_conexion_cpu, (void *) socketClienteCPU);

	}

	if (socketClienteCPU <= 0){
		printf("Error al aceptar conexiones de CPU\n");
		exit(errno);
	}

	//LLEGA ALGUNA VEZ A ESTO?
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

	queue_push(cola_cpu, &cpus);

	int * socketCliente = (int *) &sock;

	int result = recv(* socketCliente, message, sizeof(message), 0);

	while (result > 0) {
		printf("%s", message);

		result = recv(* socketCliente, message, sizeof(message), 0);
	}

	if (result <= 0) {
		int corte, i, encontrado;

		corte = queue_size(cola_cpu);
		i = 0;
		encontrado = 0;

		estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

		while (i <= corte && encontrado == 0){
			temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);

			if(temporalCpu->socket == *socketCliente){
				encontrado = 1;
			}else{
				queue_push(cola_cpu, temporalCpu);
				i++;
			}
		}
		printf("Se desconecto un CPU\n");
	}

	return EXIT_SUCCESS;
}

t_pcb *nuevo_pcb(int pid, int program_counter, int* tabla_arch, int pos_stack, int* socket_cpu, int exit_code){
	t_pcb* new = malloc(sizeof(t_pcb));
	new->pid = pid;
	new->program_counter = program_counter;
	new->tabla_archivos = tabla_arch;
	new->pos_stack = pos_stack;
	new->socket_cpu = socket_cpu;
	new->exit_code = exit_code;
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

void eliminar_pcb(t_pcb *self){
	free(self->tabla_archivos);
	free(self);
}

void switchear_colas(t_queue* origen, t_queue* fin, t_pcb* element){
	queue_pop(origen);
	queue_push(fin, element);
}

void planificar(){
	int corte, i, encontrado;

	while (1){
		usleep(configuracion->quantum_sleep);

		corte = queue_size(cola_cpu);
		i = 0;
		encontrado = 0;

		if(!(queue_is_empty(cola_cpu))){
			estruct_cpu* temporalCpu = malloc(sizeof(estruct_cpu));

			while (i <= corte && encontrado == 0){
				temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);

				if(temporalCpu->pid_asignado == -1){

					encontrado = 1;
				}else{
					i++;
				}
				queue_push(cola_cpu, temporalCpu);
			}

			if(encontrado == 1){
				if(!(queue_is_empty(cola_listos))){

					t_pcb* pcbtemporalListos = malloc(sizeof(t_pcb));
					pcbtemporalListos = (t_pcb*) queue_pop(cola_listos);

					temporalCpu->pid_asignado = pcbtemporalListos->pid;
					pcbtemporalListos->socket_cpu = &temporalCpu->socket;

					char* mensajeACPUPlan = serializar_pcb(pcbtemporalListos);
					enviarMensaje(pcbtemporalListos->socket_cpu, mensajeACPUPlan);
					queue_push(cola_ejecucion, pcbtemporalListos);
					free(mensajeACPUPlan);
				}
			}
		}
	}
}

