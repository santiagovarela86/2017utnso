/*
 * helper.c

 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "helper.h"

char * leerArchivo(char * path){
	FILE * fp;
	long lSize;
	char * buffer;

	fp = fopen (path , "r");
	if( !fp ) {
		perror("No se pudo abrir el archivo");
		exit(errno);
	}

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	buffer = calloc( 1, lSize+1 );
	if( !buffer ){
		fclose(fp);
		perror("No se pudo reservar memoria");
		exit(errno);
	}

	if ( 1!=fread( buffer , lSize, 1 , fp) ){
		fclose(fp);
		free(buffer);
		perror("No se pudo leer el archivo");
		exit(errno);
	}

	fclose(fp);

	return buffer;
}

void procesoLineas(char * programa){ //tendria que usar el string_iterate_lines ??? // que pasa con los comentarios y los begin? el analizador no los detecta
	AnSISOP_funciones funcANSI;
	AnSISOP_kernel kernelANSI;

	char * curLine = programa;
	   while(curLine)
	   {
	      char * nextLine = strchr(curLine, '\n');
	      if (nextLine){
	    	  *nextLine = '\0';
	      }

	      //printf("curLine=[%s]\n", curLine);

	      if (!esComentario(curLine) && !esBegin(curLine)){
	    	  analizadorLinea(curLine, &funcANSI, &kernelANSI);
	      }

	      if (nextLine){
	    	  *nextLine = '\n';
	      }

	      curLine = nextLine ? (nextLine+1) : NULL;
	   }
}

bool esComentario(char* linea){
	return string_starts_with(linea, TEXT_COMMENT);
}

bool esBegin(char* linea){
	return string_starts_with(linea, TEXT_BEGIN);
}
