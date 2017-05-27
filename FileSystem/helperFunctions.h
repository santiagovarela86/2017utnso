/*
 * socketHelper.h
 *
 *  Created on: 2/4/2017
 *      Author: utnso
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#define MAXBUF 2048
#define MAXLIST 20

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto);
void bindSocket(int * sock, struct sockaddr_in * direccion);
void escuchoSocket(int * sock);
void conectarSocket(int * sock, struct sockaddr_in * direccion);
void enviarMensaje(int * sock, char * message);
void handShakeListen(int * socketCliente, char * codigoEsperado, char * codigoAceptado, char * codigoRechazado, char * componente);
void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args);
void handShakeSend(int * socketServer, char * codigoEnvio, char * codigoEsperado, char * componente);
char *trim(char *s);
char *rtrim(char *s);
char *ltrim(char *s);
char * serializarMensaje(int cant, ... );
int substr_count (char* string,char *string2);

#endif /* HELPERFUNCTIONS_H_ */
