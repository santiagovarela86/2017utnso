/*
 * socketHelper.h
 *
 *  Created on: 2/4/2017
 *      Author: utnso
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#define MAXBUF 1024

typedef struct threadSocketInfo {
	int sock;
	struct sockaddr_in direccion;
} threadSocketInfo;

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto);
void bindSocket(int * sock, struct sockaddr_in * direccion);
void escuchoSocket(int * sock);
void conectarSocket(int * sock, struct sockaddr_in * direccion);
void enviarMensaje(int * sock, char * message);
void handShake(int * socketServer, int * socketCliente, int codigoEsperado, int codigoAceptado, int codigoRechazado, char * componente);
void creoThread(pthread_t * threadID, void *(*threadHandler)(void *), void * args);

#endif /* HELPERFUNCTIONS_H_ */
