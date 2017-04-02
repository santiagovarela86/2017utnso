//CODIGOS
//300 CON A KER - HANDSHAKE
//101 KER A CON - RESPUESTA HANDSHAKE

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
    char* codigo;

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

	message[0] = '3';
	message[1] = '0';
	message[2] = '0';
	message[3] = ';';

    if(send(sock , message , strlen(message) , 0) < 0)
    {
        puts("Fallo el envio al servidor");
        return EXIT_FAILURE;
    }

	while((recv(sock, message, sizeof(message), 0)) > 0)
	{
		codigo = strtok(message, ";");

		if(atoi(codigo) == 101){
			printf("El kernel acepto la conexion \n");
		}else{
			printf("Conexion rechazada \n");
			return EXIT_FAILURE;
		}

		puts("");
		puts("***********************************************************");
		puts("Ingrese numero de la acción a realizar");
		puts("1) Iniciar programa");
		puts("2) Finalizar programa");
		puts("3) Desconectar");
		puts("4) Limpar");
		puts("***********************************************************");

		int numero = 0;

		scanf("%d",&numero);

		if(numero == 1){
			puts("Ingrese nombre del programa");

			char directorio[1000];

			scanf("%s", directorio);

			FILE *ptr_fich1 = fopen(directorio, "r");

		    int num;
		    char buffer[1000 + 1];
		    while(!feof(ptr_fich1)){

		      num = fread(buffer,sizeof(char), 1000 + 1, ptr_fich1);
		      buffer[num*sizeof(char)] = '\0';
		      printf("%s.",buffer);
		    }

		    printf("\n");

		}

	}

    //Loop para seguir comunicado con el servidor
    while(1)
    {


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
