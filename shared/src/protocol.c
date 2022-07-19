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
	int i = 0;
    for(i = 0; i < length_lista; i++){
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

/************************* DATOS NECESARIOS (MEMORIA -> CPU) ***********************/
// Envio y serializacion
bool send_datos_necesarios(int fd, uint16_t entradas_por_tabla, uint16_t tam_pagina) {
    size_t size = 2 * sizeof(uint16_t);
    void* stream = serializar_datos_necesarios(entradas_por_tabla, tam_pagina);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_datos_necesarios(uint16_t entradas_por_tabla, uint16_t tam_pagina) {
    void* stream = malloc(2 * sizeof(uint16_t));

    size_t acumulador = 0;
    memcpy(stream + acumulador, &entradas_por_tabla, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &tam_pagina, sizeof(uint16_t));

    return stream;
}

bool recv_datos_necesarios(int fd, uint16_t* entradas_por_tabla, uint16_t* tam_pagina) {
	size_t size = 2 * sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	 }

	size_t acumulador = 0;
	memcpy(entradas_por_tabla, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(tam_pagina, stream + acumulador, sizeof(uint16_t));

	printf("tam_pagina=%d - cant_ent_paginas=%d", *tam_pagina, *entradas_por_tabla);
	free(stream);
    return true;
}


/***************************** SOLICITUD TABLA PAGINAS *****************************/
// Envio y serializacion
bool send_crear_tabla(int fd, uint16_t tamanio, uint16_t pid) {
    size_t size = sizeof(op_code) + 2 * sizeof(uint16_t);
    void* stream = serializar_crear_tabla(tamanio, pid);
    printf("Mandando %d \n", tamanio);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_crear_tabla(uint16_t tamanio, uint16_t pid) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint16_t) + sizeof(uint16_t));

    op_code cop = CREAR_TABLA;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &tamanio, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &pid, sizeof(uint16_t));

    return stream;
}

bool recv_crear_tabla(int fd, uint16_t* tamanio, uint16_t* pid) {
    size_t size = 2 * sizeof(uint16_t);
    void* stream = malloc(size);
    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
     }

    size_t acumulador = 0;
    memcpy(tamanio, stream + acumulador, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(pid, stream + acumulador, sizeof(uint16_t));

    free(stream);
    return true;
}

/***************************** ELIMINAR ESTRUCTURAS *****************************/
bool send_eliminar_estructuras(int fd, uint32_t tabla_paginas, uint16_t pid) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = serializar_eliminar_estructuras(tabla_paginas, pid);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t));

    op_code cop = ELIMINAR_ESTRUCTURAS;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &tabla_paginas, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
	memcpy(stream + acumulador, &pid, sizeof(uint16_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_eliminar_estructuras(int fd, uint16_t* pid, uint32_t* tabla_paginas) {
	size_t size = sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	 }

	size_t acumulador = 0;
	memcpy(&tabla_paginas, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&pid, stream + acumulador, sizeof(uint16_t));

	free(stream);
    return true;
}


/***************************** SUSENDER PROCESO *****************************/
bool send_suspender_proceso(int fd, uint16_t pid, uint32_t tabla_paginas) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = serializar_suspender_proceso(tabla_paginas, pid);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_suspender_proceso(uint16_t pid, uint32_t tabla_paginas) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t));

    op_code cop = SUSPENDER_PROCESO;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
	memcpy(stream + acumulador, &pid, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &tabla_paginas, sizeof(uint32_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_suspender_proceso(int fd, uint16_t* pid, uint32_t* tabla_paginas) {
	size_t size = sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	 }

	size_t acumulador = 0;
	memcpy(&pid, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(&tabla_paginas, stream + acumulador, sizeof(uint32_t));

	free(stream);
    return true;
}

/***************************** SOLICITUD TABLA 2DO NIVEL *****************************/
bool send_solicitud_nro_tabla_2do_nivel(int fd, uint16_t pid, uint32_t nro_tabla_1er_nivel, uint32_t entrada_tabla) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) * 2 + sizeof(uint16_t);
	void* stream = serializar_solicitud_nro_tabla_2do_nivel(pid, nro_tabla_1er_nivel, entrada_tabla);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_solicitud_nro_tabla_2do_nivel(uint16_t pid, uint32_t nro_tabla_1er_nivel, uint32_t entrada_tabla) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) * 2+ sizeof(uint16_t));

    op_code cop = SOLICITUD_NRO_TABLA_2DO_NIVEL;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
	memcpy(stream + acumulador, &pid, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &nro_tabla_1er_nivel, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &entrada_tabla, sizeof(uint32_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_solicitud_nro_tabla_2do_nivel(int fd, uint16_t* pid, uint32_t* nro_tabla_1er_nivel, uint32_t* entrada_tabla) {
	size_t size = sizeof(uint32_t) * 2 + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	 }

	size_t acumulador = 0;
	memcpy(&pid, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(&nro_tabla_1er_nivel, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&entrada_tabla, stream + acumulador, sizeof(uint32_t));

	free(stream);
    return true;
}

/***************************** SOLICITUD NRO MARCO *****************************/
bool send_solicitud_nro_marco(int fd, uint16_t pid, uint32_t nro_tabla_2do_nivel, uint32_t entrada_tabla, uint32_t index_tabla_1er_nivel) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) * 3 + sizeof(uint16_t);
	void* stream = serializar_solicitud_nro_marco(pid, nro_tabla_2do_nivel, entrada_tabla, index_tabla_1er_nivel);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_solicitud_nro_marco(uint16_t pid, uint32_t nro_tabla_2do_nivel, uint32_t entrada_tabla, uint32_t index_tabla_1er_nivel) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) * 3 + sizeof(uint16_t));

    op_code cop = SOLICITUD_NRO_MARCO;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
	memcpy(stream + acumulador, &pid, sizeof(uint16_t));
    acumulador += sizeof(uint16_t);
    memcpy(stream + acumulador, &nro_tabla_2do_nivel, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &entrada_tabla, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &index_tabla_1er_nivel, sizeof(uint32_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_solicitud_nro_marco(int fd, uint16_t* pid, uint32_t* nro_tabla_2do_nivel, uint32_t* entrada_tabla, uint32_t* index_tabla_1er_nivel) {
	size_t size = sizeof(uint32_t) * 3 + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}

	size_t acumulador = 0;
	memcpy(&pid, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint16_t);
	memcpy(&nro_tabla_2do_nivel, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&entrada_tabla, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&index_tabla_1er_nivel, stream + acumulador, sizeof(uint32_t));

	free(stream);
    return true;
}

/***************************** READ *****************************/
bool send_read(int fd, uint32_t nro_marco, uint16_t desplazamiento) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = serializar_read(nro_marco, desplazamiento);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_read(uint32_t nro_marco, uint16_t desplazamiento) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) + sizeof(uint16_t));

    op_code cop = READ;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &nro_marco, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &desplazamiento, sizeof(uint32_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_read(int fd, uint32_t* nro_marco, uint16_t* desplazamiento) {
	size_t size = sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}

	size_t acumulador = 0;
	memcpy(&nro_marco, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&desplazamiento, stream + acumulador, sizeof(uint16_t));

	free(stream);
    return true;
}

/***************************** WRITE *****************************/
bool send_write(int fd, uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato) {
	size_t size = sizeof(op_code) + sizeof(uint32_t) * 2 + sizeof(uint16_t) ;
	void* stream = serializar_write(nro_marco, desplazamiento, dato);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_write(uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint32_t) * 2 + sizeof(uint16_t));

    op_code cop = WRITE;
    size_t acumulador = 0;
    memcpy(stream + acumulador, &cop, sizeof(op_code));
    acumulador += sizeof(op_code);
    memcpy(stream + acumulador, &nro_marco, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &desplazamiento, sizeof(uint32_t));
    acumulador += sizeof(uint32_t);
    memcpy(stream + acumulador, &dato, sizeof(uint32_t));

    return stream;
}

//Recepcion y deserealizacion
bool recv_write(int fd, uint32_t* nro_marco, uint16_t* desplazamiento, uint32_t* dato) {
	size_t size = sizeof(uint32_t) + sizeof(uint16_t);
	void* stream = malloc(size);
	if (recv(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	 }

	size_t acumulador = 0;
	memcpy(&nro_marco, stream + acumulador, sizeof(uint32_t));
	acumulador += sizeof(uint32_t);
	memcpy(&desplazamiento, stream + acumulador, sizeof(uint16_t));
	acumulador += sizeof(uint32_t);
	memcpy(&dato, stream + acumulador, sizeof(uint16_t));

	free(stream);
    return true;
}


