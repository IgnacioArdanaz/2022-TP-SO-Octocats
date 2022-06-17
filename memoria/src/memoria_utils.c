#include "memoria_utils.h"

t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;
uint16_t retardo_swap;


void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");

	tam_memoria = config_get_double_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_double_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_double_value(config,"ENTRADAS_POR_TABLA");
	retardo_memoria = config_get_double_value(config,"RETARDO_MEMORIA");
	marcos_por_proceso = config_get_double_value(config,"MARCOS_POR_PROCESO");
	retardo_swap = config_get_double_value(config,"RETARDO_SWAP");


	lista_tablas_1er_nivel = list_create();
	lista_tablas_2do_nivel = list_create();

}

uint32_t inicializar_estructuras_proceso(int tamanio){
	uint32_t numero_tabla = crear_tabla_1er_nivel();
	crear_tablas_2do_nivel(numero_tabla);
	return numero_tabla; //Es el que se guarda el PCB
}

uint32_t crear_tabla_1er_nivel(){
	//creamos la tabla
	fila_1er_nivel tabla [entradas_por_tabla];
	for (int i = 0; i < entradas_por_tabla; i++){
		tabla[i] = malloc(sizeof(fila_1er_nivel));
	}
	return list_add(lista_tablas_1er_nivel, tabla); //devuelve el index en el que quedó el elemento agregado
}

void crear_tablas_2do_nivel(){
//	fila_2do_nivel tabla [entradas_por_tabla] = malloc(sizeof(fila_2do_nivel)*entradas_por_tabla);
//	return list_add(lista_tablas_2do_nivel, tabla);
//	//No va a ser tan así, por ejemplo debería considerar el tamaño del proceso para no crear tablas de más.
//	for(int i=0; i < entradas_por_tabla; i++){
//		t_list* tabla_2do_nivel = list_create();
//		list_add(lista_tablas_2do_nivel, tabla_2do_nivel);
//		//asignar_marcos(tabla_2do_nivel);
//		numero_tabla_2do_nivel = list_size(lista_tablas_2do_nivel) - 1;
//	}
}

void asignar_marcos(t_list* tabla_2do_nivel){

}
