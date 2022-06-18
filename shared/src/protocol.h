#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <inttypes.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "structures.h"
#include "pcb.h"

typedef enum {
    PROGRAMA,
	PROCESO,
	INTERRUPTION,
	EXIT,
	CONTINUE,
	BLOCKED,
	SOLICITUD_TABLA,
	ELIMINAR_ESTRUCTURAS
} op_code;

//PROGRAMA
bool send_programa(int fd, t_list* instrucciones, uint16_t tamanio);
bool recv_programa(int fd, t_list* instrucciones, uint16_t* tamanio);
static void* serializar_programa(size_t* size, t_list* instrucciones, uint16_t tamanio);
static void deserializar_programa(void* stream, t_list* instrucciones, uint16_t* tamanio);

//PROCESO
bool send_proceso(int fd, PCB_t* proceso, op_code codigo);
static void* serializar_proceso(size_t* size, PCB_t* proceso, op_code codigo);
bool recv_proceso(int fd, PCB_t* proceso);
static void deserializar_proceso(void* stream, PCB_t* proceso);

//SOLICITUD TABLA PAGINAS
bool send_solicitud_tabla_paginas(int fd, uint32_t tamanio);
static void* serializar_solicitud(uint32_t tamanio);

// ELIMINAR ESTRUCTURAS
bool send_eliminar_estructuras(int fd, uint32_t tabla_paginas, uint16_t pid)
static void* serializar_eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid)
bool recv_eliminar_estructuras(int fd, uint16_t* pid, uint32_t* tabla_paginas)

#endif
