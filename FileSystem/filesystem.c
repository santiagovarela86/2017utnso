#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "configuracion.h"

#define MAXBUF 1024

int main(int argc, char** argv){

    if (argc == 1)
    {
    	printf("Error. Parametros incorrectos \n");
    	return EXIT_FAILURE;
    }

	int sockfd;
	struct sockaddr_in self;
	char buffer[MAXBUF];

    char* path = argv[1];

    FileSystem_Config* config = cargar_config(path);

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Socket");
		exit(errno);
	}

	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(config->puerto);
	self.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	{
		perror("error en bind");
		exit(errno);
	}

	if ( listen(sockfd, 20) != 0 )
	{
		perror("error en listen");
		exit(errno);
	}

	while (1)
	{	int clientfd;
		struct sockaddr_in client_addr;
		int addrlen=sizeof(client_addr);

		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		printf("%s:%d conectado\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		send(clientfd, buffer, recv(clientfd, buffer, MAXBUF, 0), 0);

		close(clientfd);
	}



	close(sockfd);
	return 0;



	return EXIT_SUCCESS;
}

