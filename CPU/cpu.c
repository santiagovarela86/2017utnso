//CODIGOS
//500 CPU A KER - HANDSHAKE
//500 CPU A MEM - HANDSHAKE
//510 CPU A MEM - SOLICITUD SCRIPT
//511 CPU A MEM - ASIGNAR VARIABLE
//512 CPU A MEM - DEFINIR VARIABLE
//513 CPU A MEM - DEREFERENCIAR VARIABLE
//102 KER A CPU - RESPUESTA HANDSHAKE DE CPU
//202 MEM A CPU - RESPUESTA HANDSHAKE
//801 CPU A KER - CERRAR ARCHIVO
//802 CPU A KER - BORRAR ARCHIVO
//803 CPU A KER - ABRIR ARCHIVO
//600 CPU A KER - RESERVAR MEMORIA HEAP
//700 CPU A KER - ELIMINAR MEMORIA HEAP
//710 KER A CPU - SE ELIMINA MEMORIA OK
//615 CPU A KER - NO SE PUEDEN ASIGNAR MAS PAGINAS A UN PROCESO
//777 CPU A KER - SUMAR UNA PAGINA AL PCB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include "configuracion.h"
#include "helperFunctions.h"
#include "helperParser.h"
#include "cpu.h"


CPU_Config* configuracion;
int socketMemoria;
int sktKernel;
t_list* variables_locales;
t_pcb* pcb;
int bloqueo;
pthread_mutex_t mutex_instrucciones;

_Bool pcbHabilitado = true;


int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	pthread_t thread_id_kernel;
	pthread_t thread_id_memoria;
	pthread_mutex_init(&mutex_instrucciones, NULL);

	signal(SIGUSR1, manejador_signal);

	configuracion = leerConfiguracion(argv[1]);
	imprimirConfiguracion(configuracion);
	bloqueo = 0;

	variables_locales = list_create();

	creoThread(&thread_id_kernel, manejo_kernel, NULL);
	creoThread(&thread_id_memoria, manejo_memoria, NULL);

	pthread_join(thread_id_kernel, NULL);
	pthread_join(thread_id_memoria, NULL);

	free(configuracion);

	pthread_mutex_destroy(&mutex_instrucciones);

	return EXIT_SUCCESS;
}

void* manejo_kernel(void *args) {
	int socketKernel;
	struct sockaddr_in direccionKernel;

	creoSocket(&socketKernel, &direccionKernel,
			inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
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
	while (1) {
		//RECIBO EL PCB
		pcb = reciboPCB(&socketKernel);
		printf("el pid recibido es %d \n", pcb->pid);
		bloqueo = 0;
		printf("Recibi un PCB para Ejecutar\n");
		//MUESTRO LA INFO DEL PCB
		imprimoInfoPCB(pcb);

		//Guardo el Quantum
		int q = pcb->quantum;

		while (pcb->quantum > 0
				&& pcb->indiceCodigo->elements_count != pcb->program_counter
					&& bloqueo == 0
						&& pcbHabilitado == true) {

			pthread_mutex_lock(&mutex_instrucciones);
			//printf("antes de ir a instrucción el progrma vale  %d",pcb->program_counter);
			instruccion = "";

			instruccion = solicitoInstruccion(pcb);

			analizadorLinea(instruccion, funciones, kernel);

			pcb->quantum--;
			pcb->program_counter++;

			pthread_mutex_unlock(&mutex_instrucciones);

			if(!string_contains(instruccion, "end"))
			{
				printf("El program counter es: %d\n", pcb->program_counter);
				printf("\n");
			}

		}

		if (bloqueo == 1 && pcbHabilitado == true) {
			char* mensajeAKernel = string_new();

			string_append(&mensajeAKernel, "532");
			string_append(&mensajeAKernel, ";");
			string_append(&mensajeAKernel, string_itoa(pcb->pid));
			string_append(&mensajeAKernel, ";");

			enviarMensaje(&socketKernel, mensajeAKernel);

			free(mensajeAKernel);
		} else if (pcb->program_counter >= pcb->indiceCodigo->elements_count && pcbHabilitado == true) { //Se ejecutaron todas las instrucciones

			char* mensajeAKernel = string_new();

			string_append(&mensajeAKernel, "531");
			string_append(&mensajeAKernel, ";");
			enviarMensaje(&socketKernel, mensajeAKernel);

			//ESPERO A QUE EL KERNEL ME CONFIRME QUE ESTA LISTO
			int result = recv(socketKernel, mensajeAKernel, MAXBUF, 0);

			if (result > 0) {
				if (strcmp(mensajeAKernel, "531;") == 0) {
					char* mensajeAKernel2 = serializar_pcb(pcb);
					enviarMensaje(&socketKernel, mensajeAKernel2);
					free(mensajeAKernel2);
				} else {
					printf("Error en protocolo de mensajes entre procesos\n");
					exit(errno);
				}
			} else {
				printf("Error de comunicacion de fin de programa con el Kernel\n");
				exit(errno);
			}

			free(mensajeAKernel);

		} else if (pcb->quantum <= 0 && pcbHabilitado == true){
				//FIN DE QUANTUM
				char * buffer = malloc(MAXBUF);

				enviarMensaje(&socketKernel, serializarMensaje(1, 530));

				pcb->quantum = q;

				//CUANDO EL KERNEL TERMINA DE PROCESAR EL FIN DE QUANTUM
				recv(socketKernel, buffer, MAXBUF, 0);

				//ENVIO EL PCB AL KERNEL NUEVAMENTE
				enviarMensaje(&socketKernel, serializar_pcb(pcb));

				free(buffer);
			}

		pcbHabilitado = true;
	}

	pause();

	free(pcb);

	shutdown(socketKernel, 0);
	close(socketKernel);
	return EXIT_SUCCESS;
}

char * solicitoInstruccion(t_pcb* pcb) {

	int inst_pointer = pcb->program_counter;
	//printf("los pm aca son %d", inst_pointer);
	elementoIndiceCodigo* coordenadas_instruccion =
			((elementoIndiceCodigo*) list_get(pcb->indiceCodigo, inst_pointer));
	//printf("el indice de codigo devolvio las coodernadas %d \n", coordenadas_instruccion->start);
//	printf("el indice de codigo devolvio las coodernadas %d \n", coordenadas_instruccion->offset);
	char * buffer = malloc(MAXBUF);

	enviarMensaje(&socketMemoria, serializarMensaje(4, 510, pcb->pid, coordenadas_instruccion->start, coordenadas_instruccion->offset));

	int result = recv(socketMemoria, buffer, MAXBUF, 0);

	if (result > 0) {

		//DE ESTA MANERA ME ASEGURO DE TOMAR LA INSTRUCCION Y NO BASURA
		char * instr = malloc(result);
		instr = string_substring(buffer, 0, result);

		printf("Se recibe la instruccion: %s\n", instr);
		//printf("Longitud Instruccion: %d\n", strlen(instr));
		//printf("\n");
		return instr;

	} else {
		printf("Error de comunicacion al recibir Instruccion a la Memoria\n");
		exit(errno);
	}
}

void manejador_signal(){
	puts("");
	puts("Se intercepto una señal");

	char* mensajeAKernel = string_new();

	string_append(&mensajeAKernel, "573");
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);
}

void imprimoInfoPCB(t_pcb * pcb) {
	printf("PID: %d\n", pcb->pid);
	printf("PC: %d\n", pcb->program_counter);
	printf("Quantum: %d\n", pcb->quantum);

	printf("Etiquetas:\n");

	int i = 0;

	while (i < pcb->etiquetas_size) {
		printf("[%d]", pcb->etiquetas[i]);
		i++;
	}

	printf("\n\n");
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

	 puts("");
	 */
}

t_pcb * reciboPCB(int * socketKernel) {
	t_pcb * pcb;
	char * buffer = malloc(MAXBUF);

	int result = recv(*socketKernel, buffer, MAXBUF, 0);

	//Se agrega manejo de error al recibir el PCB
	if (result > 0) {
		//INICIO CODIGO DE DESERIALIZACION DEL PCB
		pcb = deserializar_pcb(buffer);
		//FIN CODIGO DE DESERIALIZACION DEL PCB

	} else {
		printf("Error de comunicacion al recibir PCB\n");
		exit(errno);
	}

	return pcb;
}

t_pcb * deserializar_pcb(char * mensajeRecibido) {

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

	while (i < 13 + cantIndiceCodigo * 2) {
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

	if (pcb->etiquetas_size > 0) {
		pcb->etiquetas = malloc(pcb->etiquetas_size);
		while (i < j + pcb->etiquetas_size) {
			int ascii = atoi(message[i]);

			if (ascii >= 32 && ascii <= 126) {
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
			} else {
				i++;
			}
			char nombre_variable = atoi(message[i]);
			var->nombre_variable = nombre_variable;

			list_add(sta->variables, var);
			i++;
			cantVariables--;
		}

		int cantArgumentos = 0;
		if (message[i] != NULL){
			cantArgumentos = atoi(message[i]);
		}
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
			} else {
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

void* manejo_memoria(void * args) {
	struct sockaddr_in direccionMemoria;

	creoSocket(&socketMemoria, &direccionMemoria,
			inet_addr(configuracion->ip_memoria),
			configuracion->puerto_memoria);
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

void asignar(t_puntero direccion, t_valor_variable valor) {
	if (pcbHabilitado == true) {
		//printf("Asignar Valor %d en la Direccion %d\n", valor, direccion);
		printf("\n");

		if (direccion == -1) {

			puts("Variable no definida");
			char* mensajeAKernel = string_new();
			string_append(&mensajeAKernel, "778");
			string_append(&mensajeAKernel, ";");
			string_append(&mensajeAKernel, string_itoa(pcb->pid));
			string_append(&mensajeAKernel, ";");

			enviarMensaje(&sktKernel, mensajeAKernel);
			free(mensajeAKernel);
		} else {
			enviarMensaje(&socketMemoria,
									serializarMensaje(4, 511, pcb->pid, direccion, valor));

			/*

			if (direccion == VARIABLE_EN_CACHE) {
				enviarMensaje(&socketMemoria,
						serializarMensaje(6, 511, direccion, pcb->pid,
								pagina_a_leer_cache, offset_a_leer_cache,
								valor));
			} else {
				enviarMensaje(&socketMemoria,
						serializarMensaje(3, 511, direccion, valor));
			}
			*/

			char * buffer = malloc(MAXBUF);

			int result = recv(socketMemoria, buffer, MAXBUF, 0);

			if (result > 0) {

				char**mensajeDesdeMemoria = string_split(buffer, ";");
				int direccion = atoi(mensajeDesdeMemoria[0]);
				valor = atoi(mensajeDesdeMemoria[1]);

				printf("Asigne el valor %d en la direccion %d\n", valor,
											direccion);
									printf("\n");

				/*

				if (direccion != VARIABLE_EN_CACHE) {
					printf("Asigne el valor %d en la direccion %d\n", valor,
							direccion);
					printf("\n");
				} else {
					printf(
							"Asigne el valor %d en la pagina %d offset %d de la Cache\n",
							valor, pagina_a_leer_cache, offset_a_leer_cache);
					printf("\n");
				}
				*/

				free(mensajeDesdeMemoria);
			} else {
				printf("Error de comunicacion con Memoria al Asignar Valor\n");
				exit(errno);
			}

			free(buffer);

		}
	} else {
		printf(
				"Instruccion Asignar cancelada debido a finalizacion del programa\n");
	}
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	if (pcbHabilitado == true){
		printf("Obtener Posicion Variable %c\n", identificador_variable);
		puts("");

		int encontrar_var(t_variables* var) {
			return (var->nombre_variable == (char) identificador_variable);
		}

		t_variables* entrada_encontrada;

		t_Stack* StackDeContexto = list_get(pcb->indiceStack, (pcb->indiceStack->elements_count - 1));

		if (StackDeContexto == NULL){
			return -1;
		}

		if (esArgumentoDeFuncion(identificador_variable)) {
			entrada_encontrada = list_find(StackDeContexto->args, (void*) encontrar_var);
		}

		else {
			entrada_encontrada = list_find(StackDeContexto->variables, (void*) encontrar_var);
		}

		if (entrada_encontrada == NULL){
			//Si no encontro la variable retorna -1 acorde a la documentacion del parser
			return -1;
		}

		char * buffer = malloc(MAXBUF);

		enviarMensaje(&socketMemoria,
				serializarMensaje(4, 601, pcb->pid, entrada_encontrada->pagina,	entrada_encontrada->offset));

		int result = recv(socketMemoria, buffer, MAXBUF, 0);

		if (result > 0) {
			char**mensajeDesdeMemoria = string_split(buffer, ";");
			int direccion = atoi(mensajeDesdeMemoria[0]);

			/*
			if (valor == VARIABLE_EN_CACHE) {
				pagina_a_leer_cache = entrada_encontrada->pagina;
				offset_a_leer_cache = entrada_encontrada->offset;
				tamanio_a_leer_cache = entrada_encontrada->size;
			}
			*/

			return direccion;

		} else {
			printf("Error de comunicacion entre Memoria y CPU al Obtener Posicion Variable\n");
			exit(errno);
		}
	}else{
		printf("Instruccion Obtener Posicion Variable cancelada debido a finalizacion del programa\n");
		return 0;
	}
}

t_valor_variable dereferenciar(t_puntero direccion_variable) {

	printf("Dereferenciar Direccion %d\n", direccion_variable);
	printf("\n");

	if (direccion_variable == -1) {

		//Se asigna el exit code -20 (Sin definicion)
		puts("Variable no definida");
		char* mensajeAKernel = string_new();
		string_append(&mensajeAKernel, "778");
		string_append(&mensajeAKernel, ";");
		string_append(&mensajeAKernel, string_itoa(pcb->pid));
		string_append(&mensajeAKernel, ";");

		enviarMensaje(&sktKernel, mensajeAKernel);
		free(mensajeAKernel);

		return 0;
	} else {

		enviarMensaje(&socketMemoria, serializarMensaje(3, 513, pcb->pid, direccion_variable)); //// REVISAR MEMORIA

		char * buffer = malloc(MAXBUF);

		int result = recv(socketMemoria, buffer, MAXBUF, 0);

		if (result > 0) {
			char ** respuestaDeMemoria = string_split(buffer, ";");

			int codigo = atoi(respuestaDeMemoria[0]);

			if (codigo == DEREFERENCIAR_OK){

				printf("Lei el valor %s en la posicion %d\n", respuestaDeMemoria[1], direccion_variable);
				printf("\n");

				int valor = atoi(respuestaDeMemoria[1]);

				free(buffer);

				return valor;
			}
			else if (codigo == DEREFERENCIAR_ERROR) {

				pcbHabilitado= false;
				enviarMensaje(&sktKernel, serializarMensaje(2, 813, pcb->pid));

				return 0;
			}

		} else {
			printf("Error de comunicación con Memoria al Dereferenciar\n");
			exit(errno);
		}
	}
}

t_puntero definirVariable(t_nombre_variable identificador_variable) {
	printf("Definir Variable %c \n", identificador_variable);
	puts("");
	//printf("ahora el program counter es: %d\n", pcb->program_counter);
	char* mensajeAMemoria = string_new();
	string_append(&mensajeAMemoria, "512");
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, &identificador_variable);
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->pid));
	string_append(&mensajeAMemoria, ";");
	string_append(&mensajeAMemoria, string_itoa(pcb->cantidadPaginas)); ////REVISAR QUE ACTUALICE
	string_append(&mensajeAMemoria, ";");

	enviarMensaje(&socketMemoria, mensajeAMemoria);

	int result = recv(socketMemoria, mensajeAMemoria, MAXBUF, 0);

	if (result > 0) {

		char** mensajeSplit = string_split(mensajeAMemoria, ";");

		int codigo = atoi(mensajeSplit[0]);
		int paginaNueva = atoi(mensajeSplit[1]);

		if (codigo == ASIGNACION_MEMORIA_OK) {

			t_variables* entrada_stack;
			t_Stack* stackDeContexto;


			if (esArgumentoDeFuncion(identificador_variable)) {

				stackDeContexto = list_get(pcb->indiceStack, ((int) pcb->indiceStack->elements_count - 1));
				entrada_stack = deserializar_entrada_stack(mensajeSplit);
				list_add(stackDeContexto->args, entrada_stack);
				list_remove(pcb->indiceStack,(pcb->indiceStack->elements_count - 1));
				list_add(pcb->indiceStack, stackDeContexto);

				int posicionDeArgumento = obtenerPosicionVariable(identificador_variable);
				return posicionDeArgumento;
			}

			else {

				stackDeContexto = list_get(pcb->indiceStack,((int) pcb->indiceStack->elements_count - 1));
				entrada_stack = deserializar_entrada_stack(mensajeSplit);

				list_add(stackDeContexto->variables, entrada_stack);
				list_remove(pcb->indiceStack,(pcb->indiceStack->elements_count - 1));
				list_add(pcb->indiceStack, stackDeContexto);

			}

			if (paginaNueva == 1) {
				pcb->cantidadPaginas++;

				char * buffer = malloc(MAXBUF);

				enviarMensaje(&sktKernel, serializarMensaje(2, 777, pcb->pid));

				//paginaNueva = false;

				recv(sktKernel, buffer, MAXBUF, 0);

			}

			printf("La variable %c se guardo en la pag %d con offset %d\n\n",
					entrada_stack->nombre_variable, entrada_stack->pagina,
					entrada_stack->offset);

			free(mensajeAMemoria);

			return entrada_stack->offset;

		} else if (codigo == ASIGNACION_MEMORIA_ERROR) {

			//Se asigna el exit code -9 (No se pueden asignar mas paginas al proceso)
			enviarMensaje(&sktKernel, serializarMensaje(2, 655, pcb->pid));
			pcbHabilitado= false;

		} else if (codigo == STACK_OVERFLOW){
			puts("STACK OVERFLOW");
			enviarMensaje(&sktKernel, serializarMensaje(2, 809, pcb->pid));
			pcbHabilitado= false;
		}
	} else {
		printf("Error de comunicación con la Memoria durante la definicion de una variable\n");
		exit(errno);
	}


}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	puts("Obtener Valor Compartida");
	puts("");

	int valor;
	bool existe_var_compartida = false;
	char* msj = string_new();

	string_append(&msj, "514");
	string_append(&msj, ";");
	string_append(&msj, variable);
	string_append(&msj, ";");

	enviarMensaje(&sktKernel, msj);

	int result = recv(sktKernel, msj, MAXBUF, 0);

	if (result > 0) {

		char**mensajeDesdeKernel = string_split(msj, ";");

		existe_var_compartida = atoi(mensajeDesdeKernel[0]);

		if (existe_var_compartida == true){
			valor = atoi(mensajeDesdeKernel[1]);
			free(msj);
			printf("El valor de la compartida es: %d \n", valor);
			printf("\n");
			return valor;
		} else {
			enviarMensaje(&sktKernel, serializarMensaje(2, 810, pcb->pid));
			pcbHabilitado= false;
		}
	}
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor) {
	puts("Asignar Valor Compartida");
	puts("");

	char* msj = string_new();
	string_append(&msj, "515");
	string_append(&msj, ";");
	string_append(&msj, variable);
	string_append(&msj, ";");
	string_append(&msj, string_itoa(valor));
	string_append(&msj, ";");

	enviarMensaje(&sktKernel, msj);

	int result = recv(sktKernel, msj, MAXBUF, 0);

	if (result > 0) {

		char**mensajeDesdeKernel = string_split(msj, ";");

		bool existe_var_compartida = atoi(mensajeDesdeKernel[0]);

		if (existe_var_compartida == true){
			printf("Se asigno el valor %d \n", valor);
			printf("\n");
			return valor;
		} else {
			enviarMensaje(&sktKernel, serializarMensaje(2, 810, pcb->pid));
			pcbHabilitado= false;
		}
	}

	free(msj);
}

void irAlLabel(t_nombre_etiqueta identificador_variable) {
	puts("Ir a Label");
	puts("");

	if(pcb->exit_code < 1000){
	pcb->exit_code++;
	printf("ahora el program counter es: %d\n", pcb->program_counter);

	t_puntero_instruccion instruccion = metadata_buscar_etiqueta(trim(identificador_variable), pcb->etiquetas, pcb->etiquetas_size);
	pcb->program_counter = instruccion-1;
	}else{

		char* mensajeAKernel = string_new();
		string_append(&mensajeAKernel, "812");
		string_append(&mensajeAKernel, ";");
		string_append(&mensajeAKernel, string_itoa(pcb->pid));
		string_append(&mensajeAKernel, ";");

		enviarMensaje(&sktKernel, mensajeAKernel);

		pcbHabilitado= false;
		puts("aqui");
	}
	//pcb->program_counter++;

	//printf("ahora el program counter es: %d\n", pcb->program_counter);
	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta) {
	puts("Llamar Sin Retorno");
	puts("");

	printf("ahora el program counter es: %d\n", pcb->program_counter);
	//if(pcb->indiceStack->elements_count < 64)
	//{
		t_Stack* stackFuncion = malloc(sizeof(t_Stack));

		stackFuncion->stack_pointer = pcb->indiceStack->elements_count + 1;
		stackFuncion->args = list_create();
		stackFuncion->variables = list_create();
		list_add(pcb->indiceStack, stackFuncion);

		printf("Etiqueta: %s\n", trim(etiqueta));
		printf("\n");

	//	imprimoInfoPCB(pcb);

		t_puntero_instruccion instruccion = metadata_buscar_etiqueta(trim(etiqueta), pcb->etiquetas, pcb->etiquetas_size);
		pcb->program_counter = instruccion-1;

		if(instruccion == -1)
		{
			char* mensajeAKernel = string_new();
			string_append(&mensajeAKernel, "808");
			string_append(&mensajeAKernel, ";");
			string_append(&mensajeAKernel, string_itoa(pcb->pid));
			string_append(&mensajeAKernel, ";");

			enviarMensaje(&sktKernel, mensajeAKernel);
			free(mensajeAKernel);
		}
	//}
	//else
	//{
	//	pthread_mutex_unlock(&mutex_instrucciones);
	//	char* mensajeAKernel = string_new();
	//	string_append(&mensajeAKernel, "809");
	//	string_append(&mensajeAKernel, ";");
	//	string_append(&mensajeAKernel, string_itoa(pcb->pid));
	//	string_append(&mensajeAKernel, ";");

	//	enviarMensaje(&sktKernel, mensajeAKernel);

	//	pcbHabilitado= false;
	//}


	//pcb->program_counter++;
	//printf("ahora el program counter es: %d\n", pcb->program_counter);
	//printf("\n");
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
	puts("Llamar Con Retorno");
	puts("");

	printf("ahora el program counter es: %d\n", pcb->program_counter);
	printf("\n");
	//if(pcb->indiceStack->elements_count < 64)
	//{
		t_Stack* stackFuncion = malloc(sizeof(t_Stack));

		stackFuncion->retPost = pcb->program_counter;
		stackFuncion->retVar = donde_retornar;
		stackFuncion->stack_pointer = pcb->indiceStack->elements_count + 1;
		stackFuncion->args = list_create();
		stackFuncion->variables = list_create();
		list_add(pcb->indiceStack, stackFuncion);

		t_puntero_instruccion instruccion = metadata_buscar_etiqueta(trim(etiqueta), pcb->etiquetas, pcb->etiquetas_size);

		pcb->program_counter = instruccion - 1;

		//pcb->program_counter++;
		//printf("el valor de la instruccion es: %d\n", instruccion);
		//printf("\n");
		//free(stackFuncion);
		//printf("ahora el program counter es: %d\n", pcb->program_counter);

		if(instruccion == -1)
		{
			char* mensajeAKernel = string_new();
			string_append(&mensajeAKernel, "808");
			string_append(&mensajeAKernel, ";");
			string_append(&mensajeAKernel, string_itoa(pcb->pid));
			string_append(&mensajeAKernel, ";");

			enviarMensaje(&sktKernel, mensajeAKernel);
			pcbHabilitado= false;
		}
	//}
	//else
	//{
	//	pthread_mutex_unlock(&mutex_instrucciones);
	//	char* exitLlamada = string_new();
	//	string_append(&exitLlamada, "809");
	//	string_append(&exitLlamada, ";");
	//	string_append(&exitLlamada, string_itoa(pcb->pid));
	//	string_append(&exitLlamada, ";");

	//	enviarMensaje(&sktKernel, exitLlamada);
		//free(aux);
	//	pcbHabilitado= false;
	//}
}

void finalizar(void) {

	if(pcb->indiceStack->elements_count > 1)
	{
		pcb->indiceCodigo->elements_count = pcb->program_counter;
		pcbHabilitado = false;
		list_remove(pcb->indiceStack, pcb->indiceStack->elements_count - 1);
	}

	puts("FIN DEL PROGRAMA");
	puts("");
	puts("******************************");
	puts("");
	puts("");
	puts("");
	return;
}

void retornar(t_valor_variable retorno) {
	puts("Retornar");
	puts("");

	t_Stack* stackFuncion = malloc(sizeof(t_Stack*));
	stackFuncion = list_get(pcb->indiceStack, pcb->indiceStack->elements_count - 1);
	retorno = stackFuncion->retVar;
	t_puntero_instruccion instruccion = stackFuncion->retPost;

	list_remove(pcb->indiceStack, pcb->indiceStack->elements_count - 1);
	pcb->program_counter = instruccion;
	//pcb->program_counter++;
	return;
}

void wait(t_nombre_semaforo identificador_semaforo) {
	puts("Wait");
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "570");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, identificador_semaforo);
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);

	int valor_esperado = 0;

	while (valor_esperado == 0) {

		int result = recv(sktKernel, mensajeAKernel, MAXBUF, 0);

		if (result > 0) {

			char**mensajeDesdeKernel = string_split(mensajeAKernel, ";");

			int sem_existe = atoi(mensajeDesdeKernel[0]);

			if (sem_existe == true){

				int valor = atoi(mensajeDesdeKernel[1]);

				if (valor == 570) {
					valor_esperado = 1;
				} else if (valor == 577) {
					valor_esperado = 1;
					bloqueo = 1;
				}
			}
			else {
				valor_esperado = 1;
				enviarMensaje(&sktKernel, serializarMensaje(2, 811, pcb->pid));
				pcbHabilitado= false;
			}
			free(mensajeDesdeKernel);
		}
	}

	free(mensajeAKernel);

	return;
}

void signale(t_nombre_semaforo identificador_semaforo) {
	puts("Signal");
	puts("");

	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "571");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, identificador_semaforo);
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);

	int result = recv(sktKernel, mensajeAKernel, MAXBUF, 0);

	if (result > 0){
		char**mensajeDesdeKernel = string_split(mensajeAKernel, ";");
		int sem_existe = atoi(mensajeDesdeKernel[0]);

		if (sem_existe == false){
			enviarMensaje(&sktKernel, serializarMensaje(2, 811, pcb->pid));
			pcbHabilitado= false;
		}

		free(mensajeDesdeKernel);
	}

	free(mensajeAKernel);

	return;
}

t_puntero reservar(t_valor_variable espacio) {
	puts("Reservar");
	puts("");

	enviarMensaje(&sktKernel, serializarMensaje(3, 600, pcb->pid, espacio));
	//printf("Envie a Kernel: %s\n",
		//	serializarMensaje(3, 600, pcb->pid, espacio));

	//printf("Espero a que el Kernel me mande la direccion\n");
	char * buffer = malloc(MAXBUF);

	int result = recv(sktKernel, buffer, MAXBUF, 0);
	//printf("Buffer que viene del Kernel: %s\n", buffer);

	if (result > 0) {
		char ** respuesta = string_split(buffer, ";");
		if (strcmp(respuesta[0], "606") == 0){
			printf("Direccion Puntero: %d\n", atoi(respuesta[1]));
			printf("\n");
			return atoi(respuesta[1]);
		}else{
			enviarMensaje(&sktKernel, serializarMensaje(2, 617, pcb->pid));
			printf("Error reservando Memoria de Heap, espacio insuficiente\n");
			//FORMA CABEZA DE TERMINARLO
			pcbHabilitado = false;
			return 0;
		}
	} else {
		printf("Error de comunicacion reservando Memoria de Heap\n");
		exit(errno);
	}
}

void liberar(t_puntero puntero) {
	printf("Liberar Direccion %d\n", puntero);
	printf("\n");

	enviarMensaje(&sktKernel, serializarMensaje(3, 700, pcb->pid, puntero));

	char * buffer = malloc(MAXBUF);
	int result = recv(sktKernel, buffer, MAXBUF, 0);

	if (result > 0) {
		char ** respuestaDeKernel = string_split(buffer, ";");
		if (strcmp(respuestaDeKernel[0], "710") == 0){
			printf("Se eliminó la reserva de memoria ubicada en %d correctamente\n", puntero);
			printf("\n");
		}else{
			printf("Error liberando Memoria de Heap, memoria inexistente\n");

			printf("\n");
			enviarMensaje(&sktKernel, serializarMensaje(2, 618, pcb->pid));
			//FORMA CABEZA DE TERMINARLO
			pcbHabilitado = false;
		}
	} else {
		printf("Error de comunicacion con el Kernel liberando Memoria de Heap\n");
		exit(errno);
	}
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags) {
	puts("Abrir");
	puts("");

	char* flagEnviar = string_new();
	if(flags.creacion != 0)
	{
		string_append(&flagEnviar,"c");
	}
	if(flags.escritura)
	{
		string_append(&flagEnviar,"w");
	}
	if(flags.lectura)
	{
		string_append(&flagEnviar,"r");
	}

	char* mensajeAbrirEnviar = string_new();
	string_append(&mensajeAbrirEnviar, "803");
	string_append(&mensajeAbrirEnviar, ";");
	string_append(&mensajeAbrirEnviar, string_itoa(pcb->pid));
	string_append(&mensajeAbrirEnviar, ";");
	string_append(&mensajeAbrirEnviar, ((char*) (direccion)));
	string_append(&mensajeAbrirEnviar, ";");
	string_append(&mensajeAbrirEnviar, flagEnviar);
	string_append(&mensajeAbrirEnviar, ";");
    //printf("la direccion es %s", direccion);
	enviarMensaje(&sktKernel, mensajeAbrirEnviar);

	t_descriptor_archivo retorno;

	char* mensajeAbrirRespuesta =  malloc(MAXBUF);
	int result = recv(sktKernel, mensajeAbrirRespuesta, MAXBUF, 0);

	if (result > 0) {
		if(string_contains(mensajeAbrirRespuesta,"Finalización"))
		{
			pcbHabilitado = false;
			retorno = 0;
			free(mensajeAbrirEnviar);
			free(mensajeAbrirRespuesta);
		}
		else
		{
			puts("El Archivo se abrió correctamente \n");
			int fdNuevo = atoi(mensajeAbrirRespuesta);
			printf("El file descriptor es %d \n", fdNuevo);
			 retorno = fdNuevo;
			 puts("\n");


		free(mensajeAbrirEnviar);
		free(mensajeAbrirRespuesta);
		}

	} else {
		printf("Error al abrir el archivo \n");
		retorno = 0;
	}
	return retorno;
}

void borrar(t_descriptor_archivo descriptor) {
	puts("Borrar");
	puts("");
	if(descriptor <= 0 || descriptor > 99)
	{
		descriptor = -1;
	}
	char* mensajeAKernel = string_new();
	string_append(&mensajeAKernel, "802");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(pcb->pid));
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(descriptor));
	string_append(&mensajeAKernel, ";");
	//puts("por enviar");
	enviarMensaje(&sktKernel, mensajeAKernel);
	//puts("envidado");
	//recv(sktKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

	//int result =recv(sktKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

	//recv(sktKernel, mensajeAKernel, MAXBUF, 0);
	char* resulMenBorrar = malloc(MAXBUF);
	int result = recv(sktKernel, resulMenBorrar, MAXBUF, 0);

	if (result > 0) {
		if(string_contains(resulMenBorrar,"Finalización"))
		{
			pcbHabilitado = false;

		}
		else
		{
			printf("%s \n",resulMenBorrar);

		}

	} else {
		printf("Error no se pudo borrar el archivo \n");
	}
	//free(mensajeAKernel);
	free(resulMenBorrar);
	free(mensajeAKernel);
	return;
}
void cerrar(t_descriptor_archivo descriptor) {
	puts("Cerrar");
		puts("");
if(descriptor <= 0 || descriptor > 99)
{
	descriptor = -1;
}
		char* mensajeCerrar= string_new();
		string_append(&mensajeCerrar, "801");
		string_append(&mensajeCerrar, ";");
		string_append(&mensajeCerrar, string_itoa(pcb->pid));
		string_append(&mensajeCerrar, ";");
		string_append(&mensajeCerrar, string_itoa(descriptor));
		string_append(&mensajeCerrar, ";");

		enviarMensaje(&sktKernel, mensajeCerrar);

		//recv(sktKernel, mensajeAKernel, MAXBUF, 0);
		char* resulMenCerrar = malloc(MAXBUF);
		int result = recv(sktKernel, resulMenCerrar, MAXBUF, 0);

		if (result > 0) {
			if(string_contains(resulMenCerrar,"Finalización"))
			{
				pcbHabilitado = false;

			}
			else
			{
				printf("%s \n",resulMenCerrar);
			}

		} else {
			printf("Error el archivo no se pudo cerrar \n");
		}
		//free(mensajeAKernel);
		//free(mensajeCerrar);
		return;
}

void moverCursor(t_descriptor_archivo descriptor_archivo,
		t_valor_variable posicion) {
	/*puts("Mover Cursor");
	puts("");

	char* mensajeAKernel = string_new();

	string_append(&mensajeAKernel, "805");
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(descriptor_archivo));
	string_append(&mensajeAKernel, ";");
	string_append(&mensajeAKernel, string_itoa(posicion));
	string_append(&mensajeAKernel, ";");

	enviarMensaje(&sktKernel, mensajeAKernel);

	//free(mensajeAKernel);

	puts("Cursor movido en el archivo");*/
	
	return;
}
void escribir(t_descriptor_archivo descriptor_archivo, void * informacion, t_valor_variable tamanio) {
	puts("Escribir");
	puts("");
	if(descriptor_archivo < 0 || descriptor_archivo > 99)
	{
		descriptor_archivo = -1;
	}
	if(descriptor_archivo == 0 || descriptor_archivo == 1) //esto es porque si me mandan un string para la consola lo tengo que trabajar distinto.
	{
		//printf("su contenido es %s \n", informacion);
		char* mensajeFs = string_new();
		string_append(&mensajeFs, "804");
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(descriptor_archivo));
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(pcb->pid));
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, (char *) informacion);
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(tamanio));
		string_append(&mensajeFs, ";");

		enviarMensaje(&sktKernel, mensajeFs);
	}
	else
	{
		char* mensajeFs = string_new();
		string_append(&mensajeFs, "804");
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(descriptor_archivo));
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(pcb->pid));
		string_append(&mensajeFs, ";");
		string_append(&mensajeFs, string_itoa(tamanio));
		string_append(&mensajeFs, ";");
		  printf("su contenido es %s \n", informacion);
		enviarMensaje(&sktKernel, mensajeFs);

		 recv(sktKernel, mensajeFs, 12, 0);
		 send(sktKernel, informacion, tamanio, 0);
			char* resulMenEscribir = malloc(MAXBUF);
			int result = recv(sktKernel, resulMenEscribir, MAXBUF, 0);

			if (result > 0) {
				if(string_contains(resulMenEscribir,"Finalización"))
				{
					pcbHabilitado = false;

				}
				else
				{
					printf("%s \n",resulMenEscribir);
				}

			} else {
				printf("Error el archivo no se pudo escribir \n");
			}

			free(resulMenEscribir);
	}


	//int result = recv(sktKernel, mensajeAKernel, sizeof(mensajeAKernel), 0);

	//free(mensajeAKernel);


	//free(mensajeFs);
	return;
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio) {
	puts("Leer");
	puts("");
	if(descriptor_archivo < 0 || descriptor_archivo > 99)
	{
		descriptor_archivo = -1;
	}
	char* mensajeFs = string_new();
	string_append(&mensajeFs, "800");
	string_append(&mensajeFs, ";");
	string_append(&mensajeFs, string_itoa(descriptor_archivo));
	string_append(&mensajeFs, ";");
	string_append(&mensajeFs, string_itoa(pcb->pid));
	string_append(&mensajeFs, ";");
	string_append(&mensajeFs, string_itoa(tamanio));
	string_append(&mensajeFs, ";");

	enviarMensaje(&sktKernel, mensajeFs);
	 recv(sktKernel, mensajeFs, 12, 0);
	 send(sktKernel, informacion, tamanio, 0);

	char* resulMenLeer = malloc(MAXBUF);
	recv(sktKernel, resulMenLeer, MAXBUF, 0);
	if(string_contains(resulMenLeer, "9000"))
	{
		enviarMensaje(&sktKernel, "todo ok");
		pcbHabilitado = false;
	}
	else
	{
		enviarMensaje(&sktKernel, "todo ok");
		void* bufferFin = malloc(tamanio);
		recv(sktKernel, bufferFin, tamanio, 0);
		asignar(informacion, bufferFin);
		puts("El archivo se leyo correctamente\n");
			//printf("%s \n", resulMenLeer);
	   printf("su contenido es %s \n", bufferFin);

		free(resulMenLeer);
		free(mensajeFs);
	}

	return;

}

char* serializar_pcb(t_pcb* pcb) {

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
	for (i = 0; i < pcb->indiceCodigo->elements_count; i++) {
		elementoIndiceCodigo * elem = malloc(sizeof(elem));
		elem = list_get(pcb->indiceCodigo, i);

		string_append(&mensajeACPU, string_itoa(elem->start));
		string_append(&mensajeACPU, ";");

		string_append(&mensajeACPU, string_itoa(elem->offset));
		string_append(&mensajeACPU, ";");
	}

	//printf("Etiquetas Serializacion: \n");

	for (i = 0; i < pcb->etiquetas_size; i++) {
		string_append(&mensajeACPU, string_itoa(pcb->etiquetas[i]));
		string_append(&mensajeACPU, ";");
		//printf("[%d]", pcb->etiquetas[i]);
	}

	//printf("\n");

	for (i = 0; i < pcb->indiceStack->elements_count; i++) {
		t_Stack *sta = malloc(sizeof(t_Stack));
		sta = list_get(pcb->indiceStack, i);

		string_append(&mensajeACPU, string_itoa(sta->retPost));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->retVar));
		string_append(&mensajeACPU, ";");
		string_append(&mensajeACPU, string_itoa(sta->stack_pointer));
		string_append(&mensajeACPU, ";");
		int j;

		string_append(&mensajeACPU,
				string_itoa(sta->variables->elements_count)); //lo necesito para deserializar
		string_append(&mensajeACPU, ";");
		for (j = 0; j < sta->variables->elements_count; j++) {

			t_variables* var = list_get(sta->variables, j);

			string_append(&mensajeACPU, string_itoa(var->offset));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->pagina));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->size));
			string_append(&mensajeACPU, ";");
			if (var->nombre_funcion != NULL) {
				string_append(&mensajeACPU, string_itoa(var->nombre_funcion));
			} else {
				string_append(&mensajeACPU,
						string_itoa(CONST_SIN_NOMBRE_FUNCION));
			}
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(var->nombre_variable));
			string_append(&mensajeACPU, ";");
		}

		string_append(&mensajeACPU, string_itoa(sta->args->elements_count)); //lo necesito para deserializar
		string_append(&mensajeACPU, ";");
		int k;

		for (k = 0; k < sta->args->elements_count; k++) {

			t_variables* args = list_get(sta->args, k);

			string_append(&mensajeACPU, string_itoa(args->offset));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->pagina));
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->size));
			string_append(&mensajeACPU, ";");
			if (args->nombre_funcion != NULL) {
				string_append(&mensajeACPU, string_itoa(args->nombre_funcion));
			} else {
				string_append(&mensajeACPU,
						string_itoa(CONST_SIN_NOMBRE_FUNCION));
			}
			string_append(&mensajeACPU, ";");
			string_append(&mensajeACPU, string_itoa(args->nombre_variable));
			string_append(&mensajeACPU, ";");

		}
	}

	return mensajeACPU;

}
int esArgumentoDeFuncion(t_nombre_variable identificador_variable)
{
	switch((char)identificador_variable){
	 		case '0':
	 			;
	  		return 1;
	 			break;
	 		case '1':
	 			;
	 			return 1;
	 			break;
	 		case '2':
	 			;
	 			return 1;
	 			break;
	 		case '3':
	 			;
	 			return 1;
	 			break;
	 		case '4':
	 			;
	 			return 1;
	 			break;
	 		case '5':
	 			return 1;
	 			break;
	 		case '6':
	 			;
	 			return 1;
	 			break;
	 		case '7':
	    		;
	 			return 1;
	 			break;
	 		case '8':
	 			;
	 			return 1;
	 			break;
	 		case '9':
	 			;
	 			return 1;
	 			break;
	 		default:
	 			return 0;

	 		}
}

t_variables* deserializar_entrada_stack(char** mensajeRecibido) {
	t_variables* entrada_stack = malloc(sizeof(t_variables));

	entrada_stack->nombre_variable = mensajeRecibido[2][0];
	entrada_stack->pagina = atoi(mensajeRecibido[3]);
	entrada_stack->offset = atoi(mensajeRecibido[4]);
	entrada_stack->size = atoi(mensajeRecibido[5]);

	return entrada_stack;
}
