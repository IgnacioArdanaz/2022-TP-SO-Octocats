#include "memoria_utils.h"

t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;
uint16_t retardo_swap;
void* memoria;

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");
	char* ip = config_get_string_value(config,"IP");
	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	server_memoria = iniciar_servidor(logger, "MEMORIA", ip, puerto_escucha);

	tam_memoria = config_get_double_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_double_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_double_value(config,"ENTRADAS_POR_TABLA");
	retardo_memoria = config_get_double_value(config,"RETARDO_MEMORIA");
	marcos_por_proceso = config_get_double_value(config,"MARCOS_POR_PROCESO");
	retardo_swap = config_get_double_value(config,"RETARDO_SWAP");

	memoria = malloc(tam_memoria);
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

void crear_tablas_2do_nivel(uint32_t nro_tabla_1er_nivel){



}

// dada un nro de tabla y un index devuelve la entrada correspondiente
// quizas convenga devolver directamente el valor y no su puntero
fila_1er_nivel obtener_entrada_1er_nivel(uint32_t nro_tabla, uint32_t index){
	fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel,nro_tabla);
	return tabla[index];
}

// dada un nro de tabla y un index devuelve la entrada correspondiente
// quizas convenga devolver directamente el valor y no su puntero
fila_2do_nivel obtener_entrada_2do_nivel(uint32_t nro_tabla, uint32_t index){
	fila_2do_nivel* tabla = list_get(lista_tablas_2do_nivel,nro_tabla);
	return tabla[index];
}

// dado un nro de marco y un desplazamiento, devuelve el dato en concreto
// la posicion que nos da el puntero a la memoria + tam_pagina * nro_marco nos da -->
// --> la posicion de inicio del marco especificado, luego eso + desplazamiento -->
// --> nos da la direccion que buscamos
// (chequear implementacion)
// quizas convenga devolver directamente el valor y no su puntero
uint32_t obtener_entrada_marco(uint32_t nro_marco, uint32_t desplazamiento){
	return (uint32_t) memoria + tam_pagina * nro_marco + desplazamiento;
}

void asignar_marcos(t_list* tabla_2do_nivel){

}
