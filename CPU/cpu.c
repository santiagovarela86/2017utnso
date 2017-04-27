//CODIGOS
//500 CPU A KER - HANDSHAKE
//500 CPU A MEM - HANDSHAKE
//510 CPU A MEM - SOLICITUD SCRIPT
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
#include "helperParser.h"

void* manejo_memoria();
void* manejo_kernel();
char* solicitoScript(int * socketMemoria, char ** pcb);
void imprimoInfoPCB(char ** pcb);
char ** reciboPCB(int * socketKernel);

CPU_Config* configuracion;
int socketMemoria;
int socketKernel;
int programa_ejecutando;
t_list* variables_locales;

int main(int argc , char **argv){

	if (argc != 2){
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_kernel;
	pthread_t thread_id_memoria;

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	creoThread(&thread_id_kernel, manejo_kernel, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);

	pthread_join(thread_id_kernel, NULL);
	pthread_join(thread_id_memoria, NULL);

	free(configuracion);

    return EXIT_SUCCESS;
}

void* manejo_kernel(void *args) {
	int socketKernel;
	struct sockaddr_in direccionKernel;
	variables_locales = list_create();


	creoSocket(&socketKernel, &direccionKernel,	inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

	handShakeSend(&socketKernel, "500", "102", "Kernel");

	//RECIBO EL PCB
	char ** pcb = reciboPCB(&socketKernel);

	//MUESTRO LA INFO DEL PCB
	imprimoInfoPCB(pcb);

	AnSISOP_funciones *funciones = NULL;
	AnSISOP_kernel *kernel = NULL;
    funciones = malloc(sizeof(AnSISOP_funciones));
    kernel = malloc(sizeof(AnSISOP_kernel));

    inicializar_funciones(funciones, kernel);

    programa_ejecutando = atoi(pcb[0]);
	int quantum = atoi(pcb[7]);
	int iterador = atoi(pcb[1]);

	analizadorLinea("variables x, a, g", funciones, kernel);

	pause();

	free(pcb);

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;
}

char * solicitoScript(int * socketMemoria, char ** pcb){
	char message[MAXBUF];

	int pid = atoi(pcb[0]);
	int inicio_bloque_codigo = atoi(pcb[4]);
	int offset = atoi(pcb[5]);

	char* mensajeAMemoria = string_new();

	string_append(&mensajeAMemoria, "510");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pid));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(inicio_bloque_codigo));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(offset));

	enviarMensaje(socketMemoria, mensajeAMemoria);
	free(mensajeAMemoria);

	int result = recv((int) * socketMemoria, message, sizeof(message), 0);

	if (result > 0){
		return message;
	}else{
		printf("Error al solicitar Script a la Memoria\n");
		exit(errno);
	}
}

void imprimoInfoPCB(char ** pcb) {
	printf("PCB del proceso \n");
	printf("PID: %d\n", atoi(pcb[0]));
	printf("PC: %d\n", atoi(pcb[1]));
	printf("INDICE INICIO BLOQUE MEMORIA: %d \n", atoi(pcb[4]));
	printf("OFFSET: %d \n", atoi(pcb[5]));
	printf("QUANTUM: %d \n", atoi(pcb[7]));
}

char ** reciboPCB(int * socketKernel) {
	char ** pcb;
	char message[MAXBUF];

	int result = recv(*socketKernel, message, sizeof(message), 0);

	//Se agrega manejo de error al recibir el PCB
	if (result > 0) {
		//INICIO CODIGO DE DESERIALIZACION DEL PCB
		pcb = string_split(message, ";");
		//FIN CODIGO DE DESERIALIZACION DEL PCB

		//MUESTRO EL BUFFER DEL PCB
		printf("BUFFER: %s\n", message);
	} else {
		printf("Error al recibir PCB\n");
		exit(errno);
	}

	return pcb;
}

void* manejo_memoria(void * args){
	struct sockaddr_in direccionMemoria;

	creoSocket(&socketMemoria, &direccionMemoria, inet_addr(configuracion->ip_memoria), configuracion->puerto_memoria);
	puts("Socket de conexion a la Memoria creado correctamente\n");

	conectarSocket(&socketMemoria, &direccionMemoria);
	puts("Conectado a la Memoria\n");

	handShakeSend(&socketMemoria, "500", "202", "Memoria");

	//PRUEBO CON EL BLOQUEO EN VEZ DE LA ESPERA ACTIVA
	pause();

	shutdown(socketMemoria, 0);
	close(socketMemoria);
	return EXIT_SUCCESS;
}

void asignar(t_puntero direccion, t_valor_variable valor){

	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "511");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(direccion));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(valor));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);
	free(mensajeAMemoria);

	return;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "514");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, &identificador_variable);
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(programa_ejecutando));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);
	free(mensajeAMemoria);

	int result = recv(socketMemoria, mensajeAMemoria, sizeof(mensajeAMemoria), 0);

	if(result > 0){
		char**mensajeDesdeCPU = string_split(mensajeAMemoria, ";");
		int direccion = atoi(mensajeDesdeCPU[0]);

		printf("El valor de la direccion es: %d \n", direccion);

		free(mensajeAMemoria);

		return direccion;

				//TODO de esta falta hacer la parte de la memoria;
	}

	return 0;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "513");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(direccion_variable));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);
	free(mensajeAMemoria);

	int result = recv(socketMemoria, mensajeAMemoria, sizeof(mensajeAMemoria), 0);

	if(result > 0){
		char**mensajeDesdeCPU = string_split(mensajeAMemoria, ";");
		int valor = atoi(mensajeDesdeCPU[0]);

		printf("El valor es: %d \n", valor);

		free(mensajeAMemoria);

		return valor;
	}

	return 0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){

	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "512");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, &identificador_variable);
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(programa_ejecutando));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);

	char*respuesta = string_new();

	int result = recv(socketMemoria, mensajeAMemoria, string_length(mensajeAMemoria), 0);

	if(result > 0){
		char**mensajeDesdeCPU = string_split(mensajeAMemoria, ";");
		int posicion = atoi(mensajeDesdeCPU[0]);

		variables* var = malloc(sizeof(variables));
		var->direcion = posicion;
		var->nro_variable = (list_size(variables_locales) + 1);
		var->variable = identificador_variable;
		var->pid = programa_ejecutando;

		list_add(variables_locales, var);

		printf("La variable %c se guardo en la pos: %d \n", identificador_variable , posicion);

		free(mensajeAMemoria);

		return posicion;
	}

	return -1;

}
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "514");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, &variable);
	string_append(&mensajeAKernel, ";");


	enviarMensaje(&socketKernel, mensajeAKernel);
	free(mensajeAKernel);

	int result = recv(socketKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

	if(result > 0){
		char**mensajeDesdeKernel = string_split(mensajeAKernel, ";");

		int valor = atoi(mensajeDesdeKernel[0]);

		printf("El valor de la compartida es: %d \n", valor);

		free(mensajeAKernel);

		return valor;

		//TODO de esta falta hacer la parte de la memoria;
	}

	return 0;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "515");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, &variable);
	string_append(&mensajeAKernel, ";");


	enviarMensaje(&socketKernel, mensajeAKernel);
	free(mensajeAKernel);

		//TODO de esta falta hacer la parte de la memoria;

	return 0;
}

void irAlLabel(t_nombre_etiqueta identificador_variable){

	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

	return;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

	return;
}

void finalizar(void){

	return;
}

void retornar(t_valor_variable retorno){

	return;
}

void wait(t_nombre_semaforo identificador_semaforo){

	return;
}

void signal(t_nombre_semaforo identificador_semaforo){

	return;
}

t_puntero reservar(t_valor_variable espacio){

	return 0;
}

void liberar(t_puntero puntero){

	return;
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){

	return 0;
}

void borrar(t_descriptor_archivo direccion){

	return;
}
void cerrar(t_descriptor_archivo descriptor){

	return;
}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

	return;
}
void escribir(t_descriptor_archivo descriptor_archivo, void * informacion, t_valor_variable tamanio){

	return;
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){

	return;
}

