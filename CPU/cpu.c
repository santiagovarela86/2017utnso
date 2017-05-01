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
#include "cpu.h"

CPU_Config* configuracion;
int socketMemoria;
int socketKernel;
t_list* variables_locales;
t_pcb* pcb;

int main(int argc , char **argv){

	if (argc != 2){
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_kernel;
	pthread_t thread_id_memoria;

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);

	variables_locales = list_create();

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

	creoSocket(&socketKernel, &direccionKernel,	inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
	puts("Socket de conexion al Kernel creado correctamente\n");

	conectarSocket(&socketKernel, &direccionKernel);
	puts("Conectado al Kernel\n");

	handShakeSend(&socketKernel, "500", "102", "Kernel");

	AnSISOP_funciones *funciones = NULL;
	AnSISOP_kernel *kernel = NULL;
    funciones = malloc(sizeof(AnSISOP_funciones));
    kernel = malloc(sizeof(AnSISOP_kernel));
    inicializar_funciones(funciones, kernel);

    while(1){
    	//RECIBO EL PCB
    	pcb = reciboPCB(&socketKernel);

    	//MUESTRO LA INFO DEL PCB
    	imprimoInfoPCB(pcb);

    	char* instruccion = string_new();

    	instruccion = solicitoInstruccion(pcb);

    	analizadorLinea(instruccion, funciones, kernel);

    	pause();

     	char* mensajeAKernel = string_new();

    	if(pcb->indiceCodigo->elements_count-1 == pcb->program_counter){ //Se ejecutaron todas las instrucciones
        	string_append(&mensajeAKernel, "531");
        	string_append(&mensajeAKernel, ";");
        	string_append(&mensajeAKernel, string_itoa(pcb->pid));
        	string_append(&mensajeAKernel, ";");
    	}else{
        	string_append(&mensajeAKernel, "530");
        	string_append(&mensajeAKernel, ";");
        	string_append(&mensajeAKernel, string_itoa(pcb->pid));
        	string_append(&mensajeAKernel, ";");
    	}

    	enviarMensaje(&socketKernel, mensajeAKernel);

    	free(mensajeAKernel);
    }

	pause();

	free(pcb);

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;
}

//TODO Cambiar esta funcion por "solicitoInstruccion"
char * solicitoInstruccion(t_pcb* pcb){

    int inst_pointer = pcb->program_counter;

    elementoIndiceCodigo* coordenadas_instruccion = ((elementoIndiceCodigo*) list_get(pcb->indiceCodigo, inst_pointer));

	char message[MAXBUF];

	char* mensajeAMemoria = string_new();

	string_append(&mensajeAMemoria, "510");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->pid));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(coordenadas_instruccion->start));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(coordenadas_instruccion->offset));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->inicio_codigo));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);

	free(mensajeAMemoria);

	int result = recv(socketMemoria, message, sizeof(message), 0);

	if (result > 0){
		return message;
	}else{
		printf("Error al solicitar Instruccion a la Memoria\n");
		exit(errno);
	}
}

void imprimoInfoPCB(t_pcb * pcb){
	printf("PID: %d\n", pcb->pid);
	printf("PC: %d\n", pcb->program_counter);
	printf("Cantidad de Paginas: %d\n", pcb->cantidadPaginas);
	printf("Inicio de Codigo: %d\n", pcb->inicio_codigo);
	printf("Tabla de Archivos: %d\n", pcb->tabla_archivos);
	printf("Posicion de Stack: %d\n", pcb->pos_stack);
	printf("Exit Code: %d\n", pcb->exit_code);

	int i;
	for (i = 0; i < pcb->indiceCodigo->elements_count; i++){
		elementoIndiceCodigo * elem = malloc(sizeof(elem));
		elem = list_get(pcb->indiceCodigo, i);
		printf("Indice de Instruccion %d: Start %d, Offset %d\n", i, elem->start, elem->offset);
	}

	for (i = 0; i < pcb->indiceEtiquetas->elements_count; i++){
		int * elem = list_get(pcb->indiceEtiquetas, i);
		printf("Etiqueta %d: %d\n", i, * elem);
	}

	for (i = 0; i < pcb->indiceStack->elements_count; i++){
		int * elem = list_get(pcb->indiceStack, i);
		printf("Elemento de Pila %d: %d\n", i, * elem);
	}

}

t_pcb * reciboPCB(int * socketKernel) {
	t_pcb * pcb;
	char message[MAXBUF];

	int result = recv(*socketKernel, message, sizeof(message), 0);

	//Se agrega manejo de error al recibir el PCB
	if (result > 0) {
		//INICIO CODIGO DE DESERIALIZACION DEL PCB
		pcb = deserializar_pcb(message);
		//FIN CODIGO DE DESERIALIZACION DEL PCB

		//MUESTRO EL BUFFER DEL PCB
		printf("BUFFER: %s\n", message);
	} else {
		printf("Error al recibir PCB\n");
		exit(errno);
	}

	return pcb;
}

t_pcb * deserializar_pcb(char * mensajeRecibido){
	t_pcb * pcb = malloc(sizeof(t_pcb));
	int cantIndiceCodigo, cantIndiceEtiquetas, cantIndiceStack;
	char ** message = string_split(mensajeRecibido, ";");
	pcb->indiceCodigo = list_create();
	pcb->indiceEtiquetas = list_create();
	pcb->indiceStack = list_create();

	pcb->pid = atoi(message[0]);
	pcb->program_counter = atoi(message[1]);
	pcb->cantidadPaginas = atoi(message[2]);
	pcb->inicio_codigo = atoi(message[3]);
	pcb->tabla_archivos = atoi(message[4]);
	pcb->pos_stack = atoi(message[5]);
	pcb->exit_code = atoi(message[6]);
	cantIndiceCodigo = atoi(message[7]);
	cantIndiceEtiquetas = atoi(message[8]);
	cantIndiceStack = atoi(message[9]);

	int i = 10;

	while (i < 10 + cantIndiceCodigo * 2){
		elementoIndiceCodigo * elem = malloc(sizeof(elem));
		elem->start = atoi(message[i]);
		i++;
		elem->offset = atoi(message[i]);
		i++;
		list_add(pcb->indiceCodigo, elem);
	}

	int j = i;

	while (i < j + cantIndiceEtiquetas){
		list_add(pcb->indiceEtiquetas, message[i]);
		i++;
	}

	int k = i;

	while (i < k + cantIndiceStack){
		list_add(pcb->indiceStack, message[i]);
		i++;
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

	printf("Entre a asignar el valor %d en la direccion %d \n", valor, direccion);
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

	int encontrar_var(variables *var) {
		return (var->pid == pcb->pid && var->variable == identificador_variable);
	}

	variables* var_encontrada = list_find(variables_locales, (void*) encontrar_var);

	printf("Encontre la direccion %d para la variable %c \n", var_encontrada->direcion, identificador_variable);
	return var_encontrada->direcion;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	printf("Busco el valor de la posicion %d \n", direccion_variable);

	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "513");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(direccion_variable));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);

	int result = recv(socketMemoria, mensajeAMemoria, sizeof(mensajeAMemoria), 0);

	if(result > 0){
		char**mensajeDesdeCPU = string_split(mensajeAMemoria, ";");

		printf("Lei el valor %s en la posicion %d \n", mensajeDesdeCPU[0], direccion_variable);

		int valor = atoi(mensajeDesdeCPU[0]);

		printf("El valor de la posicion %d es %d\n", direccion_variable, valor);

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
	string_append(&mensajeAMemoria, string_itoa(pcb->pid));
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
		var->pid = pcb->pid;

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
	puts("FIN DEL PROGRAMA");
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

