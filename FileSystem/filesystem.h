/*
 * filesystem.h
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

void * hilo_conexiones_kernel(void * args);


typedef struct {
	int fileDescriptor;
	char* flags;
	int global_fd;
} t_fileProceso;

typedef struct {
	char* path;
	FILE* referenciaArchivo;
} t_archivosFileSystem;

typedef struct {
	int tamanio;
	t_list* bloquesEscritos;
}t_metadataArch;

typedef struct {
	char* archivoMapeado;
	struct stat script;
}t_mapeoArchivo;

void * handler_conexion_kernel(void * sock);

void inicializarEstructuras(char * pathConfig);
void liberarEstructuras();
void obtener_datos(char* directorio, int size, char* buffer, int offset);
int validar_archivo(char* directorio);
void crear_archivo(char* directorio, char* flag);
void borrarArchivo(char* directorio);
void guardar_datos(char* directorio, int size, char* buffer, int offset);
void crearMetadataSadica(char* montaje);
void crearBloques(char* montaje, int cantidad);
int buscarPrimerBloqueLibre();
t_metadataArch* leerMetadataDeArchivoCreado(char* arch);
void cerrarUnArchivoBloque(char* pmap, struct stat script);
void actualizarArchivoCreado(t_metadataArch* regArchivo, t_archivosFileSystem* arch);
t_mapeoArchivo* abrirUnArchivoBloque(int idBloque);
void grabarUnArchivoBloque(t_mapeoArchivo* archBloque, int idBloque, char* buffer, int size);
void pidoBloquesEnBlancoYgrabo(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size );
void grabarParteEnbloquesYparteEnNuevos(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size );
void graboEnLosBloquesQueYaTiene(int offset, t_metadataArch* regMetaArchBuscado, char* buffer, int size );
#endif /* FILESYSTEM_H_ */

