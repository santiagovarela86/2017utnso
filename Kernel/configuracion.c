#include "configuracion.h"


void imprimir_config(Kernel* kernel){

	printf("PUERTO_PROG: %d\n", kernel->puerto_programa);
	printf("PUERTO_CPU: %d\n", kernel->puerto_cpu);
	printf("IP_MEMORIA: %s\n", kernel->ip_memoria);
	printf("PUERTO_MEMORIA: %d\n", kernel->puerto_memoria);
	printf("IP_FS: %s\n", kernel->ip_fs);
	printf("PUERTO_FS: %d\n", kernel->puerto_fs);
	printf("QUANTUM: %d\n", kernel->quantum);
	printf("QUANTUM_SLEEP: %d\n", kernel->quantum_sleep);
	printf("ALGORITMO: %s\n", kernel->algoritmo);
	printf("GRADO_MULTIPROG: %d\n", kernel->grado_multiprogramacion);

	int i = 0;
	while (kernel->sem_ids[i] != NULL) {
		printf("SEM_IDS: %s\n",kernel->sem_ids[i]);
		i++;
	}

	i = 0;
	while (kernel->sem_init[i] != NULL) {
		printf("SEM_INIT: %s\n",kernel->sem_init[i]);
		i++;
	}

	i= 0;
	while (kernel->shared_vars[i] != NULL){
		printf("SHARED_VARS: %s\n", kernel->shared_vars[i]);
		i++;
	}
	printf("STACK_SIZE: %d\n", kernel->stack_size);
}

Kernel* cargar_config(char* path){

	Kernel* kernel = malloc(sizeof(Kernel));

	t_config* metadata = config_create(path);

	if(path == NULL){
		exit(EXIT_FAILURE);
	}

	kernel->puerto_programa = config_get_int_value(metadata, "PUERTO_PROG");
	kernel->puerto_cpu = config_get_int_value(metadata, "PUERTO_CPU");
	kernel->ip_memoria = config_get_string_value(metadata, "IP_MEMORIA");
	kernel->puerto_memoria = config_get_int_value(metadata, "PUERTO_MEMORIA");
	kernel->ip_fs = config_get_string_value(metadata, "IP_FS");
	kernel->puerto_fs = config_get_int_value(metadata, "PUERTO_FS");
	kernel->quantum = config_get_int_value(metadata, "QUANTUM");
	kernel->quantum_sleep = config_get_int_value(metadata, "QUANTUM_SLEEP");
	kernel->algoritmo = config_get_string_value(metadata, "ALGORITMO");
	kernel->grado_multiprogramacion = config_get_int_value(metadata, "GRADO_MULTIPROG");
	kernel->sem_ids = config_get_array_value(metadata, "SEM_IDS");
	kernel->sem_init = config_get_array_value(metadata, "SEM_INIT");
	kernel->shared_vars = config_get_array_value(metadata, "SHARED_VARS");
	kernel->stack_size = config_get_int_value(metadata, "STACK_SIZE");

	imprimir_config(kernel);

	return kernel;
}

