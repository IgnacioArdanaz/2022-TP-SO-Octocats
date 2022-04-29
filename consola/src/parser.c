#include "parser.h"

/*
 *  Para guardar las instrucciones podemos usar string_split (biblioteca commons)
 *  si es que guardamos la instruccion completa como string,
 *  almacenando estos en un array
 */
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
    /*
     * si quisieramos saber si hay un parametro sobrante, podriamos hacer una verificacion
     * si lo que lee es numerico (basicamente piensa que viene una operacion pero en realiadad
     * es un parametro extra en alguna instruccion)
     */
}

int isNumber(char s[]){
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}

void leerParametros(FILE *archivo,char** instruccion, char *auxP, int cantParametros){
	int j;
	for(j=0;j<cantParametros;j++){
		fscanf(archivo, "%s",auxP);
		if(!isNumber(auxP)){
			    	printf("error parametro invalido\n");
			    	exit(1); //error 1: parametro invalido
			    }
		string_append_with_format(instruccion," %s",auxP);
	}



}

void parser(char* path, char*** tabla) {

	char *auxP = string_new();

    int cant;
    int i;
    FILE *archivo;
    printf("asd %s\n",path);
    archivo= fopen(path,"r");
    int inst=0;
    while(!feof(archivo)){
        //leemos la primer palabra de la instruccion (una operacion)
        fscanf(archivo, "%s",auxP);
        cant = cantParametros(auxP);
        inst++;
        //guardar operacion
        //instruccion = auxP;
        char *instruccion = string_new();
        strcpy(instruccion, auxP);
        //segun que operacion deba realizar va a leer X cantidad de parametros
        //for(i=0;i<cant;i++){
        	leerParametros(archivo,&instruccion,auxP,cant);
        	printf("auxP: %s\n",auxP);
        	//guardar parametro
        	//string_append_with_format(&instruccion," %s",auxP);
        	printf("info: %s\n",instruccion);
        //}
        string_array_push(tabla, instruccion);
        for(i=0;i<cant;i++){
        	printf("info tabla: %s\n",**tabla);
        }
    }
    printf("Termino bien\n");

    fclose(archivo);

}
