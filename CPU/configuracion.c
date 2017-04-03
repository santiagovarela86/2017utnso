#include "configuracion.h"

CPU_Config* leerConfiguracion(char* path) {

	if (path == NULL) {
		exit(errno);
	}

	t_config* metadata = config_create(path);
	CPU_Config* cpu = malloc(sizeof(CPU_Config));

	cpu->ip_kernel = config_get_string_value(metadata, "IP_KERNEL");
	cpu->puerto_kernel = config_get_int_value(metadata, "PUERTO_KERNEL");
	cpu->ip_memoria = config_get_string_value(metadata, "IP_MEMORIA");
	cpu->puerto_memoria = config_get_int_value(metadata, "PUERTO_MEMORIA");

	//imprimirConfiguracion(cpu);

	return cpu;
}

void imprimirConfiguracion(CPU_Config* configuracion) {
	printf("IP_KERNEL: %s\n", configuracion->ip_kernel);
	printf("PUERTO_KERNEL: %d\n", configuracion->puerto_kernel);
	printf("IP_MEMORIA: %s\n", configuracion->ip_memoria);
	printf("PUERTO_MEMORIA: %d\n", configuracion->puerto_memoria);
}
