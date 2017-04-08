/*
 * pruebasParser.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "helper.h"

int main(int argc, char **argv) {
	//char * programa = "a = a + b";

	//Esto lo puede implementar la consola, la cual envía el programa al Kernel
	//Y luego el CPU recibe el programa por sockets, a efectos de probar el parser
	//se lee desde un archivo
	char * programa = leerArchivo("/home/utnso/git/tp-2017-1c-Nene-Malloc/PruebasParser/Debug/facil.ansisop");

	t_metadata_program * metadataPrograma;
	metadataPrograma = metadata_desde_literal(programa);

	procesoLineas(programa);

	free(programa);

	return EXIT_SUCCESS;
}
