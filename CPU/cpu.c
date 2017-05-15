//CODIGOS
//500 CPU A KER - HANDSHAKE
//500 CPU A MEM - HANDSHAKE
//510 CPU A MEM - SOLICITUD SCRIPT
//511 CPU A MEM - ASIGNAR VARIABLE
//512 CPU A MEM - DEFINIR VARIABLE
//513 CPU A MEM - DEREFERENCIAR VARIABLE
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
int sktKernel;
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
	sktKernel = socketKernel;

	AnSISOP_funciones *funciones = NULL;
	AnSISOP_kernel *kernel = NULL;

    funciones = malloc(sizeof(AnSISOP_funciones));
    kernel = malloc(sizeof(AnSISOP_kernel));

	inicializar_funciones(funciones, kernel);

    char* instruccion = string_new();
    while(1){
    	//RECIBO EL PCB

    	pcb = reciboPCB(&socketKernel);

    	//MUESTRO LA INFO DEL PCB
    	imprimoInfoPCB(pcb);

    	instruccion = "";

    	while(pcb->quantum > 0 && pcb->indiceCodigo->elements_count != pcb->program_counter){
    		instruccion = solicitoInstruccion(pcb);

    		analizadorLinea(instruccion, funciones, kernel);

    		pcb->quantum--;
    		pcb->program_counter++;
    	}


    	if(pcb->program_counter >= pcb->indiceCodigo->elements_count){ //Se ejecutaron todas las instrucciones

    		char* mensajeAKernel = string_new();

        	string_append(&mensajeAKernel, "531");
        	string_append(&mensajeAKernel, ";");
        	string_append(&mensajeAKernel, string_itoa(pcb->pid));
        	string_append(&mensajeAKernel, ";");

        	enviarMensaje(&socketKernel, mensajeAKernel);

        	free(mensajeAKernel);
    	}else{ //FIN DE QUANTUM

    		char* mensajeAKernel = string_new();

        	string_append(&mensajeAKernel, "530");
        	string_append(&mensajeAKernel, ";");

        	enviarMensaje(&socketKernel, mensajeAKernel);

        	char* mensajeAKernel2 = serializar_pcb(pcb);

        	printf("Enviando mensaje %s \n", mensajeAKernel);
        	enviarMensaje(&socketKernel, mensajeAKernel2);

        	free(mensajeAKernel);
        	free(mensajeAKernel2);
    	}


    }

	pause();

	free(pcb);

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;
}

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
	string_append(&mensajeAMemoria, string_itoa(pcb->cantidadPaginas));
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
	printf("Quantum: %d\n", pcb->quantum);
/*
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
*/
	puts("");
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

	} else {
		printf("Error al recibir PCB\n");
		exit(errno);
	}

	return pcb;
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

	//pcb->etiquetas = malloc(pcb->etiquetas_size);
	pcb->etiquetas = string_new();
	while (i < j + pcb->etiquetas_size){
		int ascii = atoi(message[i]);

		if (ascii >= 32 || ascii <= 126){
			pcb->etiquetas[i] = (char) ascii;
		} else {
			pcb->etiquetas[i] = atoi(message[i]);
		}

		//printf("[%c, %d]", pcb->etiquetas[i], pcb->etiquetas[i]);
		i++;
	}

	int k = i;

	while (i < k + cantIndiceStack){
		t_Stack *sta = malloc(sizeof(t_Stack));
		sta->direccion.offset = atoi(message[i]);
		i++;
		sta->direccion.pagina = atoi(message[i]);
		i++;
		sta->direccion.size = atoi(message[i]);
		i++;
		sta->nombre_funcion = message[i][0];
		i++;
		sta->nombre_variable = message[i][0];
		list_add(pcb->indiceStack, sta);
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
	puts("Asignar");
	puts("");


	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "511");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(direccion));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(valor));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);
	free(mensajeAMemoria);

	printf("Asigne el valor %d en la direccion %d \n", valor, direccion);

	return;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	puts("Obtener Posicion Variable");
	puts("");

	int encontrar_var(variables *var) {
		return (var->pid == pcb->pid && var->variable == identificador_variable);
	}

	variables* var_encontrada = list_find(variables_locales, (void*) encontrar_var);

	//TODO CAMBIAR EL TIPO DE DATO DE var_encontrada A t_Stack Y LUEGO ENVIARMENSAJE A MEMORIA PIDIENDO LA POSICION EXACTA DE LA VARIABLE
	//ENVIANDOLE PID, PAGINA Y OFFSET Y ESA DIRECCION ES LA QUEDE ENVIARSE EN EL RETURN.

	return var_encontrada->direcion;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	puts("Dereferenciar");
	puts("");


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

		free(mensajeAMemoria);

		return valor;
	}

	return 0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){
	puts("Definir Variable");
	puts("");

	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "512");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, &identificador_variable);
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->pid));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->cantidadPaginas));
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);

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
	puts("Obtener Valor Compartida");
	puts("");

	int valor;
	char* msj = string_new();

	string_append(&msj, "514");
	string_append(&msj, ";");
	string_append(&msj, variable);
	string_append(&msj, ";");

	enviarMensaje(&sktKernel, msj);

	int result = recv(sktKernel, msj, sizeof(msj), 0);

	if(result > 0){

		char**mensajeDesdeKernel = string_split(msj, ";");

		valor = atoi(mensajeDesdeKernel[0]);

	}

	free(msj);

	return valor;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	puts("Asignar Valor Compartida");
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "515");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, variable);
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(valor));
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);
	free(mensajeAKernel);

	return valor;
}

void irAlLabel(t_nombre_etiqueta identificador_variable){
	puts("Ir a Label");
	puts("");
	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
	puts("Llamar Sin Retorno");
	puts("");
	return;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	puts("Llamar Con Retorno");
	puts("");
	return;
}

void finalizar(void){
	puts("FIN DEL PROGRAMA");
	puts("");
	return;
}

void retornar(t_valor_variable retorno){
	puts("Retornar");
	return;
}

void wait(t_nombre_semaforo identificador_semaforo){
	puts("Wait");
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "570");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, identificador_semaforo);
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);

	int valor_esperado = 0;

	while(valor_esperado == 0){
		int result = recv(sktKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

		if(result > 0){

			char**mensajeDesdeKernel = string_split(mensajeAKernel, ";");

			int valor = atoi(mensajeDesdeKernel[0]);

			if(valor == 570){
				valor_esperado = 1;
			}
		}
	}

	free(mensajeAKernel);

	return;
}

void signal(t_nombre_semaforo identificador_semaforo){
	puts("Signal");
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "571");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, identificador_semaforo);
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);

	recv(sktKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

	free(mensajeAKernel);

	return;
}

t_puntero reservar(t_valor_variable espacio){
	puts("Reservar");
	puts("");
	return 0;
}

void liberar(t_puntero puntero){
	puts("Liberar");
	puts("");
	return;
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){
	puts("Abrir");
	puts("");
	return 0;
}

void borrar(t_descriptor_archivo direccion){
	puts("Borrar");
	puts("");
	return;
}
void cerrar(t_descriptor_archivo descriptor){
	puts("Cerrar");
	puts("");
	return;
}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	puts("Mover Cursor");
	puts("");
	return;
}
void escribir(t_descriptor_archivo descriptor_archivo, void * informacion, t_valor_variable tamanio){
	puts("Escribir");
	printf("El descriptor es: %d \n", descriptor_archivo);
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "575");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(descriptor_archivo));
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(pcb->pid));
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, ((char*) informacion));
	string_append(&mensajeAKernel, ";");


	enviarMensaje(&sktKernel, mensajeAKernel);

	free(mensajeAKernel);

	return;
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	puts("Leer");
	puts("");
	return;
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

	for (i = 0; i < pcb->etiquetas_size; i++){
		string_append(&mensajeACPU, string_itoa(pcb->etiquetas[i]));
		string_append(&mensajeACPU, ";");
		//printf("[%d]", pcb->etiquetas[i]);
	}

	for (i = 0; i < pcb->indiceStack->elements_count; i++){
		t_Stack *sta = malloc(sizeof(t_Stack));
		sta =  list_get(pcb->indiceStack, i);
		string_append(&mensajeACPU, string_itoa(sta->direccion.offset));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->direccion.pagina));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->direccion.size));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, &(sta->nombre_funcion));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, &(sta->nombre_variable));
		string_append(&mensajeACPU, ";");
	}



	return mensajeACPU;

}

