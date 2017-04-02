//CODIGOS
//201 MEM A KER - RESPUESTA HANDSHAKE
//202 MEM A CPU - RESPUESTA HANDSHAKE
//299 MEM A OTR	- RESPUESTA DE CONEXION INCORRECTA
//100 KER A MEM - HANDSHAKE DE KERNEL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>

typedef struct {
	int puerto;
	int marcos;
	int marco_size;
	int entradas_cache;
	int cache_x_proc;
	char* reemplazo_cache;
	int retardo_memoria;
} Configuracion;

typedef struct{
	int socket_consola;
} estructura_socket;

Configuracion* leer_configuracion(char* directorio);
void* handler_conexion(void *socket_desc);
int sumarizador_conecciones;

int main(int argc , char **argv)
{
    int sock;
    struct sockaddr_in server;
    char message[1000] = "";
    char server_reply[2000] = "";

    int socket_desc, client_sock, c;
    struct sockaddr client;
    sumarizador_conecciones = 0;

    if (argc <= 1)
    {
    	printf("Error. Parametros incorrectos \n");
    	return EXIT_FAILURE;
    }

    Configuracion* config = malloc (sizeof(Configuracion));

	config = leer_configuracion(argv[1]);

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1)
	{
		printf("Error. No se pudo crear el socket de conexion");
	}

	// struct de administracion del socket
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(config->puerto);

	// Asigno la IP y puerto al socket
	if(bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Error en el bind");
		return 1;
	}

	// El servidor se queda esperando a que se conecten los clientes
	listen(socket_desc, 3);

	while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))){
		pthread_t thread_id;
		estructura_socket es;
		es.socket_consola = client_sock;
		if(pthread_create( &thread_id, NULL,  handler_conexion, (void*) &es) < 0)
		{
			perror("Error al crear el Hilo");
			return 1;
		}
	}

	if (client_sock < 0)
	{
		perror("Fallo en la conexion");
		return 1;
	}

	return 0;
}

void* handler_conexion(void *socket_desc){
	sumarizador_conecciones++;
	printf("Tengo %d Conectados \n", sumarizador_conecciones);

	estructura_socket* est_socket = malloc(sizeof(estructura_socket));
	est_socket = (estructura_socket*) socket_desc;

	char consola_message[1000] = "";
	char* codigo;

	while((recv(est_socket->socket_consola, consola_message, sizeof(consola_message), 0)) > 0)
	{
		codigo = strtok(consola_message, ";");

		if(atoi(codigo) == 100){
			printf("Se acepto la conexion del Kernel \n");

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '1';
			consola_message[3] = ';';

		    if(send(est_socket->socket_consola, consola_message, strlen(consola_message) , 0) < 0)
		    {
		        puts("Fallo el envio al servidor");
		        return EXIT_FAILURE;
		    }

		}else if (atoi(codigo) == 500){
			printf("Se acepto la conexion de la CPU \n");

			consola_message[0] = '2';
			consola_message[1] = '0';
			consola_message[2] = '2';
			consola_message[3] = ';';

		    if(send(est_socket->socket_consola, consola_message, strlen(consola_message) , 0) < 0)
		    {
		        puts("Fallo el envio al servidor");
		        return EXIT_FAILURE;
		    }
		}else{
			printf("Se rechazo una conexion incorrecta \n");

			consola_message[0] = '2';
			consola_message[1] = '9';
			consola_message[2] = '9';
			consola_message[3] = ';';

		    if(send(est_socket->socket_consola, consola_message, strlen(consola_message) , 0) < 0)
		    {
		        puts("Fallo el envio al servidor");
		        return EXIT_FAILURE;
		    }
		}

	}

	while(1){}

	return 0;
}

Configuracion* leer_configuracion(char* directorio){
	char* path = string_new();

    string_append(&path,directorio);

	t_config* config_memoria = config_create(path);

	Configuracion* config = malloc(sizeof(Configuracion));

	config->cache_x_proc = config_get_int_value(config_memoria, "CACHE_X_PROC");
	config->entradas_cache = config_get_int_value(config_memoria, "ENTRADAS_CACHE");
	config->marco_size = config_get_int_value(config_memoria, "MARCO_SIZE");
	config->marcos = config_get_int_value(config_memoria, "MARCOS");
	config->puerto = config_get_int_value(config_memoria, "PUERTO");
	config->reemplazo_cache = config_get_string_value(config_memoria, "REEMPLAZO_CACHE");
	config->retardo_memoria = config_get_int_value(config_memoria, "RETARDO_MEMORIA");

	imprimir_configuracion(config);

	return config;
}

void imprimir_configuracion(Configuracion* config) {
	printf("CACHE_X_PROC: %d \n", config->cache_x_proc);
	printf("ENTRADAS_CACHE: %d\n", config->entradas_cache);
	printf("MARCO_SIZE: %d\n", config->marco_size);
	printf("PUERTO: %d\n", config->puerto);
	printf("REEMPLAZO_CACHE: %s\n", config->reemplazo_cache);
	printf("RETARDO_MEMORIA: %d\n", config->retardo_memoria);
}
