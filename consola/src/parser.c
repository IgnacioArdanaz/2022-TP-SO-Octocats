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

void leerParametros(FILE *archivo, int cantParametros,int32_t* arg1, int32_t* arg2){
	int j;
	char* string_aux = string_new();
	int32_t args[2] = {-1,-1};
	for(j=0;j<cantParametros;j++){
		fscanf(archivo, "%s",string_aux);
		validarParametro(string_aux);
		//si es valido el parametro lo concatena a la instruccion
		args[j] = atoi(string_aux);
	}
	*arg1 = args[0];
	*arg2 = args[1];
	free(string_aux);
}

instruccion_t* instruccion_crear(){
	instruccion_t* inst = malloc(sizeof(instruccion_t));
	inst->arg1 = -1;
	inst->arg2 = -1;
	return inst;
}

void iterarNOOP(FILE *archivo, t_list* codigo){
	int repeticiones;
	char* string_aux = string_new();
	fscanf(archivo, "%s",string_aux);
	validarParametro(string_aux);
	repeticiones = atoi(string_aux);
	printf("cant de NOOP: %d\n",repeticiones);
	for(int i=0;i<repeticiones;i++){
		//iteramos hasta cantidad de repeticiones menos uno para al final
		//utilizar el puntero leido
		instruccion_t* inst = instruccion_crear();
		inst->operacion = 'N'; //N de NOOP
		list_add(codigo, inst);
	}
	free(string_aux);
}

void parser(char* path, t_list* codigo) {

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
        char operacion = auxP[0];
        if(operacion == 'N'){
        	iterarNOOP(archivo, codigo);
        }
        else{
        	instruccion_t* inst = instruccion_crear();
        	inst->operacion = operacion;
        	//en leerParametros directamente modifica las variables de la instruccion
        	leerParametros(archivo,cant,&inst->arg1,&inst->arg2);
        	list_add(codigo,inst);
        }

    }

    printf("\n------------------\nParser termino bien\n------------------\n\n");
    free(auxP);
    fclose(archivo);

}
