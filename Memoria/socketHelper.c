#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

void creoSocket(int * sock, struct sockaddr_in * direccion, in_addr_t ip, int puerto) {

	*sock = socket(AF_INET, SOCK_STREAM, 0);

	if (*sock < 0) {
		perror("Socket");
		exit(errno);
	}

	memset(direccion, 0, sizeof(*direccion));

	direccion->sin_family = AF_INET;
	direccion->sin_addr.s_addr = ip;
	direccion->sin_port = htons(puerto);
}

void bindSocket(int * sock, struct sockaddr_in * direccion) {

	int resultado = bind(*sock, direccion, sizeof(*direccion));

	if (resultado != 0) {
		perror("error en bind");
		exit(errno);
	}
}

void escuchoSocket(int * sock) {

	int resultado = listen(*sock, 20);

	if (resultado != 0) {
		perror("error en listen");
		exit(errno);
	}
}

void conectarSocket(int * sock, struct sockaddr_in * direccion){

	int resultado = connect(* sock, (struct sockaddr *) direccion, sizeof(* direccion));

	if (resultado < 0) {
		perror("Fallo el intento de conexion al servidor\n");
		exit(errno);
	}
}

void enviarMensaje(int * sock, char * message){
	int resultado = send(* sock, message, strlen(message), 0);

	if (resultado < 0) {
		puts("Fallo el enviar mensaje");
		exit(errno);
	}
}
