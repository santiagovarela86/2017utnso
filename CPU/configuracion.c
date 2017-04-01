#include "configuracion.h"

void imprimir_config(CPU_Config* config){
	printf("IP_KERNEL: %s\n", config->ip_kernel);
	printf("PUERTO_KERNEL: %d\n", config->puerto_kernel);
	printf("IP_MEMORIA: %s\n", config->ip_memoria);
	printf("PUERTO_MEMORIA: %d\n", config->puerto_memoria);
}

CPU_Config* cargar_config(char* path){

	CPU_Config* cpu_config = malloc(sizeof(CPU_Config));

	t_config* metadata = config_create(path);

	if(path == NULL){
		exit(EXIT_FAILURE);
	}

	cpu_config->ip_kernel = config_get_string_value(metadata,"IP_KERNEL");
	cpu_config->puerto_kernel = config_get_int_value(metadata,"PUERTO_KERNEL");
	cpu_config->ip_memoria = config_get_string_value(metadata,"IP_MEMORIA");
	cpu_config->puerto_memoria = config_get_int_value(metadata,"PUERTO_MEMORIA");

	imprimir_config(cpu_config);

	return cpu_config;

}

