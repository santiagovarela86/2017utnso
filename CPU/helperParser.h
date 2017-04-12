/*
 * helper.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef HELPERPARSER_H_
#define HELPERPARSER_H_

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
bool esNewLine(char* linea);
bool meInteresa(char * linea);
bool esVacia(char* linea);

t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void finalizar(void);
void retornar(t_valor_variable retorno);
void imprimirValor(t_valor_variable valor_mostrar);
void imprimirLiteral(char* texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
t_puntero alocar(t_valor_variable espacio);
void liberar(t_puntero puntero);

#endif /* HELPERPARSER_H_ */
