//CODIGOS
//300 CON A KER - HANDSHAKE
//399 CON A KER - ADIOS CONSOLA
//398 CON A KER - ENVIO DE PID DEL PROGRAMA A FINALIZAR
//101 KER A CON - RESPUESTA HANDSHAKE
//103 KER A CON - PID DE PROGRAMA
//666 KER A CON - PID DE PROGRAMA MUERTO

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include "configuracion.h"
#include <pthread.h>
#include "helperFunctions.h"
#include "consola.h"
#include <signal.h>
#define BST (-3)


   struct tm *info;


t_queue* cola_programas;

Consola_Config* configuracion;

InfoConsola infoConsola;

pthread_t threadKernel;
pthread_t threadConsola;


int main(int argc , char **argv)
{

	if (argc != 2)
    {
    	printf("Error. Parametros incorrectos.\n");
    	return EXIT_FAILURE;
    }

	cola_programas = crear_cola_programas();

	inicializarEstado(&infoConsola);

	configuracion = leerConfiguracion(argv[1]);
    imprimirConfiguracion(configuracion);

	int socketKernel;
	struct sockaddr_in direccionKernel;
	list_add(infoConsola.sockets, &socketKernel);

	creoSocket(&socketKernel, &direccionKernel, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

    //pthread_t threadConsola;
    //pthread_t threadKernel;

    list_add(infoConsola.threads, &threadConsola);
    list_add(infoConsola.threads, &threadKernel);

    creoThread(&threadConsola, handlerConsola, &socketKernel);
    creoThread(&threadKernel, handlerKernel, &socketKernel);

	pthread_join(threadConsola, NULL);
	pthread_join(threadKernel, NULL);

	destruirEstado(&infoConsola);
	free(configuracion);
	free(cola_programas);

    return EXIT_SUCCESS;
}

void * handlerConsola(void * args){

	int * socketKernel = (int *) args;

	char* valorIngresado = string_new();

	int intentos_fallidos = 0;

	while(intentos_fallidos < 10 && infoConsola.estado_consola == 1){

		puts("");
		puts("***********************************************************");
		puts("Ingrese el numero de la acción a realizar");
		puts("1) Iniciar programa");
		puts("2) Finalizar programa");
		puts("3) Desconectar");
		puts("4) Limpiar");
		puts("***********************************************************");

		scanf("%s", valorIngresado);
		 int numero =	atoi(valorIngresado);
				switch(numero){
				case 1:
					intentos_fallidos = 0;
					iniciar_programa(socketKernel);
					break;
				case 2:
					intentos_fallidos = 0;
					terminar_proceso(socketKernel);
					break;
				case 3:
					intentos_fallidos = 0;
					desconectar_consola(socketKernel);
					break;
				case 4:
					intentos_fallidos = 0;
					limpiar_mensajes();
					break;
				default:
					intentos_fallidos++;
					break;
				}

				if(intentos_fallidos == 10){
					return 0;
				}

	}

	free(valorIngresado);
	return 0;
}

void * handlerKernel(void * args){

	int * socketKernel = (int *) args;

	handShakeSend(socketKernel, "300", "101", "Kernel");

    pthread_t threadEscuchaKernel;
    creoThread(&threadEscuchaKernel, escuchar_Kernel, socketKernel);

	pthread_join(threadEscuchaKernel, NULL);
	return EXIT_SUCCESS;
}

void * manejoPrograma(void * args){

	int * socketKernel = (int *) args;

	printf("Se creo el hilo para manejar programa \n");

	return 0;
}

void terminar_proceso(int* socket_kernel){

	puts(" ");
	puts("Ingrese PID del proceso a terminar");

	char* pid_ingresado = string_new();

	scanf("%s", pid_ingresado);

	char* message = string_new();
	string_append(&message, "398");
	string_append(&message, ";");
	string_append(&message, pid_ingresado);
	string_append(&message, ";");


	enviarMensaje(socket_kernel, message);

	return;
}

void desconectar_consola(int* socket_kernel){
	infoConsola.estado_consola = 0;

	char message[MAXBUF];
	strcpy(message, "399;");

	enviarMensaje(socket_kernel, message);

	//exit(-1);
	//PUSE ESTO CON LA ESPERANZA DE QUE EL MAIN
	//LLEGUE AL PTHREAD_JOIN Y LIBERE LOS RECURSOS
	pthread_kill(threadConsola, SIGTERM);
	pthread_kill(threadKernel, SIGTERM);

	return;
}

void limpiar_mensajes(){
	system("clear");
	return;
}

void * escuchar_Kernel(void * args){
	char buffer[MAXBUF];
	char bufferHoraFin[26];
	char bufferHoraCom[26];

	int * socketKernel = (int *) args;

	while(infoConsola.estado_consola == 1){

		int result = recv(*socketKernel, buffer, sizeof(buffer), 0);

		if (result > 0){
			char** respuesta_kernel = string_split(buffer, ";");

			if(atoi(respuesta_kernel[0]) == 103){
				/*103 es creacion*/
				programa* program = malloc(sizeof(program));
				time_t * comienzo = malloc(sizeof(time_t));
				  printf("Comenzó el programa de pid: %d\n",  atoi(respuesta_kernel[1]));
				  time(comienzo);
				   /* Get GMT time */


				program->pid = atoi(respuesta_kernel[1]);
				program->inicio = localtime(comienzo);
				program->fin = localtime(comienzo);
				program->duracion = 0;
				program->mensajes = 0;
				program->socket_kernel = *socketKernel;

				struct tm* tm_info;
				tm_info = localtime(comienzo);

				strftime(bufferHoraCom, 26, "%Y-%m-%d %H:%M:%S", program->inicio);
				printf( "Comienzo: %s\n", bufferHoraCom );


				queue_push(cola_programas, program);

			}else if(atoi(respuesta_kernel[0]) == 197){
				printf("El programa no pudo iniciarse por falta de memoria\n");
			}else if (atoi(respuesta_kernel[0]) == 575){
				printf("Mensaje de programa %d : %s\n", atoi(respuesta_kernel[1]), respuesta_kernel[2]);
				puts("");

			}else if (atoi(respuesta_kernel[0]) == 666){
				/*666 es muerte*/
				programa* p = malloc(sizeof(programa));
			//TODO
				int pid = atoi(respuesta_kernel[1]);
				int encontrado = 0;
				int fin = queue_size(cola_programas);


							while(fin > 0 && encontrado == 0){
								//pthread_mutex_lock(&mtx_bloqueados);
								p = queue_pop(cola_programas);
								//pthread_mutex_unlock(&mtx_bloqueados);
								time_t * final = malloc(sizeof(time_t));
								if(p->pid == pid){
									encontrado = 1;
									//time(final);
									p->fin =localtime(final);
									p->duracion = difftime(p->fin, p->inicio);

									struct tm* tm_infoF;
									tm_infoF = localtime(final);
									//tm_infoF = localtime(&final);
									time(final);

									strftime(bufferHoraFin, 26, "%Y-%m-%d %H:%M:%S", localtime(final));

									printf( "Final: %s\n", bufferHoraFin );
								}

								//pthread_mutex_lock(&mtx_bloqueados);
								queue_push(cola_programas, p);
								//pthread_mutex_unlock(&mtx_bloqueados);

								fin--;
							}

					       // char output[128];
					        //strftime(output,128,"%d/%m/%y %H:%M:%S",p->inicio);


				printf("Finalizó el programa de pid: %d\n",  atoi(respuesta_kernel[1]));
				printf("Su hora de inicio fue:\n");
				printf(" %s\n", bufferHoraCom);

				//char outputF[128];
				//strftime(outputF,128,"%d/%m/%y %H:%M:%S",p->fin);
				printf("Su hora finalizacion fue: \n");
				printf( " %s\n", bufferHoraFin );


			  printf( "Número de segundos transcurridos desde el comienzo del programa: %f s\n", difftime(p->fin, p->inicio) );
			  printf( "ó: %f s\n", p->duracion );

			}

			//free(respuesta_kernel);
		}
	}

	return 0;
}

void iniciar_programa(int* socket_kernel){

	puts("");
	puts("Ingrese nombre del programa");

	pthread_t threadPrograma;
	list_add(infoConsola.threads, &threadPrograma);


	char directorio[MAXBUF];

	scanf("%s", directorio);

	int fd_script = open(directorio, O_RDWR);

	while (fd_script < 0) //por si ingresamos mal el nombre del archivo no cuelgue.
	{
			puts("");
			puts("No se encuentra el archivo solicitado");
			puts("");
			puts("Ingrese nombre del programa");
			puts("");
			scanf("%s", directorio);
			fd_script = open(directorio, O_RDWR);

	}
	struct stat scriptFileStat;
	fstat(fd_script, &scriptFileStat);
	char* pmap_script = mmap(0, scriptFileStat.st_size, PROT_READ, MAP_SHARED, fd_script, 0);
	char* respuestaConsola =  string_new();
	string_append(&respuestaConsola, "303");
	string_append(&respuestaConsola, ";");
	string_append(&respuestaConsola, pmap_script);

	enviarMensaje(socket_kernel, respuestaConsola);
	free(respuestaConsola);

	close(fd_script);
	munmap(pmap_script,scriptFileStat.st_size);

	creoThread(&threadPrograma, manejoPrograma, socket_kernel);

	return;
}

t_queue* crear_cola_programas(){
	t_queue* cola = queue_create();
	return cola;
}

void eliminar_programa(programa *self){

}

void flush_cola_programas(t_queue* queue){
	 queue_destroy_and_destroy_elements(queue, (void*) eliminar_programa);
}

void inicializarEstado(InfoConsola * infoConsola){
	infoConsola->proceso_a_terminar = -1;
	infoConsola->estado_consola = 1;
	infoConsola->programas_ejecutando = 0;
	infoConsola->threads = list_create();
	infoConsola->sockets = list_create();
}

void socketDestroyer(void * socket){
	int * skt = (int *) socket;
	shutdown(*skt, 0);
	//VA ESTO?
	close(*skt);
}

void destruirEstado(InfoConsola * infoConsola){
	list_destroy(infoConsola->threads);
	list_destroy_and_destroy_elements(infoConsola->sockets, socketDestroyer);
}

unsigned char *cGetDate() {
     time_t hora;
     struct tm *tiempo;
     char *fecha;

     hora = time(NULL);
     tiempo = localtime(&hora);

     fecha = (char *)malloc(sizeof(char)*SIZE_FECHA);
     if (fecha==NULL) {perror ("No hay memoria"); return "";}

     sprintf (fecha,"%02d/%02d/%4d",tiempo->tm_mday, tiempo->tm_mon+1, tiempo->tm_year+1900);
     return fecha;
}
