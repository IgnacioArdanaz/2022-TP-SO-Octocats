#include "protocol.h"

/***************************** PROGRAMA *****************************/
// Envio y serializacion
bool send_programa(int fd, t_list* instrucciones, uint16_t tamanio) {
    size_t size;
    void* stream = serializar_programa(&size, instrucciones, tamanio);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_programa(size_t* size, t_list* instrucciones, uint16_t tamanio) {
    size_t inst_size = sizeof(char) + sizeof(int32_t)*2;
    size_t length_lista = list_size(instrucciones);
    size_t codigo_size = inst_size*length_lista;
    *size =
          sizeof(op_code)	// 4
        + sizeof(size_t)	// size de size_payload
        + sizeof(size_t)	// size de length_lista
        + codigo_size  		// size de la lista de  instrucciones
        + sizeof(uint16_t);	// tamanio de memoria
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);
    op_code cop = PROGRAMA;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &size_payload, sizeof(size_t));
    acumulador += sizeof(size_t);
    memcpy(stream + acumulador, &length_lista, sizeof(size_t));
    acumulador += sizeof(size_t);
    for (int j = 0; j < length_lista; j++){
    	instruccion_t* inst = list_get(instrucciones,j);
        memcpy(stream + acumulador, &inst->operacion, sizeof(char));
        acumulador += sizeof(char);
        memcpy(stream + acumulador, &inst->arg1, sizeof(int32_t));
        acumulador += sizeof(int32_t);
        memcpy(stream + acumulador, &inst->arg2, sizeof(int32_t));
        acumulador += sizeof(int32_t);
    }
    memcpy(stream + acumulador, &tamanio, sizeof(uint16_t));

    return stream;
}

// Recepcion y deserializacion
bool recv_programa(int fd, t_list* instrucciones, uint16_t* tamanio) {
	size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), 0) != sizeof(size_t))
        return false;
    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, 0) != size_payload) {
        free(stream);
        return false;
    }
    deserializar_programa(stream, instrucciones, tamanio);
    free(stream);
    return true;
}

static void deserializar_programa(void* stream, t_list* instrucciones, uint16_t* tamanio) {
	size_t length_lista, acumulador = 0;
	// Primero deserializo la longitud de la lista (cantidad de elementos)
    memcpy(&length_lista, stream, sizeof(size_t));

	acumulador += sizeof(size_t);
    for(int i = 0; i < length_lista; i++){
    	instruccion_t* instruccion = malloc(sizeof(instruccion_t));
    	// Despues deserializo el size de cada instruccion
    	memcpy(&instruccion->operacion, stream + acumulador, sizeof(char));
    	acumulador += sizeof(char);
    	memcpy(&instruccion->arg1, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	memcpy(&instruccion->arg2, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	list_add(instrucciones,instruccion);
    }
    uint16_t r_tamanio;
    // Finalmente deserializo el tamanio del programa para memoria
    memcpy(&r_tamanio, stream + acumulador, sizeof(uint16_t));
    *tamanio = r_tamanio;
}

/***************************** PROCESO *****************************/
// Envio y serializacion PROGRAMA
bool send_proceso(int fd, PCB_t *proceso, op_code codigo) {
    size_t size;
    void* stream = serializar_proceso(&size, proceso, codigo);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_proceso(size_t* size, PCB_t *proceso, op_code codigo) {
    size_t inst_size = sizeof(char) + sizeof(int32_t)*2;
    size_t length_lista = list_size(proceso->instrucciones);
    size_t codigo_size = inst_size*length_lista;
    *size =
          sizeof(op_code)   // 4
        + sizeof(size_t)    // size de size_payload
		+ sizeof(uint16_t)  // size de pid
		+ sizeof(uint16_t)  // size de tamanio
        + sizeof(size_t)    // size de length_lista
        + codigo_size  // size del stream de instrucciones (stream_inst)
		+ sizeof(uint32_t)  // size de pc
		+ sizeof(uint32_t)  // size de tabla_paginas
        + sizeof(double);   // size de est_rafaga
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);
	size_t acumulador = 0;
    op_code cop = codigo;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &size_payload, sizeof(size_t));
    acumulador += sizeof(size_t);
    memcpy(stream + acumulador, &proceso->pid, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &proceso->tamanio, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &length_lista, sizeof(size_t));
    acumulador += sizeof(size_t);

    for(int i=0; i<length_lista; i++){
    	instruccion_t* inst = list_get(proceso->instrucciones,i);
        memcpy(stream + acumulador, &inst->operacion, sizeof(char));
        acumulador += sizeof(char);
        memcpy(stream + acumulador, &inst->arg1, sizeof(int32_t));
        acumulador += sizeof(int32_t);
        memcpy(stream + acumulador, &inst->arg2, sizeof(int32_t));
        acumulador += sizeof(int32_t);
    }

    memcpy(stream + acumulador, &proceso->pc, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &proceso->tabla_paginas, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &proceso->est_rafaga, sizeof(double));
    return stream;
}

//Recepcion y deserealizacion
bool recv_proceso(int fd, PCB_t* proceso) {
    size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), 0) != sizeof(size_t))
        return false;
    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, 0) != size_payload) {
        free(stream);
        return false;
    }
    deserializar_proceso(stream, proceso);
    free(stream);
    return true;
}

static void deserializar_proceso(void* stream, PCB_t* proceso) {
	size_t length_lista, acumulador = 0;

	memcpy(&proceso->pid, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(&proceso->tamanio, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(&length_lista, stream + acumulador, sizeof(size_t));
	acumulador += sizeof(size_t);

    for(int i = 0; i < length_lista; i++){
    	instruccion_t* instruccion = malloc(sizeof(instruccion_t));
    	// Despues deserializo el size de cada instruccion
    	memcpy(&instruccion->operacion, stream + acumulador, sizeof(char));
    	acumulador += sizeof(char);
    	memcpy(&instruccion->arg1, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	memcpy(&instruccion->arg2, stream + acumulador, sizeof(int32_t));
    	acumulador += sizeof(int32_t);
    	list_add(proceso->instrucciones,instruccion);
    }

    // Finalmente deserializo el tamanio del programa para memoria
    memcpy(&proceso->pc, stream + acumulador, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(&proceso->tabla_paginas, stream + acumulador, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(&proceso->est_rafaga, stream + acumulador, sizeof(double));

}

/***************************** SOLICITUD TABLA PAGINAS *****************************/
// Envio y serializacion
bool send_solicitud_tabla_paginas(int fd, uint32_t tamanio) {
    size_t size = sizeof(op_code) + sizeof(uint32_t);
    void* stream = serializar_solicitud(tamanio);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_solicitud(uint32_t tamanio) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t));

    op_code cop = SOLICITUD_TABLA;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &tamanio, sizeof(uint32_t));

    return stream;
}
