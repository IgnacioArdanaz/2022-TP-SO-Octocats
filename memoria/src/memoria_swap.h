#ifndef MEMORIA_SWAP_H_
#define MEMORIA_SWAP_H_

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>

t_log* logger;
t_config* config;
uint16_t retardo_swap;

void inicializar_swap();

// crear archivo
int crear_archivo_swap(uint16_t pid, uint32_t tamanio_en_bytes);

// borrar archivo
void borrar_archivo_swap(uint16_t pid, int fd);

// actualizar marco en archivo
void actualizar_marco_en_swap(int fd, uint32_t nro_marco, void* marco, uint32_t tamanio_marcos);

// leer marco
void* leer_marco_en_swap(int fd, uint32_t nro_marco, uint32_t tamanio_marcos);

#endif
