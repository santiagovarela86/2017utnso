/*
 * pruebasParser.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "helperParser.h"

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Error. Parametros incorrectos.\n");
		return EXIT_FAILURE;
	}

	//char * programa = leerArchivo("/home/utnso/git/tp-2017-1c-Nene-Malloc/PruebasParser/Debug/facil.ansisop");
	char * programa = leerArchivo(argv[1]);

	t_metadata_program * metadataPrograma;
	metadataPrograma = metadata_desde_literal(programa);

	procesoLineas(programa);

	free(programa);

	return EXIT_SUCCESS;
}
