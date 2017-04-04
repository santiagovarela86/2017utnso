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
#include "configuracion.h"
#include "socketHelper.h"

void iniciar_programa();

int main(int argc , char **argv)
{

	if (argc != 2)
    {
    	printf("Error. Parametros incorrectos.\n");
    	return EXIT_FAILURE;
    }

	Consola_Config* configuracion;
	int socketConsola;
    struct sockaddr_in direccionConsola;
    char message[1000] = "";
    char server_reply[2000] = "";
    char* codigo;

    configuracion = leerConfiguracion(argv[1]);
    imprimirConfiguracion(configuracion);

    creoSocket(&socketConsola, &direccionConsola, inet_addr(configuracion->ip_kernel), configuracion->puerto_kernel);
    puts("Socket de Consola creado correctamente\n");

    conectarSocket(&socketConsola, &direccionConsola);
    puts("Conectado al Kernel\n");

    strcpy(message, "300;");

	//message[0] = '3';
	//message[1] = '0';
	//message[2] = '0';
	//message[3] = ';';

	enviarMensaje(&socketConsola, message);

	while((recv(socketConsola, message, sizeof(message), 0)) > 0)
	{
		codigo = strtok(message, ";");

		if(atoi(codigo) == 101){
			printf("El Kernel acepto la conexion\n");
		}else{
			printf("El Kernel rechazo la conexion\n");
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
		int numero_correcto = 0 ;
		int intentos_fallidos = 0;

		while(numero_correcto == 0 && intentos_fallidos < 3){
			scanf("%d",&numero);

			if(numero == 1){
				numero_correcto = 1;

				iniciar_programa(socketConsola);

				}else{
					intentos_fallidos++;
					puts("Por ahora solo funciona si ingresa 1");
				}
			}

			if(intentos_fallidos == 3){
				return EXIT_FAILURE;
			}

		}

    //Loop para seguir comunicado con el servidor
    while(1)
    {


        //Verifico si hubo respuesta del servidor
        if(recv(socketConsola, server_reply , 2000 , 0) < 0)
        {
            puts("Desconexion del cliente");
            break;
        }

    }

    close(socketConsola);
    return EXIT_SUCCESS;
}

void iniciar_programa(int socket){

	puts("Ingrese nombre del programa");

	char directorio[1000];

	scanf("%s", directorio);

	FILE *ptr_fich1 = fopen(directorio, "r");

	int num;
	char buffer[1000 + 1];

	while(!feof(ptr_fich1)){
		num = fread(buffer,sizeof(char), 1000 + 1, ptr_fich1);
		buffer[num*sizeof(char)] = '\0';

	}

	enviarMensaje(&socket, buffer);

	return;
}
