#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>

/*
 *  Para guardar las instrucciones podemos usar string_split (biblioteca commons)
 *  si es que guardamos la instruccion completa como string,
 *  almacenando estos en un array
 */
/*
 * TODO investigar si exit puede devolver el tipo de error
 * crear nuestros propios #define como EXIT_FAILURE pero mas descriptivos
 */


typedef struct {
	char *operacion;
	int cantParametros;
} t_identificador;

static t_identificador tablaIdentificadores[] = {
        {"NO_OP",1},{"I/O",1}, {"READ",1},
        {"COPY",2},{"WRITE",2},{"EXIT",0}
};

#define CANT_IDENTIFICADORES (sizeof(tablaIdentificadores)/sizeof(t_identificador))
/*estamos dividiendo la cantidad de bytes que ocupa la tabla por lo que ocupa un identificador
 * obteniendo la cantidad de identificadores, capaz se podria hacer con size?
*/

int cantParametros(char *instruccion){
    for(int i=0;i < CANT_IDENTIFICADORES;i++){
        t_identificador sym;
        sym = tablaIdentificadores[i];
        if(strcmp(sym.operacion,instruccion)==0) return sym.cantParametros;
    }
    printf("error operacion invalida\n"); //si no es un operacion valido devuelve -1
    exit(EXIT_FAILURE);
    /*
     * si quisieramos saber si hay un parametro sobrante, podriamos hacer una verificacion
     * si lo que lee es numerico (basicamente piensa que viene una operacion pero en realiadad
     * es un parametro extra en alguna instruccion)
     */
}

int isNumber(char s[])
{
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}

void leerParametro(FILE *archivo,char *auxP){
	fscanf(archivo, "%s",auxP);
		if(!isNumber(auxP)){
	    	printf("error parametro invalido\n");
	    	exit(1); //error 1: parametro invalido
	    }
}

int parser(int argc, char** argv) {
    if(argc < 2) {
            return EXIT_FAILURE;
        }

    char *auxP = malloc(sizeof(char));
    int cant;
    int i;
    FILE *archivo;
    archivo= fopen(argv[1],"r");

    while(!feof(archivo)){
        //leemos la primer palabra de la instruccion (una operacion)
        fscanf(archivo, "%s",auxP);

        cant = cantParametros(auxP);

        //segun que operacion deba realizar va a leer X cantidad de parametros
        for(i=0;i<cant;i++){

        	leerParametro(archivo,auxP);
        }
    }
    printf("Termino bien\n");
    free(auxP);
    fclose(archivo);

}
