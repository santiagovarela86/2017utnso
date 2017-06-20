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
//700 CPU A KER - ELIMINAR MEMORIA HEAP
//705 KER A MEM - ELIMINAR MEMORIA HEAP
//706 MEM A KER - ELIMINAR MEMORIA HEAP OK
//710 KER A CPU - ELIMINAR MEMORIA HEAP OK
//605 KER A MEM - NUEVA PAGINA
//606 KER A CPU - NUEVA PAGINA OK
//612 KER A MEM - ENVIO DE CANT MAXIMA DE PAGINAS DE STACK POR PROCESO
//615 CPU A KER - NO SE PUEDEN ASIGNAR MAS PAGINAS A UN PROCESO
//616 KER A MEM - FINALIZAR PROGRAMA
//621 KER A CPU - ESPACIO INSUFICIENTE EN MEMORIA DE HEAP
//622 KER A CPU - ERROR AL LIBERAR MEMORIA DE HEAP INEXISTENTE
//777 CPU A KER - SUMO EN UNO LA CANTIDAD DE PAGINAS DE UN PCB
//778 CPU A KER - FINALIZAR PROGRAMA POR ERROR EN OBTENER VARIABLE
//617 MEM A KER - FINALIZAR PROGRAMA POR ERROR DE HEAP

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
pthread_mutex_t mtx_estadistica;
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
t_list* lista_estadistica;
int numerador_pcb = 1000;
int skt_memoria;
int skt_filesystem;
int plan;
t_list * offsetArch;
t_list * lista_paginas_heap;;
sem_t semaforoMemoria;
sem_t semaforoFileSystem;
sem_t sem_prog;
sem_t sem_cpus;
int programCounter;
int longitud_pag;

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

	char * pathConfig = argv[1];
	inicializarEstructuras(pathConfig);

	imprimirConfiguracion(configuracion);

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
	pthread_mutex_init(&mtx_estadistica, NULL);

	sem_init(&semaforoMemoria, 0, 0);
	sem_init(&semaforoFileSystem, 0, 0);
	sem_init(&sem_cpus, 0, 0);

	sem_init(&sem_prog, 0, 0);

	lista_paginas_heap = list_create();

	lista_semaforos = list_create();
	lista_variables_globales = list_create();
	lista_File_global = list_create();
	lista_File_proceso = list_create();
	lista_procesos = list_create();
	registro_bloqueados = list_create();
	lista_estadistica = list_create();
	offsetArch = list_create();

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

	list_destroy_and_destroy_elements(lista_paginas_heap, heapElementDestroyer);

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
	sem_destroy(&sem_cpus);

	sem_destroy(&sem_prog);
}

void * inicializar_consola(void* args){

	char* valorIngresado = string_new();

	while(1){
		puts("");
		puts("***********************************************************");
		puts("CONSOLA KERNEL - ACCIONES");
		puts("1)  Listado de procesos del sistema");
		puts("2)  Obtener dato de un proceso");
		puts("3)  Obtener tabla global de archivos");
		puts("4)  Modificar el grado de multiprogramacion del sistema");
		puts("5)  Finalizar Proceso");
		puts("6)  Detener la Planificacion");
		puts("***********************************************************");

		int accion = 0;
		int accion_correcta = 0;
		int nuevo_grado_multiprog = 0;
		int pid_buscado;

		while (accion_correcta == 0){

			char * mensaje = string_new();
			scanf("%s", valorIngresado);
			accion = atoi(valorIngresado);

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

				t_pcb* p = existe_proceso(pid_buscado);

				if(p != NULL){
					string_append(&mensaje, "Datos del Proceso: ");
					string_append(&mensaje, string_itoa(pid_buscado));
					log_console_in_disk(mensaje);

					abrir_subconsola_dos(p);
				}else{
					puts("No existe el proceso");
				}

				break;
			case 3:
				accion_correcta = 1;

				puts("Se busca la Tabla global de archivos");

				char* tablaGlobalArchivos = string_new();

				int j= 0;
				if(lista_File_global->elements_count != 0)
				{
					for (j = 0; j < lista_File_global->elements_count; j++){

						t_fileGlobal * elem = malloc(sizeof(elem));
						elem = list_get(lista_File_global, j);

						string_append(&tablaGlobalArchivos, "Canitdad de Aparturas:");
						string_append(&tablaGlobalArchivos, string_itoa(elem->cantidadDeAperturas));
						string_append(&tablaGlobalArchivos, ";");

						string_append(&tablaGlobalArchivos, "Indice de Tabla Global:");
						string_append(&tablaGlobalArchivos, string_itoa(elem->fdGlobal));
						string_append(&tablaGlobalArchivos, ";");

						string_append(&tablaGlobalArchivos, "Dirección del Archivo:");
						string_append(&tablaGlobalArchivos, elem->path);
						string_append(&tablaGlobalArchivos, ";");
					}
				}
				else
				{
					tablaGlobalArchivos = "No hay archivos creados en la Tabla Global de Archivos";
				}

				log_console_in_disk(tablaGlobalArchivos);
				puts("La tabla de archivos globales fue grabada en Log correctamente");
				break;
			case 4:
				accion_correcta = 1;
				printf("Ingrese nuevo grado de multiprogramacion: ");
				scanf("%d", &nuevo_grado_multiprog);

				int dif_gr_mult = nuevo_grado_multiprog - grado_multiprogramacion;

				if(dif_gr_mult > 0){
					int i = 0;
					for(;i < dif_gr_mult;){
						multiprogramar();
						i++;
					}
				}

				pthread_mutex_lock(&mutex_grado_multiprog);
				grado_multiprogramacion = nuevo_grado_multiprog;
				pthread_mutex_unlock(&mutex_grado_multiprog);

				string_append(&mensaje, "Ahora el grado de multiprogramacion es ");
				string_append(&mensaje, string_itoa(nuevo_grado_multiprog));
				log_console_in_disk(mensaje);
				break;
			case 5:
				accion_correcta = 1;
				printf("Ingrese PID del proceso: ");
				scanf("%d", &pid_buscado);
				matarProceso(pid_buscado);
				string_append(&mensaje, "Se finalizo el proceso con PID ");
				string_append(&mensaje, string_itoa(pid_buscado));
				log_console_in_disk(mensaje);
				break;
			case 6:
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
				puts("2)  Obtener dato de un proceso");
				puts("3)  Obtener tabla global de archivos");
				puts("4)  Modificar el grado de multiprogramacion del sistema");
				puts("5)  Finalizar Proceso");
				puts("6)  Detener la Planificacion");
				break;
			}

			free(mensaje);
		}
	}
	free(valorIngresado);
}

t_pcb* existe_proceso(int pid){
	int fin = queue_size(cola_terminados);
	int encontrado = 0;
	t_pcb* p;

	while(fin > 0 && encontrado == 0){
		pthread_mutex_lock(&mtx_terminados);
		p = queue_pop(cola_terminados);
		pthread_mutex_unlock(&mtx_terminados);

		if(p->pid == pid){
			encontrado = 1;
		}

		pthread_mutex_lock(&mtx_terminados);
		queue_push(cola_terminados, p);
		pthread_mutex_unlock(&mtx_terminados);

		fin--;
	}

	if(encontrado == 1){
		return p;
	}else{
		fin = queue_size(cola_listos);

		while(fin > 0 && encontrado == 0){
			pthread_mutex_lock(&mtx_listos);
			p = queue_pop(cola_listos);
			pthread_mutex_unlock(&mtx_listos);

			if(p->pid == pid){
				encontrado = 1;
			}

			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, p);
			pthread_mutex_unlock(&mtx_listos);

			fin--;
		}

		if(encontrado == 1){
			return p;
		}else{
			fin = queue_size(cola_bloqueados);

			while(fin > 0 && encontrado == 0){
				pthread_mutex_lock(&mtx_bloqueados);
				p = queue_pop(cola_bloqueados);
				pthread_mutex_unlock(&mtx_bloqueados);

				if(p->pid == pid){
					encontrado = 1;
				}

				pthread_mutex_lock(&mtx_bloqueados);
				queue_push(cola_bloqueados, p);
				pthread_mutex_unlock(&mtx_bloqueados);

				fin--;
			}

			if(encontrado == 1){
				return p;
			}else{
				fin = queue_size(cola_ejecucion);

				while(fin > 0 && encontrado == 0){
					pthread_mutex_lock(&mtx_ejecucion);
					p = queue_pop(cola_ejecucion);
					pthread_mutex_unlock(&mtx_ejecucion);

					if(p->pid == pid){
						encontrado = 1;
					}

					pthread_mutex_lock(&mtx_ejecucion);
					queue_push(cola_ejecucion, p);
					pthread_mutex_unlock(&mtx_ejecucion);

					fin--;
				}

				if(encontrado == 1){
					return p;
				}else{
					return NULL;
				}
			}
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

void abrir_subconsola_dos(t_pcb* p){


		puts("");
		puts("***********************************************************");
		puts("SUB CONSOLA KERNEL - DATOS DE UN PROCESO");
		puts("1)  Cantidad de rafagas ejecutadas");
		puts("2)  Cantidad de operaciones privilegiadas ejecutadas");
		puts("3)  Obtener tabla de archivos");
		puts("4)  Cantidad de paginas de heap utilizadas");
		puts("5)  Cantidad de acciones alocar realizadas");
		puts("6)  Cantidad de acciones liberar realizadas");
		puts("7)  Cantidad de syscall ejecutadas");
		puts("***********************************************************");

		int accion = 0;
		int accion_correcta = 0;
		t_estadistica* est;

		while (accion_correcta == 0){
			char * log = string_new();
			scanf("%d", &accion);

			switch(accion){
			case 1:
				puts("Procesando solicitud");
				accion_correcta = 1;
				printf("La cantidad de rafagas realizadas son: %d \n", p->program_counter + 1);
				puts("Solicitud procesada. Almacenada en Archivo Log");
				break;
			case 2:
				puts("Procesando solicitud");
				accion_correcta = 1;

				est = encontrar_estadistica(p->pid);
				printf("Grabando accion en Log");
				string_append(&log, "La cantidad de operaciones privilegiadas son: ");
				string_append(&log, string_itoa(est->cant_oper_privilegiadas));
				log_console_in_disk(log);
				puts("Solicitud procesada. Almacenada en Archivo Log");

				break;
			case 3:
				puts("Procesando solicitud");
				accion_correcta = 1;

				//ESTA OPCION ESTA FALLANDO
				t_lista_fileProcesos* tablaDeProcesoActual = malloc(sizeof(t_lista_fileProcesos));
				tablaDeProcesoActual = existeEnListaProcesosArchivos(p->pid);

				if(tablaDeProcesoActual == NULL){

					string_append(&log, "EL proceso no tiene archivos asociados");
					log_console_in_disk(log);
				}else{

					int size = tablaDeProcesoActual->tablaProceso->elements_count;
					int i = 0;

					if(size != 0)
					{
						string_append(&log, "El proceso tiene los siguientes Archivos: \n");
						t_fileProceso* filePro = malloc(sizeof(t_fileProceso));

						while (i < size){

							filePro = list_get(tablaDeProcesoActual->tablaProceso, i);
							//printf("FD: %d \n", filePro->fileDescriptor);
							string_append(&log, "FD: ");
							string_append(&log, string_itoa(filePro->fileDescriptor));
							string_append(&log, ";");

							i++;
						}
						log_console_in_disk(log);
						free(filePro);
					}
					else
					{
						string_append(&log, "EL proceso no tiene archivos asociados");
						log_console_in_disk(log);
					}
				}
				puts("Solicitud procesada. Almacenada en Archivo Log");
				break;
			case 4:
				puts("Procesando solicitud");
				accion_correcta = 1;

				int buscar_pid(admPaginaHeap* he){
					return (he->pid == p->pid);
				}

				t_list* list_aux = list_create();
				list_aux = list_filter(lista_paginas_heap, (void *) buscar_pid);

				if(list_aux == NULL){
					string_append(&log, "El proceso tiene 0 paginas de heap");
					log_console_in_disk(log);

				}else{
					string_append(&log, "El proceso tiene ");
					string_append(&log, string_itoa(list_aux->elements_count));
					string_append(&log, "paginas de heap");
					log_console_in_disk(log);

				}

				//list_destroy(list_aux);
				puts("Solicitud procesada. Almacenada en Archivo Log");
				break;
			case 5:
				puts("Procesando solicitud");
				accion_correcta = 1;

				est = encontrar_estadistica(p->pid);

				string_append(&log, "La cantidad de acciones alocar son: ");
				string_append(&log, string_itoa(est->cant_alocar));
				log_console_in_disk(log);
				puts("Solicitud procesada. Almacenada en Archivo Log");

				break;
			case 6:
				puts("Procesando solicitud");
				accion_correcta = 1;

				est = encontrar_estadistica(p->pid);

				string_append(&log, "La cantidad de acciones liberar son: ");
				string_append(&log, string_itoa(est->cant_liberar));
				log_console_in_disk(log);
				puts("Solicitud procesada. Almacenada en Archivo Log");

				break;
			case 7:
				puts("Procesando solicitud");
				accion_correcta = 1;

				est = encontrar_estadistica(p->pid);

				string_append(&log, "La cantidad Syscalls son: ");
				string_append(&log, string_itoa(est->cant_syscalls));
				log_console_in_disk(log);
				puts("Solicitud procesada. Almacenada en Archivo Log");

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
		t_pcb * temporalP;
		int largoColaListada;
/*de cola listos*/
		largoColaListada = queue_size(cola_listos);
		while(largoColaListada != 0){

			pthread_mutex_lock(&mtx_listos);
			temporalP = queue_pop(cola_listos);
			pthread_mutex_unlock(&mtx_listos);

			tempPid = temporalP->pid;
			/*si pid no es el pid a matar*/
			if(temporalP->pid != pidAMatar && tempPid > 0){

				pthread_mutex_lock(&mtx_listos);
				queue_push(cola_listos, temporalP);
				pthread_mutex_unlock(&mtx_listos);

			}else{

				sem_wait(&sem_prog);

				pthread_mutex_lock(&mtx_terminados);
				queue_push(cola_terminados, temporalP);

				int * sock = &temporalP->socket_consola;

				printf("Murio en Listos\n");
				printf("SOCKET DE CONSOLA: %d\n", * sock);

				char* msjAConsolaXEstadistica = string_new();
				string_append(&msjAConsolaXEstadistica, "666;");

				int pidDelMatado = temporalP->pid;
				string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

				enviarMensaje(&(temporalP->socket_consola), msjAConsolaXEstadistica);

				free(msjAConsolaXEstadistica);
				pthread_mutex_unlock(&mtx_terminados);

				multiprogramar();
			}

			largoColaListada--;
		}
		/*de cola bloqueados*/
		largoColaListada = queue_size(cola_bloqueados);
		while(largoColaListada != 0){

			pthread_mutex_lock(&mtx_bloqueados);
			temporalP = queue_pop(cola_bloqueados);
			pthread_mutex_unlock(&mtx_bloqueados);

			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){

				pthread_mutex_lock(&mtx_bloqueados);
				queue_push(cola_bloqueados, temporalP);
				pthread_mutex_unlock(&mtx_bloqueados);

			}else{

				pthread_mutex_lock(&mtx_terminados);
				queue_push(cola_terminados, temporalP);

				int * sock =  &temporalP->socket_consola;

				printf("Murio en Bloqueados\n");
				printf("SOCKET DE CONSOLA: %d\n",* sock);

				char* msjAConsolaXEstadistica = string_new();
				string_append(&msjAConsolaXEstadistica, "666;");

				int pidDelMatado = temporalP->pid;
				string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

				enviarMensaje(&(temporalP->socket_consola), msjAConsolaXEstadistica);

				free(msjAConsolaXEstadistica);
				pthread_mutex_unlock(&mtx_terminados);

				multiprogramar();
			}

			largoColaListada--;
		}
		/*de cola ejec*/
		largoColaListada = queue_size(cola_ejecucion);
		while(largoColaListada != 0){

			pthread_mutex_lock(&mtx_ejecucion);
			temporalP = queue_pop(cola_ejecucion);
			pthread_mutex_unlock(&mtx_ejecucion);

			tempPid = temporalP->pid;
			if(temporalP->pid != pidAMatar && tempPid > 0){

				pthread_mutex_lock(&mtx_ejecucion);
				queue_push(cola_ejecucion, temporalP);
				pthread_mutex_unlock(&mtx_ejecucion);

			}else{

				pthread_mutex_lock(&mtx_terminados);
				queue_push(cola_terminados, temporalP);

				int * sock =  &temporalP->socket_consola;

				printf("Murio en Ejec\n");
				printf("pid: %d\n", tempPid);

				char* msjAConsolaXEstadistica = string_new();
				string_append(&msjAConsolaXEstadistica, "666;");

				int pidDelMatado = temporalP->pid;
				string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

				enviarMensaje(&(temporalP->socket_consola), msjAConsolaXEstadistica);

				free(msjAConsolaXEstadistica);
				pthread_mutex_unlock(&mtx_terminados);

				multiprogramar();
			}

			largoColaListada--;
		}

}

t_estadistica* encontrar_estadistica(int pid){
	int inicio = 0;
	int fin = list_size(lista_estadistica);
	int encontrado = 0;
	t_estadistica* est;

	while(inicio < fin && encontrado == 0){
		est = (t_estadistica*) list_get(lista_estadistica, inicio);

		if(est->pid == pid){
			encontrado = 1;
		}

		inicio++;
	}

	return est;
}

void listarCola(t_queue * cola){

	t_pcb * temporalP;
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

	longitud_pag = handShakeSend(&socketMemoria, "100", "201", "Memoria");

	printf("El tamaño de pagina es %d \n", longitud_pag);
	puts("");

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

	string_append(&mensajeACPU, string_itoa(pcb->socket_consola));
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

		string_append(&mensajeACPU, string_itoa(sta->retPost));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->retVar));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->stack_pointer));
		string_append(&mensajeACPU, ";");
		int j;

		string_append(&mensajeACPU, string_itoa(sta->variables->elements_count)); //lo necesito para deserializar
		string_append(&mensajeACPU, ";");
		for (j = 0; j < sta->variables->elements_count; j++){

			t_variables* var = list_get(sta->variables, j);

			string_append(&mensajeACPU, string_itoa(var->offset));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->pagina));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->size));
			string_append(&mensajeACPU, ";");
			if (var->nombre_funcion != NULL){
				string_append(&mensajeACPU, string_itoa(var->nombre_funcion));
			}
			else {
				string_append(&mensajeACPU, string_itoa(CONST_SIN_NOMBRE_FUNCION));
			}
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->nombre_variable));
			string_append(&mensajeACPU, ";");
		}

		string_append(&mensajeACPU, string_itoa(sta->args->elements_count)); //lo necesito para deserializar
		string_append(&mensajeACPU, ";");
		int k;

		for (k = 0; k < sta->args->elements_count; k++){

			t_variables* args = list_get(sta->args, k);

			string_append(&mensajeACPU, string_itoa(args->offset));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->pagina));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->size));
			string_append(&mensajeACPU, ";");
			if (args->nombre_funcion != NULL){
				string_append(&mensajeACPU, string_itoa(args->nombre_funcion));
			}
			else {
				string_append(&mensajeACPU, string_itoa(CONST_SIN_NOMBRE_FUNCION));
			}
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->nombre_variable));
			string_append(&mensajeACPU, ";");

		}
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

	sem_post(&sem_cpus);

	int * socketCliente = (int *) &sock;

	int result = recv(* socketCliente, message, MAXBUF, 0);

	while (result > 0) {

		char**mensajeDesdeCPU = string_split(message, ";");
		int operacion = atoi(mensajeDesdeCPU[0]);
		int pid_mensaje;
		int fd;
		int tamanio;
		char* flag;
		char* direccion;
		char* infofile;
		int pid_recibido;

		int pid_del_programa = obtener_pid_de_cpu(socketCliente);

		t_estadistica* est = encontrar_estadistica(pid_del_programa);

		est->cant_oper_privilegiadas++;
		est->cant_syscalls++;

		switch (operacion){
			case 530:

				enviarMensaje(socketCliente, "530");
				finDeQuantum(socketCliente);
				break;

			case 531:

				finDePrograma(socketCliente);
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

			case 805: //mover cursor (offset)
				;
				//lista_posicionEnArchivos
				t_offsetArch* regOffset = malloc(sizeof(t_offsetArch));

				regOffset->fd = atoi(mensajeDesdeCPU[1]);
				regOffset->offset = atoi(mensajeDesdeCPU[2]);
				list_add(offsetArch,regOffset);

				//free(regOffset);

				break;


			case 804: //escribir archivo


			     fd = atoi(mensajeDesdeCPU[1]);
				 pid_mensaje = atoi(mensajeDesdeCPU[2]);
				 infofile = mensajeDesdeCPU[3];
				 tamanio = atoi(mensajeDesdeCPU[4]);

				escribirArchivo(pid_mensaje, fd, infofile, tamanio);

				break;

			case 803: //de CPU a File system (abrir)

				 pid_mensaje = atoi(mensajeDesdeCPU[1]);
				 direccion = mensajeDesdeCPU[2];
				 flag = mensajeDesdeCPU[3];
				 int fdNuevo = abrirArchivo(pid_mensaje, direccion, flag);
				// printf("el nuevo fddddd es %d", fdNuevo);
				enviarMensaje(socketCliente, string_itoa(fdNuevo));
				break;

			case 802:  //de CPU a File system (borrar)

			     pid_mensaje = atoi(mensajeDesdeCPU[1]);
				 fd = atoi(mensajeDesdeCPU[2]);
				 borrarArchivo(pid_mensaje, fd);

				break;

			case 801://de CPU a File system (cerrar)

				 fd = atoi(mensajeDesdeCPU[2]);
			     pid_mensaje = atoi(mensajeDesdeCPU[1]);
			     char * auxCerrar = malloc(MAXBUF);
			     auxCerrar = string_duplicate(cerrarArchivo(pid_mensaje, fd));

				 enviarMensaje(socketCliente, auxCerrar);

				break;

			case 800://de CPU a File system (leer)

				 fd = atoi(mensajeDesdeCPU[1]);
			     pid_mensaje = atoi(mensajeDesdeCPU[2]);
				 infofile = mensajeDesdeCPU[3];
				 tamanio = atoi(mensajeDesdeCPU[4]);
			     char * auxLeer = malloc(MAXBUF);

			     auxLeer = string_duplicate(leerArchivo(pid_mensaje, fd, infofile, tamanio));

				 enviarMensaje(socketCliente, auxLeer);


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
				reservarMemoriaHeap(pcb, bytes, socketCliente);
				break;

			case 700:
				;
				int otro_pid = atoi(mensajeDesdeCPU[1]);
				int direccion = atoi(mensajeDesdeCPU[2]);
				t_pcb * otro_pcb = pcbFromPid(otro_pid);
				eliminarMemoriaHeap(otro_pcb, direccion, socketCliente);
				break;

			case 615:
				;
				int pid_msg = atoi(mensajeDesdeCPU[1]);
				t_pcb * un_pcb = pcbFromPid(pid_msg);
				un_pcb->exit_code = FIN_ERROR_EXCEPCION_MEMORIA;
				finalizarPrograma(pid_msg);
				break;

			case 777:
				//SUMO EN UNO LA CANTIDAD DE PAGINAS
				;
				int pid_pcb = atoi(mensajeDesdeCPU[1]);
				un_pcb = pcbFromPid(pid_pcb);
				un_pcb->cantidadPaginas++;
				//CONFIRMACION DEL KERNEL PARA SINCRONIZAR
				enviarMensaje(socketCliente, serializarMensaje(1, 777));
				break;
			case 778:
				pid_msg = atoi(mensajeDesdeCPU[1]);
				un_pcb = pcbFromPid(pid_msg);
				un_pcb->exit_code = FIN_ERROR_SIN_DEFINICION;
				finalizarPrograma(pid_msg);
				break;

		}



		result = recv(* socketCliente, message, MAXBUF, 0);
		//printf("%s\n", message);
	}

	if (result <= 0) {
		int corte, i, encontrado;

		pthread_mutex_lock(&mtx_cpu);
		corte = queue_size(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		i = 0;
		encontrado = 0;

		estruct_cpu* temporalCpu;

		while (i <= corte && encontrado == 0){

			pthread_mutex_lock(&mtx_cpu);
			temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
			pthread_mutex_unlock(&mtx_cpu);

			if(temporalCpu->socket == *socketCliente){
				encontrado = 1;

				if(temporalCpu->pid_asignado == -1){
					sem_wait(&sem_cpus);
				}

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

int obtener_pid_de_cpu(int* skt){

	int fin = queue_size(cola_cpu);
	int encontrado = 0;
	estruct_cpu* cp;

	while(fin > 0 && encontrado == 0){

		pthread_mutex_lock(&mtx_cpu);
		cp = queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if(cp->socket == *(skt)){
			encontrado = 1;
		}

		pthread_mutex_lock(&mtx_cpu);
		queue_push(cola_cpu, cp);
		pthread_mutex_unlock(&mtx_cpu);

		fin--;
	}

	return cp->pid_asignado;

}

t_pcb *nuevo_pcb(int pid, int* socket_consola){
	t_pcb* new = malloc(sizeof(t_pcb));

	new->pid = pid;
	new->program_counter = programCounter;
	new->cantidadPaginas = 0;
	new->indiceCodigo = list_create();
	//new->indiceEtiquetas = list_create();
	new->indiceStack = list_create();
	t_Stack* sta = malloc(sizeof(t_Stack));
	sta->args = list_create();
	sta->variables = list_create();
	list_add(new->indiceStack, sta);
	new->inicio_codigo = 0;
	new->tabla_archivos = 0;
	new->pos_stack = 0;
	new->socket_consola = *socket_consola;
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

void logExitCode(int code) //ESTO NO SE ESTA USANDO
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

void * planificar() {
	int corte, i, encontrado;

	while (1) {
		sem_wait(&sem_cpus);

		sem_wait(&sem_prog);

		if (plan == 0) {
			usleep(configuracion->quantum_sleep);

			pthread_mutex_lock(&mtx_cpu);
			corte = queue_size(cola_cpu);
			pthread_mutex_unlock(&mtx_cpu);

			i = 0;
			encontrado = 0;

			if (!(queue_is_empty(cola_cpu))) {
				estruct_cpu* temporalCpu;

				while (i <= corte && encontrado == 0) {

					pthread_mutex_lock(&mtx_cpu);
					temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
					pthread_mutex_unlock(&mtx_cpu);

					if (temporalCpu->pid_asignado == -1) {
						encontrado = 1;
					} else {
						i++;
					}

					pthread_mutex_lock(&mtx_cpu);
					queue_push(cola_cpu, temporalCpu);
					pthread_mutex_unlock(&mtx_cpu);
				}

				if (encontrado == 1) {
					if (!(queue_is_empty(cola_listos))) {

						t_pcb* pcbtemporalListos;

						pthread_mutex_lock(&mtx_listos);
						pcbtemporalListos = (t_pcb*) queue_pop(cola_listos);
						pthread_mutex_unlock(&mtx_listos);

						temporalCpu->pid_asignado = pcbtemporalListos->pid;
						pcbtemporalListos->socket_cpu = &temporalCpu->socket;

						char* mensajeACPUPlan = serializar_pcb(pcbtemporalListos);

						enviarMensaje(pcbtemporalListos->socket_cpu,mensajeACPUPlan);

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
		//int i = 1;
		   while(curLine)
		   {
		      char * nextLine = strchr(curLine, '\n');
		      if (nextLine){
		    	  *nextLine = '\0';
		      }

		      if (!esComentario(curLine) && !esNewLine(curLine)){
		 	 	 if(string_contains(ltrim(curLine), "begin"))
		 		 	  {
		 	 		 	 //printf("el valor de la liena es %s", curLine);
		 		 		  //programCounter =i;
		 		 		  //puts("si entro");
		 		 		  //printf("con el valor %d \n", programCounter);
		 		 	  }

		    	  	 string_append(&codigoLimpio, ltrim(curLine));
		    	  	 string_append(&codigoLimpio, "\n");
		    	  	 //i++;

		    	  	// printf("%s \n", curLine);
		    	  	 //printf("%d \n", i);

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
		pcb->program_counter= (int)metadataProgram->instruccion_inicio;
		list_add(pcb->indiceCodigo, elem);
		//printf("el program es %d", pcb->program_counter);

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
	pcb->socket_consola = atoi(message[6]);
	pcb->exit_code = atoi(message[7]);
	cantIndiceCodigo = atoi(message[8]);
	pcb->etiquetas_size = atoi(message[9]);
	pcb->cantidadEtiquetas = atoi(message[10]);
	cantIndiceStack = atoi(message[11]);

	pcb->quantum = atoi(message[12]);

	int i = 13;

	while (i < 13 + cantIndiceCodigo * 2){
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



	while (cantIndiceStack > 0) {
		t_Stack* sta = malloc(sizeof(t_Stack));
		sta->variables = list_create();
	    sta->args = list_create();

		sta->retPost = atoi(message[i]);
		i++;

		sta->retVar = atoi(message[i]);
		i++;

		sta->stack_pointer = atoi(message[i]);
		i++;

		int cantVariables = atoi(message[i]);
		i++;

		while (cantVariables > 0) {
			t_variables* var = malloc(sizeof(t_variables));
			var->offset = atoi(message[i]);
			i++;
			var->pagina = atoi(message[i]);
			i++;
			var->size = atoi(message[i]);
			i++;
			int nro_funcion = atoi(message[i]);
			if (nro_funcion != CONST_SIN_NOMBRE_FUNCION) {
				char nombre_funcion = nro_funcion;
				var->nombre_funcion = nombre_funcion;
				i++;
			}
			else {
				i++;
			}
			char nombre_variable = atoi(message[i]);
			var->nombre_variable = nombre_variable;

			list_add(sta->variables, var);
			i++;
			cantVariables--;
		}

		int cantArgumentos = atoi(message[i]);
		i++;
		while (cantArgumentos > 0) {
			t_variables* argus = malloc(sizeof(t_variables));

			argus->offset = atoi(message[i]);
			i++;
			argus->pagina = atoi(message[i]);
			i++;
			argus->size = atoi(message[i]);
			i++;

			int nro_funcion = atoi(message[i]);
			if (nro_funcion != CONST_SIN_NOMBRE_FUNCION) {
				char nombre_funcion = nro_funcion;
				argus->nombre_funcion = nombre_funcion;
				i++;
			}
			else {
				i++;
			}
			char nombre_variable = atoi(message[i]);
			argus->nombre_variable = nombre_variable;

			list_add(sta->args, argus);
			i++;
			cantArgumentos--;
		}

		cantIndiceStack--;
		list_add(pcb->indiceStack, sta);
	 }
	return pcb;
}

void multiprogramar() {
	//validacion de nivel de multiprogramacion
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


char* serializar_codigo_por_instrucciones(char* codigo){
	char* codigo_serializado = string_new();

	t_metadata_program * metadataProgram = metadata_desde_literal(codigo);

	int i;
	for (i = 0; i < metadataProgram->instrucciones_size; i++){
		elementoIndiceCodigo * elem = malloc(sizeof(elementoIndiceCodigo));
		elem->start = metadataProgram->instrucciones_serializado[i].start;
		elem->offset = metadataProgram->instrucciones_serializado[i].offset;
		char* instruccion = string_substring(codigo, elem->start, elem->offset);
		string_append(&codigo_serializado, instruccion);
		string_append(&codigo_serializado, ";");
		free(elem);
		free(instruccion);
	}

	free(metadataProgram);

	return codigo_serializado;
}

void envioProgramaAMemoria(t_pcb * new_pcb, t_nuevo * nue){
	char * mensajeInicioPrograma = string_new();
	string_append(&mensajeInicioPrograma, "250");
	string_append(&mensajeInicioPrograma, ";");
	string_append(&mensajeInicioPrograma, string_itoa(new_pcb->pid));
	string_append(&mensajeInicioPrograma, ";");
	string_append(&mensajeInicioPrograma, nue->codigo);
	enviarMensaje(&skt_memoria, mensajeInicioPrograma);

	char message[MAXBUF];

	int result = recv(skt_memoria, message, MAXBUF, 0);

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

	t_estadistica* est = malloc(sizeof(t_estadistica));
	est->pid = new_pcb->pid;
	est->cant_alocar = 0;
	est->cant_liberar = 0;
	est->cant_oper_privilegiadas = 0;
	est->cant_syscalls = 0;
	est->tama_alocar = 0;
	est->tama_liberar = 0;

	pthread_mutex_lock(&mtx_estadistica);
	list_add(lista_estadistica, est);
	pthread_mutex_unlock(&mtx_estadistica);

	pthread_mutex_lock(&mtx_listos);
	queue_push(cola_listos, new_pcb);
	list_add(lista_procesos, new_pcb);
	pthread_mutex_unlock(&mtx_listos);

	sem_post(&sem_prog);

}

void informoAConsola(int socketConsola, int pid){
	// INFORMO A CONSOLA EL RESULTADO DE LA CREACION DEL PROCESO

	char* info_pid = string_new();
	char* respuestaAConsola = string_new();
	string_append(&info_pid, "103");
	string_append(&info_pid, ";");
	string_append(&info_pid, string_itoa(pid));
	string_append(&respuestaAConsola, info_pid);
	enviarMensaje(&socketConsola, respuestaAConsola);

	free(info_pid);
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

void finalizarProgramaEnMemoria(int pid){
	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "616");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pid));
	string_append(&mensajeAMemoria, ";");
	enviarMensaje(&skt_memoria, mensajeAMemoria);

	free(mensajeAMemoria);
}

int calcularEspacioLibrePaginaHeap(admPaginaHeap * elem){
	int result = 0;

	_Bool esMetadataFree(admMetadata * elem){
		return elem->free == true;
	}

	t_list * metadatasFree = list_filter(elem->metadatas, (void *) esMetadataFree);

	int i = 0;
	while (i < list_size(metadatasFree)){
		admMetadata * meta = list_get(metadatasFree, i);//ME JODE LA VIDA
		result = result + meta->size;
		i++;
	}

	return result;
}

void reservarMemoriaHeap(t_pcb * pcb, int bytes, int * socketCPU){

	int capacidadMaxima = longitud_pag - 2 * sizeof(heapMetadata);
	if (bytes > capacidadMaxima){
		printf("Error al reservar memoria Heap, excede el tamaño de página, se finaliza programa\n");
		finalizarPrograma(pcb->pid);
	}else{

		_Bool coincideHeapPID(admPaginaHeap * elem){
			return elem->pid == pcb->pid;
		}

		_Bool tieneLugarBloque(admMetadata * elem){
			return elem->free && (elem->size == bytes || (elem->size >= bytes + sizeof(heapMetadata)));
		}

		_Bool hayUnBloqueConEspacio(admPaginaHeap * elem){
			return list_any_satisfy(elem->metadatas, (void *) tieneLugarBloque);
		}

		_Bool hayEspacioEnDosBloques(admPaginaHeap * elem){
			_Bool result = false;

			if (list_size(elem->metadatas) >= 2){

				int i = 0;

				while (i < list_size(elem->metadatas) - 1){
					admMetadata * elem1 = list_get(elem->metadatas, i);
					admMetadata * elem2 = list_get(elem->metadatas, i + 1);

					if (elem1->free
							&& elem2->free
								&& (elem1->size + elem2->size) > bytes){
						result = true;
					}

					i++;
				}
			}

			return result;
		}

		//Verifico si todavía no hay paginas de Heap para este proceso
		if(!(list_any_satisfy(lista_paginas_heap, (void *) coincideHeapPID))){
			//Si no hay ninguna pagina, creo la primer pagina de Heap para ese Proceso
			pedirPaginaHeapNueva(pcb, bytes, socketCPU);
		}else{
			//Si hay paginas para el proceso, las filtro
			t_list * paginasHeapProceso = list_filter(lista_paginas_heap, (void *) coincideHeapPID);

			//Y busco si alguna satisface el request con un solo bloque
			if (list_any_satisfy(paginasHeapProceso, (void *) hayUnBloqueConEspacio)){
				admPaginaHeap * paginaConLugar = list_find(paginasHeapProceso, (void *) hayUnBloqueConEspacio);
				admMetadata * metaAUsar = list_find(paginaConLugar->metadatas, (void *) tieneLugarBloque);

				int espacioLibre = calcularEspacioLibrePaginaHeap(paginaConLugar);

				printf("Encontre la siguiente pagina de Heap Libre, PID: %d, PAGINA: %d, Bytes Libres: %d\n", paginaConLugar->pid, paginaConLugar->nro_pagina, espacioLibre);
				enviarMensaje(&skt_memoria, serializarMensaje(5, 607, paginaConLugar->pid, paginaConLugar->nro_pagina, metaAUsar->direccion, bytes));

				char * buffer = malloc(MAXBUF);
				int result = recv(skt_memoria, buffer, MAXBUF, 0);

				if (result > 0) {
					char ** respuesta = string_split(buffer, ";");

					if (strcmp(respuesta[0], "608") == 0){

						//SI ENTRA JUSTO
						if (metaAUsar->size == bytes){
							metaAUsar->free = false;
						}else{
							//SI QUEDA ESPACIO LIBRE INCLUYENDO EL QUE OCUPA EL META FREE
							admMetadata * nuevoMetadata = malloc(sizeof(admMetadata));
							nuevoMetadata->free = true;
							nuevoMetadata->size = metaAUsar->size - bytes - sizeof(heapMetadata);
							nuevoMetadata->direccion = metaAUsar->direccion + sizeof(heapMetadata) + bytes;

							metaAUsar->free = false;
							metaAUsar->size = bytes;

							//LO AGREGO A LA LISTA
							list_add(paginaConLugar->metadatas, nuevoMetadata);

							_Bool menorAMayorDireccion(admMetadata * elem1, admMetadata * elem2){
								return elem1->direccion < elem2->direccion;
							}

							//ORDENO LA LISTA POR DIRECCION
							list_sort(paginaConLugar->metadatas, (void *) menorAMayorDireccion);
						}

						enviarMensaje(socketCPU, serializarMensaje(2, 606, metaAUsar->direccion + sizeof(heapMetadata)));
					} else {
						printf("Error en el protocolo de mensajes entre procesos\n");
						exit(errno);
					}
				} else {
					printf("Error de comunicacion con Memoria durante la reserva de memoria heap existente\n");
					exit(errno);
				}
			}else{
				//SI NO ENCUENTRO NINGUN BLOQUE LIBRE QUE CONTENGA LUGAR
				//BUSCO SI HAY DOS BLOQUES QUE CONTENGAN LUGAR UNO JUNTO AL OTRO
				if (list_any_satisfy(paginasHeapProceso, (void *) hayEspacioEnDosBloques)){
					//UNIFICAR DOS BLOQUES Y ACTUALIZAR TABLAS, DESARROLLAR
				}else{
					//Si no puedo alocar en un solo bloque o en dos bloques, pido una pagina nueva
					pedirPaginaHeapNueva(pcb, bytes, socketCPU);
				}
			}
		}
	}
}
/*
	_Bool coincideHeapPID(admPaginaHeap * elem){
		return elem->pid == pcb->pid;
	}

	_Bool hayLugarHeap(admPaginaHeap * elem){
		return elem->tamanio_disponible >= bytes + sizeof(heapMetadata);
	}

	_Bool hayLugarHeapMismoPID(admPaginaHeap * elem){
		return hayLugarHeap(elem) && coincideHeapPID(elem);
	}

	//Verifico si todavía no hay paginas de Heap para este proceso
	if(!(list_any_satisfy(lista_paginas_heap, (void *) coincideHeapPID))){
		//Si no hay ninguna pagina, creo la primer pagina de Heap para ese Proceso
		pedirPaginaHeapNueva(pcb, bytes, socketCPU);
	}else {

		//SI EXISTEN PAGINAS HEAP PARA EL PROCESO ME FIJO QUE HAYA LUGAR
		if (list_any_satisfy(lista_paginas_heap, (void *) hayLugarHeapMismoPID)){
			//SI YA EXISTE UNA PAGINA DE HEAP PARA ESTE PROCESO ME TRAIGO LA PRIMERA QUE TENGA ESPACIO SUFICIENTE
			admPaginaHeap * paginaHeapLibre = list_find(lista_paginas_heap, (void *) hayLugarHeapMismoPID);

			//SI LA PAGINA NO FUE MANOSEADA
			if (!paginaHeapLibre->manoseada){

				//ENVIO A MEMORIA LA SOLICITUD DE GRABAR EN PAGINA HEAP EXISTENTE
				printf("Encontre la siguiente pagina de Heap Libre, PID: %d, PAGINA: %d, Bytes Libres: %d\n", paginaHeapLibre->pid, paginaHeapLibre->nro_pagina, paginaHeapLibre->tamanio_disponible);
				enviarMensaje(&skt_memoria, serializarMensaje(4, 607, paginaHeapLibre->pid, paginaHeapLibre->nro_pagina, bytes));

				char * buffer = malloc(MAXBUF);
				int result = recv(skt_memoria, buffer, MAXBUF, 0);

				if (result > 0) {
					char ** respuesta = string_split(buffer, ";");

					if (strcmp(respuesta[0], "608") == 0 || strcmp(respuesta[0], "605") == 0) {
						int direccion = atoi(respuesta[1]);
						int newFreeSpace = atoi(respuesta[2]);

						//EDITO LA ENTRADA EXISTENTE
						paginaHeapLibre->tamanio_disponible = newFreeSpace;

						admReservaHeap * reserva = malloc(sizeof(admReservaHeap));
				 		reserva->direccion = direccion;
						reserva->size = bytes;
						reserva->free = false;
						list_add(paginaHeapLibre->alocaciones, reserva);

				 		enviarMensaje(socketCPU, serializarMensaje(2, 606, direccion));
					}else{
						printf("Error en el protocolo de mensajes entre procesos\n");
						exit(errno);
					}
				} else {
					printf("Error de comunicacion con Memoria durante la reserva de memoria heap existente\n");
					exit(errno);
				}

			}else{
				//SI LA PAGINA FUE MANOSEADA

			}
		}else{
			//SI NO HAY LUGAR PIDO PAGINA NUEVA
			pedirPaginaHeapNueva(pcb, bytes, socketCPU);
		}
	}
}
*/

/*
_Bool hayLugarHeap (admPaginaHeap * elem){
	return elem->tamanio_disponible >= bytes;
}
*/

/*
_Bool hayLugarContiguo(admPaginaHeap * elem){
	int i = 0;
	_Bool hayLugar = false;

	if (list_size(elem->alocaciones) >= 2){
		while (i < list_size(elem->alocaciones) - 1 && !hayLugar){
			admReservaHeap * primeraReserva = list_get(elem->alocaciones, i);
			admReservaHeap * segundaReserva = list_get(elem->alocaciones, i+1);

			if (primeraReserva->free && segundaReserva->free &&
					(primeraReserva->size + segundaReserva->size) >= bytes){
				hayLugar = true;
			}

			i++;
		}
	}

	return hayLugar;
}
*/


/*
_Bool hayLugarHeapMismoPIDYContiguo(admPaginaHeap * elem){
	return hayLugarHeap(elem) && coincideHeapPID(elem) && hayLugarContiguo(elem);
}
*/
/*
 *
 *
 *
 *
		if (list_any_satisfy(lista_paginas_heap, (void *) hayLugarHeapMismoPID)){
			//SI HAY LUGAR EN LAS PAGINAS QUE YA TENGO Y EL LUGAR NO ES CONTIGUO
			//OBTENGO LA PRIMER PAGINA QUE ENCUENTRO CON ESPACIO SUFICIENTE
			admPaginaHeap * paginaHeapLibre = list_find(lista_paginas_heap, (void *) hayLugarHeapMismoPID);

			//ENVIO A MEMORIA LA SOLICITUD DE GRABAR EN PAGINA HEAP EXISTENTE
			printf("Encontre la siguiente pagina de Heap Libre, PID: %d, PAGINA: %d, Bytes Libres: %d\n", paginaHeapLibre->pid, paginaHeapLibre->nro_pagina, paginaHeapLibre->tamanio_disponible);
			enviarMensaje(&skt_memoria, serializarMensaje(4, 607, paginaHeapLibre->pid, paginaHeapLibre->nro_pagina, bytes));

			char * buffer = malloc(MAXBUF);
			int result = recv(skt_memoria, buffer, MAXBUF, 0);

			if (result > 0) {
				char ** respuesta = string_split(buffer, ";");

				if (strcmp(respuesta[0], "608") == 0 || strcmp(respuesta[0], "605") == 0) {
					int direccion = atoi(respuesta[1]);
					int newFreeSpace = atoi(respuesta[2]);

					//EDITO LA ENTRADA EXISTENTE
					paginaHeapLibre->tamanio_disponible = newFreeSpace;

					admReservaHeap * reserva = malloc(sizeof(admReservaHeap));
 					reserva->direccion = direccion;
					reserva->size = bytes;
					reserva->free = false;
					list_add(paginaHeapLibre->alocaciones, reserva);

 					enviarMensaje(socketCPU, serializarMensaje(2, 606, direccion));
				}else{
					printf("Error en el protocolo de mensajes entre procesos\n");
					exit(errno);
				}
			} else {
				printf("Error de comunicacion con Memoria durante la reserva de memoria heap existente\n");
				exit(errno);
			}
		}else{
			if (list_any_satisfy(lista_paginas_heap, (void *) hayLugarHeapMismoPIDYContiguo)){
				//HAGO LO QUE TENGA QUE HACER PARA UNIFICAR LOS DOS BLOQUES DE ESPACIO LIBRE CONTIGUO
				printf("Unificar los dos bloques\n");
			}else{
				//TENGO QUE PEDIR UNA PAGINA NUEVA
				pedirPaginaHeapNueva(pcb, bytes, socketCPU);
			}
		}
	}
	*/

void pedirPaginaHeapNueva(t_pcb * pcb, int bytes, int * socketCPU) {
	//Si no hay ninguna pagina, creo la primer pagina de Heap para ese Proceso
	int paginaActual = pcb->cantidadPaginas;

	enviarMensaje(&skt_memoria, serializarMensaje(4, 605, pcb->pid, paginaActual, bytes));

	char * buffer = malloc(MAXBUF);
	int result = recv(skt_memoria, buffer, MAXBUF, 0);

	if (result > 0) {
		char ** respuestaDeMemoria = string_split(buffer, ";");

		if (strcmp(respuestaDeMemoria[0], "605") == 0) {

			int puntero = atoi(respuestaDeMemoria[1]);
			int freeSpace = atoi(respuestaDeMemoria[2]);

			admPaginaHeap * heapElem = malloc(sizeof(admPaginaHeap));
			heapElem->metadatas = list_create();
			heapElem->pid = pcb->pid;
			heapElem->nro_pagina = paginaActual;

			admMetadata * meta_used = malloc(sizeof(admMetadata));
			admMetadata * meta_free = malloc(sizeof(admMetadata));

			meta_used->direccion = puntero - sizeof(heapMetadata);
			meta_used->free = false;
			meta_used->size = bytes;

			meta_free->direccion = puntero + bytes;
			meta_free->free = true;
			meta_free->size = freeSpace;

			list_add(heapElem->metadatas, meta_used);
			list_add(heapElem->metadatas, meta_free);


			list_add(lista_paginas_heap, heapElem);

			pcb->cantidadPaginas++;

			enviarMensaje(socketCPU, serializarMensaje(2, 606, puntero));

		} else {
			printf("Error en el protocolo de mensajes entre Kernel y Memoria al reserver memoria heap\n");
			exit(errno);
		}
	} else {
		printf("Error de comunicacion con Memoria durante la reserva de nueva pagina heap\n");
		exit(errno);
	}
}

void eliminarMemoriaHeap(t_pcb * pcb, int direccion, int * socketCliente){

	printf("Intento eliminar Pagina Heap de PID: %d, Direccion: %d", pcb->pid, direccion);
	printf("\n");

	_Bool coincideDireccion(admMetadata * elem){
		return elem->direccion == direccion - sizeof(heapMetadata);
	}

	_Bool coincideHeapPIDyDireccion(admPaginaHeap * elem){
		return elem->pid == pcb->pid && list_any_satisfy(elem->metadatas, (void *) coincideDireccion);
	}

	//Verifico que exista una pagina que tenga esa direccion y sea para ese proceso
	if(list_any_satisfy(lista_paginas_heap, (void *) coincideHeapPIDyDireccion)){

		printf("Existe la reserva heap con direccion %d y PID %d\n", direccion, pcb->pid);

		enviarMensaje(&skt_memoria, serializarMensaje(3, 705, pcb->pid, direccion));

		char * buffer = malloc(MAXBUF);

		int result = recv(skt_memoria, buffer, MAXBUF, 0);

		if (result > 0){
			char ** respuesta = string_split(buffer, ";");

			if (strcmp(respuesta[0], "706") == 0){

				admPaginaHeap * paginaHeapAEditar = list_find(lista_paginas_heap, (void *) coincideHeapPIDyDireccion);
				list_remove_by_condition(paginaHeapAEditar->metadatas, (void *) coincideDireccion);

				int tamanio_disponible = calcularEspacioLibrePaginaHeap(paginaHeapAEditar);

				printf("Se libera el elemento de heap en la Direccion: %d, PID: %d\n", direccion, pcb->pid);
				printf("El nuevo espacio libre de la pagina es %d\n", tamanio_disponible);

				enviarMensaje(socketCliente, serializarMensaje(1, 710));
			}else{
				printf("Error de protocolo al liberar memoria de heap\n");
				exit(errno);
			}
		}else{
			printf("Error de comunicación con memoria eliminando memoria de Heap\n");
			exit(errno);
		}
	}else{
		printf("Se intento eliminar memoria en heap no reservada previamente\n");
		finalizarPrograma(pcb->pid);
		//SE NOTIFICA AL CPU
		enviarMensaje(socketCliente, serializarMensaje(1, 622));
	}
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
		printf("SOCKET DE CONSOLA: %d\n",socketCliente);
		t_pcb * new_pcb = nuevo_pcb(pid, &socketCliente);

		char * mensajeInicioPrograma = string_new();
		char * codigoLimpio = limpioCodigo(codigo);
		new_pcb->program_counter = programCounter;
		string_append(&mensajeInicioPrograma, "250");
		string_append(&mensajeInicioPrograma, ";");
		string_append(&mensajeInicioPrograma, string_itoa(new_pcb->pid));
		string_append(&mensajeInicioPrograma, ";");
		string_append(&mensajeInicioPrograma, codigoLimpio);
		string_append(&mensajeInicioPrograma, ";");
		enviarMensaje(&skt_memoria, mensajeInicioPrograma);

		char message[MAXBUF];
		recv(skt_memoria, message, MAXBUF, 0);

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

			t_estadistica* est = malloc(sizeof(t_estadistica));
			est->pid = new_pcb->pid;
			est->cant_alocar = 0;
			est->cant_liberar = 0;
			est->cant_oper_privilegiadas = 0;
			est->cant_syscalls = 0;
			est->tama_alocar = 0;
			est->tama_liberar = 0;

			pthread_mutex_lock(&mtx_estadistica);
			list_add(lista_estadistica, est);
			pthread_mutex_unlock(&mtx_estadistica);

			pthread_mutex_lock(&mtx_listos);
			queue_push(cola_listos, new_pcb);
			list_add(lista_procesos, new_pcb);
			pthread_mutex_unlock(&mtx_listos);

			sem_post(&sem_prog);

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

			int * sock =  &temporalN->socket_consola;
			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");
			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));
			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);
			free(msjAConsolaXEstadistica);


			pthread_mutex_unlock(&mtx_terminados);
			encontre = 1;

			sem_wait(&sem_prog);
			multiprogramar();

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

			int * sock =  &temporalN->socket_consola;
			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");
			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));
			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);
			free(msjAConsolaXEstadistica);

			pthread_mutex_unlock(&mtx_terminados);
			encontreBloq = 1;

			multiprogramar();

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

			int * sock =  &temporalN->socket_consola;
			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");
			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));
			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);
			free(msjAConsolaXEstadistica);

			pthread_mutex_unlock(&mtx_terminados);
			encontreEjec = 1;

			multiprogramar();

			printf("Se termino el proceso: %d\n", temporalN->pid);

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, temporalN);
			pthread_mutex_unlock(&mtx_ejecucion);
			printf("Se intento cerrar un programa que no existe\n");

		}
	}

	finalizarProgramaEnMemoria(pidACerrar);
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

		printf("El skt es %d \n", temporalN->socket_consola);
		printf("El skt buscado es %d \n", socketCliente);
		pthread_mutex_unlock(&mtx_listos);

		largoColaListos--;

		if (temporalN->socket_consola == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);

			int * sock =  &temporalN->socket_consola;

			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");

			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);

			free(msjAConsolaXEstadistica);

			pthread_mutex_unlock(&mtx_terminados);

			sem_wait(&sem_prog);

			multiprogramar();

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

		if (temporalN->socket_consola == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);

			int * sock =  &temporalN->socket_consola;
			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");

			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);

			free(msjAConsolaXEstadistica);

			pthread_mutex_unlock(&mtx_terminados);

			multiprogramar();
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

		if (temporalN->socket_consola == socketCliente) {
			pthread_mutex_lock(&mtx_terminados);
			queue_push(cola_terminados, temporalN);

			int * sock =  &temporalN->socket_consola;
			char* msjAConsolaXEstadistica = string_new();
			string_append(&msjAConsolaXEstadistica, "666;");

			int pidDelMatado = temporalN->pid;
			string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));

			enviarMensaje(&(temporalN->socket_consola), msjAConsolaXEstadistica);

			free(msjAConsolaXEstadistica);

			pthread_mutex_unlock(&mtx_terminados);

			multiprogramar();

		} else {
			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, temporalN);
			pthread_mutex_unlock(&mtx_ejecucion);
		}
	}
}

void finDeQuantum(int * socketCliente){
	char message[MAXBUF];
	recv(* socketCliente, &message, MAXBUF, 0);

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

			sem_post(&sem_prog);

			encontrado = 1;

		} else {

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, pcb_a_cambiar);
			pthread_mutex_unlock(&mtx_ejecucion);

		}

		iter++;
	}

	encontrado = 0;

	estruct_cpu* temporalCpu;

	while (encontrado == 0) { //Libero la CPU que estaba ejecutando al programa

		pthread_mutex_lock(&mtx_cpu);
		temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if (temporalCpu->pid_asignado == pid_a_buscar) {
			encontrado = 1;
			temporalCpu->pid_asignado = -1;
		}

		sem_post(&sem_cpus);


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
		puts("No hay programas ejecutando");
	}

	while(fin > 0){
		pthread_mutex_lock(&mtx_ejecucion);
		t_pcb* aux = queue_pop(cola_ejecucion);
		pthread_mutex_unlock(&mtx_ejecucion);

		puts("Programas Ejecutando:");
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

	estruct_cpu* temporalCpu;

	while (encontrado == 0) { //Libero la CPU que estaba ejecutando al programa

		pthread_mutex_lock(&mtx_cpu);
		temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
		pthread_mutex_unlock(&mtx_cpu);

		if (temporalCpu->pid_asignado == pid_a_buscar) {
			encontrado = 1;
			temporalCpu->pid_asignado = -1;
		}

		sem_post(&sem_cpus);


		pthread_mutex_lock(&mtx_cpu);
		queue_push(cola_cpu, temporalCpu);
		pthread_mutex_unlock(&mtx_cpu);
	}

}

void finDePrograma(int * socketCliente) {
	char message[MAXBUF];

	enviarMensaje(socketCliente, "531;");

	int result = recv(*socketCliente, &message, MAXBUF, 0);

	if (result > 0) {
		t_pcb* pcb_deserializado = malloc(sizeof(t_pcb));
		pcb_deserializado = deserializar_pcb(message);

		int encontrado = 0;

		int size = queue_size(cola_ejecucion);

		int iter = 0;

		t_pcb* pcb_a_cambiar;

		while (encontrado == 0 && iter < size) {

			pthread_mutex_lock(&mtx_ejecucion);
			pcb_a_cambiar = queue_pop(cola_ejecucion);
			pthread_mutex_unlock(&mtx_ejecucion);

			if (pcb_a_cambiar->pid == pcb_deserializado->pid) {

				encontrado = 1;

				pcb_a_cambiar->program_counter =
				pcb_deserializado->program_counter;

				pthread_mutex_lock(&mtx_terminados);
				queue_push(cola_terminados, pcb_a_cambiar);
				finalizarProgramaEnMemoria(pcb_deserializado->pid);

				int * sock =  &pcb_a_cambiar->socket_consola;
				char* msjAConsolaXEstadistica = string_new();
				string_append(&msjAConsolaXEstadistica, "666;");

				int pidDelMatado = pcb_a_cambiar->pid;
				string_append(&msjAConsolaXEstadistica, string_itoa(pidDelMatado));
				enviarMensaje(&(pcb_a_cambiar->socket_consola), msjAConsolaXEstadistica);

				free(msjAConsolaXEstadistica);

				pthread_mutex_unlock(&mtx_terminados);

				multiprogramar();

			} else {

				pthread_mutex_lock(&mtx_ejecucion);
				queue_push(cola_ejecucion, pcb_a_cambiar);
				pthread_mutex_unlock(&mtx_ejecucion);

			}

			int c = 0;

			while (c < queue_size(cola_cpu)) {
				estruct_cpu* temporalCpu;

				pthread_mutex_lock(&mtx_cpu);
				temporalCpu = (estruct_cpu*) queue_pop(cola_cpu);
				pthread_mutex_unlock(&mtx_cpu);

				if (temporalCpu->pid_asignado == pcb_deserializado->pid) {
					encontrado = 1;
					temporalCpu->pid_asignado = -1;
				}

				sem_post(&sem_cpus);


				pthread_mutex_lock(&mtx_cpu);
				queue_push(cola_cpu, temporalCpu);
				pthread_mutex_unlock(&mtx_cpu);

				c++;
			}

		}
	} else {
		printf("Error de comunicacion de fin de programa con el CPU\n");
		exit(errno);
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

			sem_post(&sem_prog);

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

t_fileGlobal* traducirFDaPath(int pid_mensaje, int fd)
{
	t_lista_fileProcesos* listaDeArchivosDelProceso = malloc(sizeof(listaDeArchivosDelProceso));
	listaDeArchivosDelProceso = existeEnListaProcesosArchivos(pid_mensaje);

		int encontrar_archProceso(t_fileProceso* glo){
			if(fd == glo->fileDescriptor)
				return 1;
			else
				return 0;
		}
		t_fileProceso* regTablaProceso = malloc(sizeof(t_fileProceso));
		regTablaProceso = list_find(listaDeArchivosDelProceso->tablaProceso,(void*) encontrar_archProceso);

		t_fileGlobal* regTablaGlobal = malloc(sizeof(t_fileGlobal));
		regTablaGlobal = list_get(lista_File_global, ((int)regTablaProceso->global_fd));

		return regTablaGlobal;
		free(regTablaGlobal);
		free(regTablaProceso);



}

int hayOffsetArch(int fd){

	int encontrar_offset(t_offsetArch* glo) {
		if(fd == glo->fd)
		{
			return 1;
		}
		else
		{
			return 0;
		}

		 if(list_any_satisfy(offsetArch, (void*)encontrar_offset))
		 {
				t_offsetArch* regOffset = malloc(sizeof(t_offsetArch));
				regOffset = list_find(offsetArch, (void*)encontrar_offset);

			 return regOffset->offset;
		 }
		 else
		 {
			 return -1;
		 }
	}

	return 0;

}

void escribirArchivo( int pid_mensaje, int fd, char* infofile, int tamanio){

	if(fd == 1 || fd == 0){

		t_pcb* aux;

		int encontrado = 0;

		int fin = queue_size((cola_ejecucion));

		while(encontrado == 0 || fin > 0){
			pthread_mutex_lock(&mtx_ejecucion);
			aux = queue_pop(cola_ejecucion);
			pthread_mutex_unlock(&mtx_ejecucion);

			if(aux->pid == pid_mensaje){
				encontrado = 1;
			}

			pthread_mutex_lock(&mtx_ejecucion);
			queue_push(cola_ejecucion, aux);
			pthread_mutex_unlock(&mtx_ejecucion);

			fin--;
		}

		char* mensaje_conso = string_new();
		string_append(&mensaje_conso, "575");
		string_append(&mensaje_conso, ";");
		string_append(&mensaje_conso, string_itoa(pid_mensaje));
		string_append(&mensaje_conso, ";");
		string_append(&mensaje_conso, infofile);
		string_append(&mensaje_conso, ";");

		enviarMensaje(&aux->socket_consola, mensaje_conso);

		free(mensaje_conso);

	}else{

		if((int)lista_File_global->elements_count != 0 && (int)lista_File_proceso->elements_count != 0){

		char* mensajeFS= string_new();
		t_fileGlobal* regTablaGlobal = malloc(sizeof(t_fileGlobal));
		regTablaGlobal = traducirFDaPath(pid_mensaje,fd);

		string_append(&mensajeFS, "804");
		string_append(&mensajeFS, ";");
		string_append(&mensajeFS, regTablaGlobal->path);
		string_append(&mensajeFS, ";");
		string_append(&mensajeFS, string_itoa(tamanio));
		string_append(&mensajeFS, ";");
		string_append(&mensajeFS, ((char*)infofile));
		string_append(&mensajeFS, ";");

		int encontrar_archProceso(t_fileProceso* glo) {
			if (fd == glo->fileDescriptor)
				return 1;
			else
				return 0;
		}

		int offset = hayOffsetArch(fd);
		if( offset != -1)
		{

			string_append(&mensajeFS, string_itoa(offset));
			string_append(&mensajeFS, ";");

		}
		else
		{
			string_append(&mensajeFS, string_itoa(-1));
			string_append(&mensajeFS, ";");
		}


		//free(regTablaGlobal);

		enviarMensaje(&skt_filesystem, mensajeFS);

		int result = recv(skt_filesystem, mensajeFS, MAXBUF, 0);

		if (result > 0) {
			puts("archivo cerrado correctamente");
		}
		else {
			printf("Error no se pudo leer \n");
			exit(errno);
		}
		//free(mensajeFS);

	}

	}
}


t_fileGlobal* existeEnTablaGlobalArchivos(char* direccion)
{
	int encontrar_archGlobal(t_fileGlobal* glo){

	return (string_equals_ignore_case(((char*)glo->path),direccion));
	}
	 if(list_any_satisfy(lista_File_global, (void*)encontrar_archGlobal))
	 {
		 t_fileGlobal* reg = malloc(sizeof(t_fileGlobal));
		 reg = list_find(lista_File_global, (void*)encontrar_archGlobal);

		 return reg;
	 }
	 else
	 {
		 return NULL;
	 }
}

t_lista_fileProcesos* existeEnListaProcesosArchivos(pid_mensaje)
{

	if(lista_File_proceso->elements_count != 0){
		int encontrar_elementoListaProceso(t_lista_fileProcesos* glo){
			if(pid_mensaje == glo->pid)
				return 1;
			else
				return 0;
		}

	    if(list_any_satisfy(lista_File_proceso, (void*)encontrar_elementoListaProceso))
		{
	    t_lista_fileProcesos* regListaProcesos = malloc(sizeof(t_lista_fileProcesos));
	      regListaProcesos = list_find(lista_File_proceso,(void*) encontrar_elementoListaProceso);

	      return regListaProcesos;
		}
	    else
	    {
	    	return NULL;

	    }
	}
	else
	{
		return NULL;
	}


}

t_fileProceso* existeEnElementoTablaArchivo(t_list* tablaDelProceso, int fdGlobal)
{
	int encontrar_elementoFileProceso(t_fileProceso* glo){
		if(fdGlobal == glo->global_fd)
			return 1;
		else
			return 0;
	}
    if(list_any_satisfy(tablaDelProceso, (void*)encontrar_elementoFileProceso))
	{
		t_fileProceso* regTablaProceso = malloc(sizeof(t_fileProceso));
		regTablaProceso = list_find(tablaDelProceso,(void*) encontrar_elementoFileProceso);

		return regTablaProceso;
	}
    else
    {
    	return NULL;
    }
}

void grabarEnTablaGlobal(int cantidadAperturas, int FdGlobal, char* direccion)
{
	t_fileGlobal* archNuevo = malloc(sizeof(t_fileGlobal));
	archNuevo->cantidadDeAperturas = cantidadAperturas;
	archNuevo->fdGlobal = FdGlobal;
	archNuevo->path = direccion;

	list_add(lista_File_global,archNuevo);
}
void grabarEnTablaProcesos(int pid_mensaje, int fd, int globalFD, char* flag)
{
	t_lista_fileProcesos* archTablaProcesos = malloc(sizeof(t_lista_fileProcesos));
	archTablaProcesos->pid = pid_mensaje;
	archTablaProcesos->tablaProceso = list_create();

	t_fileProceso* archElemProceso = malloc(sizeof(t_fileProceso));
	archElemProceso->fileDescriptor = fd;
	archElemProceso->global_fd = globalFD;
	archElemProceso->flags = flag;

	list_add(archTablaProcesos->tablaProceso,archElemProceso);

	list_add(lista_File_proceso,archTablaProcesos);

}

void grabarEnTablaProcesosUnProcesoTabla(t_list* listaProcesoDeLaTablaProesos, int fd, int globalFD, char* flag)
{
	t_fileProceso* archElemProceso = malloc(sizeof(t_fileProceso));
	archElemProceso->fileDescriptor = fd;
	archElemProceso->global_fd = globalFD;
	archElemProceso->flags = flag;

	list_add(listaProcesoDeLaTablaProesos,archElemProceso);
}

int abrirArchivo(int pid_mensaje, char* direccion, char* flag)
{
	int fdNuevo;
	if((int)lista_File_global->elements_count != 0)
	{
		 t_fileGlobal* regTablaGlobal =  existeEnTablaGlobalArchivos(direccion);
			 if(regTablaGlobal != NULL) //pregunto si lo encontró el archivo en la global
			 {
			 regTablaGlobal->cantidadDeAperturas++;

			  t_lista_fileProcesos* regListaProceso = existeEnListaProcesosArchivos(pid_mensaje);

			  if(regListaProceso != NULL) //Pregunto si hay una lista de archivos para ese proceso
				{
					t_fileProceso* regTablaProceso = existeEnElementoTablaArchivo(regListaProceso->tablaProceso, regTablaGlobal->fdGlobal);

					if(regTablaProceso != NULL) //pregunto si existe una referencia a ese archivo en la tabla de procesos, en la lista de ese proceso
					{
						fdNuevo = regTablaProceso->fileDescriptor;
					}
					else
					{
					  fdNuevo = (regListaProceso->tablaProceso->elements_count + 3);
					  grabarEnTablaProcesosUnProcesoTabla(regListaProceso->tablaProceso,fdNuevo, regTablaGlobal->fdGlobal,flag);
					}
				}
				else
				{
					fdNuevo = 3;
					grabarEnTablaProcesos(pid_mensaje, fdNuevo, regTablaGlobal->fdGlobal, flag);
				}
			 }
			 else
			 {
				grabarEnTablaGlobal(1, lista_File_global->elements_count, direccion);

				t_lista_fileProcesos* regListaProceso = existeEnListaProcesosArchivos(pid_mensaje);

				if(regListaProceso != NULL) //Pregunto si hay una lista de archivos para ese proceso
				{
				  fdNuevo = (regListaProceso->tablaProceso->elements_count + 3);
				  grabarEnTablaProcesosUnProcesoTabla(regListaProceso->tablaProceso, fdNuevo, (lista_File_global->elements_count-1),flag);
				}
				else
				{
				 fdNuevo = lista_File_global->elements_count-1;
				 grabarEnTablaProcesos(pid_mensaje, 3, fdNuevo, flag);
				}
		     }
	      }
		 else
		 {
			fdNuevo = 3;
			grabarEnTablaGlobal(1, 0,direccion);
			grabarEnTablaProcesos(pid_mensaje, fdNuevo, 0, flag);
		 }

	char* mensajeAFS = string_new();
	string_append(&mensajeAFS, "803");
	string_append(&mensajeAFS, ";");
	string_append(&mensajeAFS, flag);
	string_append(&mensajeAFS, ";");
	string_append(&mensajeAFS, direccion);
	string_append(&mensajeAFS, ";");

	enviarMensaje(&skt_filesystem, mensajeAFS);

	free(mensajeAFS);
	return fdNuevo;
}
void borrarArchivo(int pid_mensaje, int fd)
{
	t_lista_fileProcesos* listaDeArchivosDelProceso = malloc(sizeof(listaDeArchivosDelProceso));
	listaDeArchivosDelProceso = existeEnListaProcesosArchivos(pid_mensaje);
	if(listaDeArchivosDelProceso != NULL)
	{
		int encontrar_archProceso(t_fileProceso* regFileProcess){
					if(fd == (int)regFileProcess->fileDescriptor)
						return 1;
					else
						return 0;
				}

			t_fileProceso* archBorrarPro = malloc(sizeof(t_fileProceso));
			archBorrarPro	= list_find(listaDeArchivosDelProceso->tablaProceso,(void*) encontrar_archProceso);

			int encontrar_archGobal(t_fileGlobal* regFileGlobal){
						if(archBorrarPro->global_fd == regFileGlobal->fdGlobal)
							return 1;
						else
							return 0;
					}
			t_fileGlobal* archBorrarGlobal = malloc(sizeof(t_fileGlobal));
			archBorrarGlobal = list_find(lista_File_global,(void*) encontrar_archGobal);
			//printf("la cantidad de aperturas es %d", archFileGlobal->cantidadDeAperturas);
			if(archBorrarGlobal->cantidadDeAperturas >= 2)
			{
				puts("El archivo tiene otras referencias y no puede ser eliminado, debe cerrar todas las aperturas primero");
			}
			else
			{

				char* mensajeAFS = string_new();
				string_append(&mensajeAFS, "802");
				string_append(&mensajeAFS, ";");
				string_append(&mensajeAFS, archBorrarGlobal->path);
				string_append(&mensajeAFS, ";");

				enviarMensaje(&skt_filesystem, mensajeAFS);


				list_remove_by_condition(listaDeArchivosDelProceso->tablaProceso,(void*) encontrar_archProceso);
				free(archBorrarPro);

				list_remove_by_condition(lista_File_global,(void*) encontrar_archGobal);
				free(archBorrarGlobal);

				/*int result = recv(skt_filesystem, mensajeAFS, sizeof(mensajeAFS), 0);

				if (result > 0) {
					puts("archivo borrado desde el fs");
				}
				else {
					perror("Error no se pudo borrar\n");
				}*/
				free(mensajeAFS);

			}

	}
	else
	{
		puts("El archivo a ser eliminado no se encuentra o no fue creado");
	}


	return;
}
char* cerrarArchivo(int pid_mensaje, int fd)
{
	t_lista_fileProcesos* listaDeArchivosDelProceso = malloc(sizeof(listaDeArchivosDelProceso));
	listaDeArchivosDelProceso = existeEnListaProcesosArchivos(pid_mensaje);

	char* resultado = string_new();

	if(listaDeArchivosDelProceso != NULL)
	{
		int encontrar_archProceso(t_fileProceso* glo){
			if(fd == glo->fileDescriptor)
				return 1;
			else
				return 0;
		}
		t_fileProceso* archAbrir1 = malloc(sizeof(t_fileProceso));
		archAbrir1 = list_find(listaDeArchivosDelProceso->tablaProceso,(void*) encontrar_archProceso);


		t_fileGlobal* archAbrir2 = malloc(sizeof(t_fileGlobal));
		archAbrir2 = list_get(lista_File_global, archAbrir1->global_fd);
		//printf("el valor a borrar es %d en global \n", archAbrir1->global_fd);
		//printf("el valor a borrar es %d en FD \n", archAbrir1->fileDescriptor);
		if(archAbrir2->cantidadDeAperturas >= 1)
		{
			char* mensajeAFS = string_new();
			string_append(&mensajeAFS, "802");
			string_append(&mensajeAFS, ";");
			string_append(&mensajeAFS, archAbrir2->path);
			string_append(&mensajeAFS, ";");

			enviarMensaje(&skt_filesystem, mensajeAFS);

			list_remove(lista_File_global, archAbrir1->global_fd);
			free(archAbrir2);
			string_append(&resultado, "El archivo fue cerrado y eliminado ya que tiene referencias en otros procesos");
		}
		else
		{
			archAbrir2->cantidadDeAperturas--;
			//list_add_in_index(lista_File_global, archAbrir1->global_fd, archAbrir2);
			string_append(&resultado, "El archivo fue cerrado del proceso, pero no se eliminó dado que tiene referencias en otros procesos");
		}

		list_remove_by_condition(listaDeArchivosDelProceso->tablaProceso,(void*) encontrar_archProceso);

		free(archAbrir1);
	}
	else
	{
		string_append(&resultado, "No se encuentra el archivo que se quiere cerrar.");
	}

	char* retorno = malloc(MAXBUF);
	retorno = string_duplicate(resultado);
	return retorno;
}
char* leerArchivo( int pid_mensaje, int fd, char* infofile, int tamanio)
{
	if((int)lista_File_global->elements_count != 0 && (int)lista_File_proceso->elements_count != 0)
	{
	t_fileGlobal* regTablaGlobal = malloc(sizeof(t_fileGlobal));
	regTablaGlobal = traducirFDaPath(pid_mensaje, fd);
	char* mensajeFSleer = string_new();
	string_append(&mensajeFSleer, "800");
	string_append(&mensajeFSleer, ";");
	string_append(&mensajeFSleer, regTablaGlobal->path);
	string_append(&mensajeFSleer, ";");
	string_append(&mensajeFSleer, string_itoa(tamanio));
	string_append(&mensajeFSleer, ";");
	string_append(&mensajeFSleer, ((char*)infofile));
	string_append(&mensajeFSleer, ";");

	int offset = hayOffsetArch(fd);
	if( offset != -1)
	{

		string_append(&mensajeFSleer, string_itoa(offset));
		string_append(&mensajeFSleer, ";");

	}
	else
	{
		string_append(&mensajeFSleer, string_itoa(-1));
		string_append(&mensajeFSleer, ";");
	}

	//free(archAbrir1);
	//free(regTablaGlobal);

	enviarMensaje(&skt_filesystem, mensajeFSleer);

	int result = recv(skt_filesystem, mensajeFSleer, MAXBUF, 0);

	if (result > 0) {
		//puts("archivo cerrado correctamente");
	}
	else {
		printf("Error no se pudo leer \n");
		exit(errno);
	}
	//free(mensajeAFS);
	return mensajeFSleer;
	}
	else
	{
		return "hubo un error y no se pudo realizar la lectura";
	}

}
