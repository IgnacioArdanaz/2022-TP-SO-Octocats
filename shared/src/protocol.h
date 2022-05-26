#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <inttypes.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "structures.h"

typedef enum {
    PROGRAMA,
	PROCESO,
} op_code;

//PROGRAMA
bool send_programa(int fd, char** instrucciones, uint16_t tamanio);
bool recv_programa(int fd, char** instrucciones, uint16_t* tamanio);
static void* serializar_programa(size_t* size, char** instrucciones, uint16_t tamanio);
static void deserializar_programa(void* stream, char** instrucciones, uint16_t* tamanio);
static void* serializar_instrucciones(size_t* size_lista, char** instrucciones, size_t* length_instrucciones);

//PROCESO
bool send_proceso(int fd, PCB_t* proceso);
static void* serializar_proceso(size_t* size, PCB_t* proceso);
bool recv_proceso(int fd, PCB_t* proceso);
static void deserializar_proceso(void* stream, PCB_t* proceso);


#endif
