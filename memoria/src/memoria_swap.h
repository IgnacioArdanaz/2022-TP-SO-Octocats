#ifndef MEMORIA_SWAP_H_
#define MEMORIA_SWAP_H_

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>

t_log* logger;
t_config* config;

void inicializar_swap();

// crear archivo
FILE* crear_archivo_swap(uint16_t pid);

// abrir archivo
FILE* abrir_archivo_swap(uint16_t pid);

// cerrar archivo
void cerrar_archivo_swap(FILE* archivo);

// borrar archivo
void borrar_archivo_swap(uint16_t pid);

// agregar marco en archivo
uint32_t agregar_marco_en_swap(FILE* archivo, uint32_t tamanio_marcos);

// actualizar marco en archivo
void actualizar_marco_en_swap(FILE* archivo, uint32_t nro_marco, void* marco, uint32_t tamanio_marcos);

// leer marco
void* leer_marco_en_swap(FILE* archivo, uint32_t nro_marco, uint32_t tamanio_marcos);

#endif
