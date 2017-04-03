#include "configuracion.h"

Consola_Config* leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);
	Consola_Config* consola = malloc(sizeof(Consola_Config));

	consola->ip_kernel = config_get_string_value(metadata, "IP_KERNEL");
	consola->puerto_kernel = config_get_int_value(metadata, "PUERTO_KERNEL");

	return consola;
}

void imprimirConfiguracion(Consola_Config* configuracion) {
	printf("IP_KERNEL: %s\n", configuracion->ip_kernel);
	printf("PUERTO_KERNEL: %d\n", configuracion->puerto_kernel);
}
