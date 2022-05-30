#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include <structures.h>

typedef struct {
	char *operacion;
	int cantParametros;
} t_identificador;


#define CANT_IDENTIFICADORES (sizeof(tablaIdentificadores)/sizeof(t_identificador))
/*estamos dividiendo la cantidad de bytes que ocupa la tabla por lo que ocupa un identificador
 * obteniendo la cantidad de identificadores, capaz se podria hacer con size?
*/
instruccion_t* instruccion_crear();
int cantParametros(char *instruccion);

int isNumber(char s[]);

void validarParametro(char *auxP);

void leerParametro(FILE *archivo, int cantParametros,int32_t* arg1, int32_t* arg2);

void iterarNOOP(FILE *archivo, t_list* codigo);

void parser(char* path, t_list* codigo);


#endif
