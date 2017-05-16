#include "cache.h"
#include <stdlib.h>

t_entrada_cache *crear_entrada_cache(char *contenido_pagina, int nro_pagina, int pid){
	t_entrada_cache* new = malloc(sizeof(t_entrada_cache));
	new->contenido_pagina = contenido_pagina;
	new->nro_pagina = nro_pagina;
	new->pid = pid;
	return new;
}

void destruir_entrada_cache(t_entrada_cache *self){
	free(self->contenido_pagina);
	free(self);
}

void flush_memoria_cache(t_list* cache){
	 list_destroy_and_destroy_elements(cache, (void*)destruir_entrada_cache);
}



