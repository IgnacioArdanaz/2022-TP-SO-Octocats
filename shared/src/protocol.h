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
	CREAR_TABLA,
	ELIMINAR_ESTRUCTURAS,
	SUSPENDER_PROCESO,
	SOLICITUD_NRO_TABLA_2DO_NIVEL,
	SOLICITUD_NRO_MARCO,
	READ,
	WRITE,
	ESCRITURA_OK,
	DATOS_NECESARIOS,
} op_code;

// PROGRAMA
bool send_programa(int fd, t_list* instrucciones, uint16_t tamanio);
bool recv_programa(int fd, t_list* instrucciones, uint16_t* tamanio);
static void* serializar_programa(size_t* size, t_list* instrucciones, uint16_t tamanio);
static void deserializar_programa(void* stream, t_list* instrucciones, uint16_t* tamanio);

// PROCESO
bool send_proceso(int fd, PCB_t* proceso, op_code codigo);
static void* serializar_proceso(size_t* size, PCB_t* proceso, op_code codigo);
bool recv_proceso(int fd, PCB_t* proceso);
static void deserializar_proceso(void* stream, PCB_t* proceso);

// DATOS NECESARIOS
bool send_datos_necesarios(int fd, uint16_t entradas_por_tabla, uint16_t tam_pagina);
static void* serializar_datos_necesarios(uint16_t entradas_por_tabla, uint16_t tam_pagina);
bool recv_datos_necesarios(int fd, uint16_t* entradas_por_tabla, uint16_t* tam_pagina);

// CREAR TABLA
bool send_crear_tabla(int fd, uint16_t tamanio, uint16_t pid);
static void* serializar_crear_tabla(uint16_t tamanio, uint16_t pid);
bool recv_crear_tabla(int fd, uint16_t* tamanio, uint16_t* pid);

// SUSENDER PROCESO
bool send_suspender_proceso(int fd, uint16_t pid, uint32_t tabla_paginas);
static void* serializar_suspender_proceso(uint16_t pid, uint32_t tabla_paginas);
bool recv_suspender_proceso(int fd, uint16_t* pid, uint32_t* tabla_paginas);

// ELIMINAR ESTRUCTURAS
bool send_eliminar_estructuras(int fd, uint32_t tabla_paginas, uint16_t pid);
static void* serializar_eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid);
bool recv_eliminar_estructuras(int fd, uint16_t* pid, uint32_t* tabla_paginas);

// SOLICITUD TABLA 2DO NIVEL
bool send_solicitud_nro_tabla_2do_nivel(int fd, uint16_t pid, uint32_t nro_tabla_1er_nivel, uint32_t entrada_tabla);
static void* serializar_solicitud_nro_tabla_2do_nivel(uint16_t pid, uint32_t nro_tabla_1er_nivel, uint32_t entrada_tabla);
bool recv_solicitud_nro_tabla_2do_nivel(int fd, uint16_t* pid, uint32_t* nro_tabla_1er_nivel, uint32_t* entrada_tabla);


// SOLICITUD NRO MARCO
bool send_solicitud_nro_marco(int fd, uint16_t pid, uint32_t nro_tabla_2do_nivel, uint32_t entrada_tabla, uint32_t index_tabla_1er_nivel);
static void* serializar_solicitud_nro_marco(uint16_t pid, uint32_t nro_tabla_2do_nivel, uint32_t entrada_tabla, uint32_t index_tabla_1er_nivel);
bool recv_solicitud_nro_marco(int fd, uint16_t* pid, uint32_t* nro_tabla_2do_nivel, uint32_t* entrada_tabla, uint32_t* index_tabla_1er_nivel);

// READ
bool send_read(int fd, uint32_t nro_marco, uint16_t desplazamiento);
static void* serializar_read(uint32_t nro_marco, uint16_t desplazamiento);
bool recv_read(int fd, uint32_t* nro_marco, uint16_t* desplazamiento);

// WRITE
bool send_write(int fd, uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato);
static void* serializar_write(uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato);
bool recv_write(int fd, uint32_t* nro_marco, uint16_t* desplazamiento, uint32_t* dato);

#endif
