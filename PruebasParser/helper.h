/*
 * helper.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>

#include "parser/parser.h"
#include "parser/metadata_program.h"

char * leerArchivo(char * path);
void procesoLineas(char * programa);
bool esComentario(char* linea);
bool esBegin(char* linea);
bool esEnd(char* linea);

#endif /* HELPER_H_ */
