#include "cache.h"
#include "memoria.h"
#include <stdlib.h>

void destruir_entrada_cache(t_entrada_cache *self){
	free(self->contenido_pagina);
	free(self);
}

void flush_memoria_cache(t_list* cache){
	 list_destroy_and_destroy_elements(cache, (void*)destruir_entrada_cache);
	 cache = list_create();
}



