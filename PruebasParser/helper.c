/*
 * helper.c

 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */


#include "helper.h"

//Esto lo puede implementar la consola, la cual envía el programa al Kernel
//Y luego el CPU recibe el programa por sockets, a efectos de probar el parser
//se lee desde un archivo
char * leerArchivo(char * path){
	FILE * fp;
	long lSize;
	char * buffer;

	fp = fopen (path , "r");
	if( !fp ) {
		perror("No se pudo abrir el archivo\n");
		exit(errno);
	}

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	buffer = calloc( 1, lSize+1 );
	if( !buffer ){
		fclose(fp);
		perror("No se pudo reservar memoria\n");
		exit(errno);
	}

	if ( 1!=fread( buffer , lSize, 1 , fp) ){
		fclose(fp);
		free(buffer);
		perror("No se pudo leer el archivo\n");
		exit(errno);
	}

	fclose(fp);

	return buffer;
}

void procesoLineas(char * programa){
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

	      if (!esComentario(curLine)){
	    	  analizadorLinea(curLine, &funcANSI, &kernelANSI);
	      }

	      if (nextLine){
	    	  *nextLine = '\n';
	      }

	      curLine = nextLine ? (nextLine+1) : NULL;
	   }
}

bool esComentario(char* linea){
	return string_starts_with(linea, "#");
}




/*
void procesoLineas(char * programa) {
	  char *p, *temp;
	  p = strtok_r(programa, "\n", &temp);
	  do {
	      //printf("current line = %s", p);
		  if (!esComentario(p)){
			analizadorLinea(curLine, &funcANSI, &kernelANSI);
		  }
	  } while ((p - strtok_r(NULL, "\n", &temp) != NULL));
}
*/

