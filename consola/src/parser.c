#include "parser.h"
/*
 * TODO investigar si exit puede devolver el tipo de error
 * crear nuestros propios #define como EXIT_FAILURE pero mas descriptivos
 */
static t_identificador tablaIdentificadores[] = {
        {"NO_OP",1},{"I/O",1}, {"READ",1},
        {"COPY",2},{"WRITE",2},{"EXIT",0}
};

int cantParametros(char *instruccion){
    for(int i=0;i < CANT_IDENTIFICADORES;i++){
        t_identificador sym;
        sym = tablaIdentificadores[i];
        if(strcmp(sym.operacion,instruccion)==0) return sym.cantParametros;
    }
    printf("error operacion invalida\n"); //si no es un operacion valido devuelve -1
    exit(EXIT_FAILURE);
}

int isNumber(char s[]){
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}

void validarParametro(char *auxP){
	if(!isNumber(auxP)){
		printf("error parametro invalido\n");
		exit(1); //error 1: parametro invalido
	}
}

void leerParametros(FILE *archivo,char** instruccion, char *auxP, int cantParametros){
	int j;
	for(j=0;j<cantParametros;j++){
		fscanf(archivo, "%s",auxP);
		validarParametro(auxP);
		//si es valido el parametro lo concatena a la instruccion
		string_append_with_format(instruccion," %s",auxP);
	}
}

void iterarNOOP(FILE *archivo, char* instruccion, char *auxP, char*** tabla){
	int repeticiones;
	fscanf(archivo, "%s",auxP);
	validarParametro(auxP);
	repeticiones = atoi(auxP);
	printf("cant de NOOP: %d\n",repeticiones);
	for(int i=0;i<repeticiones-1;i++){
		//iteramos hasta cantidad de repeticiones menos uno para al final
		//utilizar el puntero leido
		char *copia = string_duplicate(instruccion);
		string_array_push(tabla, copia);
	}
	string_array_push(tabla, instruccion);
}

void parser(char* path, char*** tabla) {
	// Puntero que nos ayuda a leer cada palabra
	char *auxP = string_new();
    int cant; // Cantidad de parametros
    FILE *archivo;
    archivo= fopen(path,"r");

    while(!feof(archivo)){
        //leemos la primer palabra de la instruccion (una operacion)
        fscanf(archivo, "%s",auxP);
        //asignamos la cantidad de parametros que tiene la operacion
        cant = cantParametros(auxP);
        //creamos el puntero de la instruccion
        char *instruccion = string_new();
        //guardar la operacion en el puntero instruccion
        strcpy(instruccion, auxP);

        if(strcmp(instruccion, "NO_OP")==0){
        	iterarNOOP(archivo, instruccion, auxP, tabla);
        }else{
        	leerParametros(archivo,&instruccion,auxP,cant);
        	string_array_push(tabla, instruccion);
        }
    }

    printf("\n------------------\nParser termino bien\n------------------\n\n");
    free(auxP);
    fclose(archivo);

}
