
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
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
t_puntero alocar(t_valor_variable espacio);
void liberar(t_puntero puntero);
t_puntero reservar(t_valor_variable espacio);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo direccion);
void cerrar(t_descriptor_archivo descriptor_archivo);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);

void inicializar_funciones(AnSISOP_funciones* funciones, AnSISOP_kernel* kernel);

#endif /* HELPERPARSER_H_ */
