#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>

typedef struct {
	char* ip;
	int puerto;
} Conector;

Conector* leer_configuracion(char* directorio);
void imprimir_configuracion(Conector* conector);

int main(int argc , char **argv)
{
    int sock;
    struct sockaddr_in server;
    char message[1000] = "";
    char server_reply[2000] = "";

    if (argc <= 1)
    {
    	printf("Error. Parametros incorrectos \n");
    	return EXIT_FAILURE;
    }

    Conector* conect = malloc (sizeof(Conector));

	conect = leer_configuracion(argv[1]);

    char* ip = conect->ip;
    int port = conect->puerto;

    //Creacion de Socket
    sock = socket(AF_INET , SOCK_STREAM , 0);

    if (sock == -1)
    {
        printf("Error. No se pudo crear el socket de conexion");
        return EXIT_FAILURE;
    }

    puts("Socket de conexion creado correctamente");

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    //Conexion al Servidor
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Fallo el intento de conexion al servidor");
        return EXIT_FAILURE;
    }

    puts("Conectado al servidor\n");

    //Loop para seguir comunicado con el servidor
    while(1)
    {
    	message[0] = '3';
    	message[1] = '0';
    	message[2] = '0';
    	message[3] = ';';

        if(send(sock , message , strlen(message) , 0) < 0)
        {
            puts("Fallo el envio al servidor");
            return EXIT_FAILURE;
        }

        //Verifico si hubo respuesta del servidor
        if(recv(sock, server_reply , 2000 , 0) < 0)
        {
            puts("Desconexion del cliente");
            break;
        }

    }

    close(sock);
    return EXIT_SUCCESS;
}

void imprimir_configuracion(Conector* conector){
	printf("IP_KERNEL: %s\n", conector->ip);
	printf("PUERTO_KERNEL: %d\n", conector->puerto);
}

Conector* leer_configuracion(char* directorio){
	char* path = string_new();

    string_append(&path,directorio);

	t_config* config_consola = config_create(path);

	Conector* conect = malloc(sizeof(Conector));

	conect->ip = config_get_string_value(config_consola, "IP_KERNEL");

	conect->puerto = config_get_int_value(config_consola,"PUERTO_KERNEL");

	imprimir_configuracion(conect);

	return conect;
}
