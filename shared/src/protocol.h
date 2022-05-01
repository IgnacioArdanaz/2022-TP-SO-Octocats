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

typedef enum {
    PROGRAMA,
} op_code;

bool send_programa(int fd, char** instrucciones, uint8_t tamanio);
bool recv_programa(int fd, char** instrucciones, uint8_t* tamanio);
static void* serializar_programa(size_t* size, char** instrucciones, uint8_t tamanio);
static void deserializar_programa(void* stream, char** instrucciones, uint8_t* tamanio);
static void* serializar_instrucciones(size_t* size_lista, char** instrucciones, size_t* length_instrucciones);

#endif
