/*
 * helper.c

 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "helperParser.h"
#include "parser/parser.h"

/*
 * DEFINIR VARIABLE
 *
 * Reserva en el Contexto de Ejecución Actual el espacio necesario para una variable llamada identificador_variable y la registra tanto en el Stack como en el Diccionario de Variables. Retornando la posición del valor de esta nueva variable del stack
 * El valor de la variable queda indefinido: no deberá inicializarlo con ningún valor default.
 * Esta función se invoca una vez por variable, a pesar que este varias veces en una línea.
 * Ej: Evaluar "variables a, b, c" llamará tres veces a esta función con los parámetros "a", "b" y "c"
 *
 * @sintax	TEXT_VARIABLE (variables identificador[,identificador]*)
 * @param	identificador_variable	Nombre de variable a definir
 * @return	Puntero a la variable recien asignada
 */

t_puntero definirVariable(t_nombre_variable identificador_variable) {
	t_puntero ret = 0;

	printf("Defino la variable %c\n", identificador_variable);

	return ret;
}

/*
 * OBTENER POSICION de una VARIABLE
 *
 * Devuelve el desplazamiento respecto al inicio del segmento Stacken que se encuentra el valor de la variable identificador_variable del contexto actual.
 * En caso de error, retorna -1.
 *
 * @sintax	TEXT_REFERENCE_OP (&)
 * @param	identificador_variable 		Nombre de la variable a buscar (De ser un parametro, se invocara sin el '$')
 * @return	Donde se encuentre la variable buscada
 */

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	t_puntero ret = 0;

	printf("Obtengo la posicion de la variable %c\n", identificador_variable);

	return ret;
}

/*
 * DEREFERENCIAR
 *
 * Obtiene el valor resultante de leer a partir de direccion_variable, sin importar cual fuera el contexto actual
 *
 * @sintax	TEXT_DEREFERENCE_OP (*)
 * @param	direccion_variable	Lugar donde buscar
 * @return	Valor que se encuentra en esa posicion
 */

t_valor_variable dereferenciar(t_puntero direccion_variable) {
	t_valor_variable ret = 0;

	printf("Desreferencio la direccion %d\n", direccion_variable);

	return ret;
}

/*
 * ASIGNAR
 *
 * Inserta una copia del valor en la variable ubicada en direccion_variable.
 *
 * @sintax	TEXT_ASSIGNATION (=)
 * @param	direccion_variable	lugar donde insertar el valor
 * @param	valor	Valor a insertar
 * @return	void
 */

void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	printf("Asigno valor %d a la direccion %d\n", valor, direccion_variable);
}

/*
 * OBTENER VALOR de una variable COMPARTIDA
 *
 * Pide al kernel el valor (copia, no puntero) de la variable compartida por parametro.
 *
 * @sintax	TEXT_VAR_START_GLOBAL (!)
 * @param	variable	Nombre de la variable compartida a buscar
 * @return	El valor de la variable compartida
 */

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	t_valor_variable ret = 0;

	printf("Obtengo el valor de la variable que esta ubicada en %c\n", variable);

	return ret;
}

/*
 * ASIGNAR VALOR a variable COMPARTIDA
 *
 * Pide al kernel asignar el valor a la variable compartida.
 * Devuelve el valor asignado.
 *
 * @sintax	TEXT_VAR_START_GLOBAL (!) IDENTIFICADOR TEXT_ASSIGNATION (=) EXPRESION
 * @param	variable	Nombre (sin el '!') de la variable a pedir
 * @param	valor	Valor que se le quire asignar
 * @return	Valor que se asigno
 */

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor) {
	t_valor_variable ret = 0;

	printf("Asigno Valor Compartida\n");

	return ret;
}

/*
 * IR a la ETIQUETA
 *
 * Cambia la linea de ejecucion a la correspondiente de la etiqueta buscada.
 *
 * @sintax	TEXT_GOTO (goto )
 * @param	t_nombre_etiqueta	Nombre de la etiqueta
 * @return	void
 */

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta) {

	printf("Voy al Label\n");

}

/*
 * LLAMAR SIN RETORNO
 *
 * Preserva el contexto de ejecución actual para poder retornar luego al mismo.
 * Modifica las estructuras correspondientes para mostrar un nuevo contexto vacío.
 *
 * Los parámetros serán definidos luego de esta instrucción de la misma manera que una variable local, con identificadores numéricos empezando por el 0.
 *
 * @sintax	Sin sintaxis particular, se invoca cuando no coresponde a ninguna de las otras reglas sintacticas
 * @param	etiqueta	Nombre de la funcion
 * @return	void
 */

void llamarSinRetorno(t_nombre_etiqueta etiqueta) {
	printf("Llamo sin Retorno\n");
}

/*
 * LLAMAR CON RETORNO
 *
 * Preserva el contexto de ejecución actual para poder retornar luego al mismo, junto con la posicion de la variable entregada por donde_retornar.
 * Modifica las estructuras correspondientes para mostrar un nuevo contexto vacío.
 *
 * Los parámetros serán definidos luego de esta instrucción de la misma manera que una variable local, con identificadores numéricos empezando por el 0.
 *
 * @sintax	TEXT_CALL (<-)
 * @param	etiqueta	Nombre de la funcion
 * @param	donde_retornar	Posicion donde insertar el valor de retornos
 * @return	void
 */

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
	printf("Llamo con Retorno\n");
}

/*
 * FINALIZAR
 *
 * Cambia el Contexto de Ejecución Actual para volver al Contexto anterior al que se está ejecutando, recuperando el Cursor de Contexto Actual y el Program Counter previamente apilados en el Stack.
 * En caso de estar finalizando el Contexto principal (el ubicado al inicio del Stack), deberá finalizar la ejecución del programa.
 *
 * @sintax	TEXT_END (end )
 * @param	void
 * @return	void
 */

void finalizar(void) {
	printf("Finalizo\n");
}

/*
 * RETORNAR
 *
 * Cambia el Contexto de Ejecución Actual para volver al Contexto anterior al que se está ejecutando, recuperando el Cursor de Contexto Actual, el Program Counter y la direccion donde retornar, asignando el valor de retornos en esta, previamente apilados en el Stack.
 *
 * @sintax	TEXT_RETURN (return )
 * @param	retornos	Valor a ingresar en la posicion corespondiente
 * @return	void
 */

void retornar(t_valor_variable retorno) {
	printf("Retorno\n");
}

/*
 * IMPRIMIR VALOR
 *
 * Envía valor_mostrar al Kernel, para que termine siendo mostrado en la consola del Programa en ejecución.
 *
 * @sintax	TEXT_PRINT_NUMBER (print )
 * @param	valor_mostrar	Dato que se quiere imprimir
 * @return	void
 */

void imprimirValor(t_valor_variable valor_mostrar) {
	printf("Imprimo valor\n");
}

/*
 * IMPRIMIR LITERAL
 *
 * Envía mensaje al Kernel, para que termine siendo mostrado en la consola del Programa en ejecución.
 * El mensaje no posee parámetros, secuencias de escape, variables ni nada.
 *
 * @sintax TEXT_PRINT_LITERAL (textPrint )
 * @param	texto	Texto a imprimir
 * @return void
 */

void imprimirLiteral(char* texto) {
	printf("Imprimo Literal\n");
}

/*
 *	ENTRADA y SALIDA
 *
 *
 *	@sintax TEXT_IO (io )
 *	@param	dispositivo	Nombre del dispositivo a pedir
 *	@param	tiempo	Tiempo que se necesitara el dispositivo
 *	@return	void
 */

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	printf("Entrada Salida\n");
}

/*
 * WAIT
 *
 * Informa al kernel que ejecute la función wait para el semáforo con el nombre identificador_semaforo.
 * El kernel deberá decidir si bloquearlo o no.
 *
 * @sintax	TEXT_WAIT (wait )
 * @param	identificador_semaforo	Semaforo a aplicar WAIT
 * @return	void
 */

void wait(t_nombre_semaforo identificador_semaforo) {
	printf("Wait\n");
}

/*
 * SIGNAL
 *
 * Informa al kernel que ejecute la función signal para el semáforo con el nombre identificador_semaforo.
 * El kernel deberá decidir si desbloquear otros procesos o no.
 *
 * @sintax	TEXT_SIGNAL (signal )
 * @param	identificador_semaforo	Semaforo a aplicar SIGNAL
 * @return	void
 */
void signal(t_nombre_semaforo identificador_semaforo) {
	printf("Signal\n");
}

/*
 * RESERVAR
 *
 * Informa al kernel que reserve en el Heap una cantidad de memoria acorde al espacio recibido como parametro
 *
 * @sintax	TEST_MALLOC space
 * @param	valor_variable Cantidad de espacio
 * @return	puntero a donde esta reservada la memoria
 */
t_puntero alocar(t_valor_variable espacio) {
	t_puntero ret = 0;

	printf("Aloco memoria\n");

	return ret;
}

/*
 * LIBERAR
 *
 * Informa al kernel que libera la memoria previamente reservada con RESERVAR.
 * Solo se podra liberar memoria previamente asignada con RESERVAR.
 *
 * @sintax	TEST_FREE variable
 * @param	puntero Inicio de espacio de memoria a liberar (previamente retornado por RESERVAR)
 * @return	void
 */
void liberar(t_puntero puntero) {
	printf("Libero Memoria\n");
}

void procesoLineas(char * programa){ //tendria que usar el string_iterate_lines ???
	AnSISOP_funciones * funciones = NULL;
	AnSISOP_kernel * kernel = NULL;

	funciones = malloc(sizeof(AnSISOP_funciones));
	kernel = malloc(sizeof(AnSISOP_kernel));

	//ESTO VA ACA?
	//SE SUPONE QUE ESTAS FUNCIONES TIENEN QUE MODIFICAR EL STACK?
	//DEBERIAN SER GLOBALES EN EL PROCESO CPU?

	funciones->AnSISOP_asignar = asignar;
	funciones->AnSISOP_asignarValorCompartida = asignarValorCompartida;
	funciones->AnSISOP_definirVariable = definirVariable;
	funciones->AnSISOP_dereferenciar = dereferenciar;
	funciones->AnSISOP_entradaSalida = entradaSalida;
	funciones->AnSISOP_finalizar = finalizar;
	funciones->AnSISOP_imprimirLiteral = imprimirLiteral;
	funciones->AnSISOP_imprimirValor = imprimirValor;
	funciones->AnSISOP_irAlLabel = irAlLabel;
	funciones->AnSISOP_llamarConRetorno = llamarConRetorno;
	funciones->AnSISOP_llamarSinRetorno = llamarSinRetorno;
	funciones->AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable;
	funciones->AnSISOP_obtenerValorCompartida = obtenerValorCompartida;
	funciones->AnSISOP_retornar = retornar;
	kernel->AnSISOP_alocar = alocar;
	kernel->AnSISOP_liberar = liberar;
	kernel->AnSISOP_signal = signal;
	kernel->AnSISOP_wait = wait;

	char * curLine = programa;
	   while(curLine)
	   {
	      char * nextLine = strchr(curLine, '\n');
	      if (nextLine){
	    	  *nextLine = '\0';
	      }


	      if (meInteresa(curLine)){//(!esComentario(curLine) && !esBegin(curLine) && !esNewLine(curLine)){
	    	  printf("Linea Actual: %s\n\n", curLine);
	    	  analizadorLinea(curLine, funciones, kernel);
	      }

	      if (nextLine){
	    	  *nextLine = '\n';
	      }

	      curLine = nextLine ? (nextLine+1) : NULL;
	   }
}

bool meInteresa(char * linea){
	return (!esComentario(linea) && !esBegin(linea) && !esNewLine(linea) && !esVacia(linea));
}

bool esVacia(char* linea){
	return string_is_empty(linea);
}

bool esComentario(char* linea){
	//return string_starts_with(linea, TEXT_COMMENT); me pincha porque esta entre comillas simples?
	return string_starts_with(linea, "#");
}

bool esBegin(char* linea){
	return string_starts_with(linea, TEXT_BEGIN);
}

bool esNewLine(char* linea){
	return string_starts_with(linea, "\n");

	//return string_starts_with(linea, "\n");
}

char * leerArchivo(char * path){
	FILE * fp;
	long lSize;
	char * buffer;

	fp = fopen (path , "r");
	if( !fp ) {
		perror("No se pudo abrir el archivo");
		exit(errno);
	}

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	buffer = calloc( 1, lSize+1 );
	if( !buffer ){
		fclose(fp);
		perror("No se pudo reservar memoria");
		exit(errno);
	}

	if ( 1!=fread( buffer , lSize, 1 , fp) ){
		fclose(fp);
		free(buffer);
		perror("No se pudo leer el archivo");
		exit(errno);
	}

	fclose(fp);

	return buffer;
}
