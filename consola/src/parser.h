#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>

typedef struct {
	char *operacion;
	int cantParametros;
} t_identificador;


#define CANT_IDENTIFICADORES (sizeof(tablaIdentificadores)/sizeof(t_identificador))
/*estamos dividiendo la cantidad de bytes que ocupa la tabla por lo que ocupa un identificador
 * obteniendo la cantidad de identificadores, capaz se podria hacer con size?
*/
int cantParametros(char *instruccion);

int isNumber(char s[]);

void leerParametro(FILE *archivo,char *auxP);

int parser(char* path,char*** tabla);


#endif
