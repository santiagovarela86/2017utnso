/*
 * socketHelper.h
 *
 *  Created on: 2/4/2017
 *      Author: utnso
 */

#ifndef SOCKETHELPER_H_
#define SOCKETHELPER_H_

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto);
void bindSocket(int * sock, struct sockaddr_in * direccion);
void escuchoSocket(int * sock);
void conectarSocket(int * sock, struct sockaddr_in * direccion);

#endif /* SOCKETHELPER_H_ */
