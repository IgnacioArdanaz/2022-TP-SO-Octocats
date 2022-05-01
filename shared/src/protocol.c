#include "protocol.h"

// PROGRAMA
bool send_programa(int fd, char** instrucciones, uint8_t tamanio) {
    size_t size;
    void* stream = serializar_programa(&size, instrucciones, tamanio);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool recv_programa(int fd, char** instrucciones, uint8_t* tamanio) {
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

static void* serializar_programa(size_t* size, char** instrucciones, uint8_t tamanio) {
    size_t size_stream_inst = 0, length_lista = 0;

    // Serializamos todas las instrucciones en un stream auxiliar
    void * stream_inst = serializar_instrucciones(&size_stream_inst, instrucciones, &length_lista);

    printf("Size_lista: %zu \n", size_stream_inst);

    *size =
          sizeof(op_code)   // 4
        + sizeof(size_t)    // size de size_payload
        + sizeof(size_t)    // size de length_lista
		+ sizeof(size_t)    // size de size_lista
        + size_stream_inst  // size del stream de instrucciones (stream_inst)
        + sizeof(uint8_t);  // tamanio de memoria
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);

    op_code cop = PROGRAMA;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream + sizeof(op_code), &size_payload, sizeof(size_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t), &length_lista, sizeof(size_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 2, &size_stream_inst, sizeof(size_t));
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 3, stream_inst, size_stream_inst);
    memcpy(stream + sizeof(op_code) + sizeof(size_t) * 3 + size_stream_inst, &tamanio, sizeof(uint8_t));

    return stream;
}

static void deserializar_programa(void* stream, char** instrucciones, uint8_t* tamanio) {
	size_t length_lista, size_stream_inst, size_instruccion, acumulador = 0;
	// Primero deserializo la longitud de la lista (cantidad de elementos) y el size del stream de las instrucciones
    memcpy(&length_lista, stream, sizeof(size_t));
    memcpy(&size_stream_inst, stream + sizeof(size_t), sizeof(size_t));

    char* r_instruccion;
    acumulador = sizeof(size_t) * 2;
    printf("1 \n");
    for(int i = 0; i < length_lista; i++){
    	// Despues deserializo el size de cada instruccion
    	memcpy(&size_instruccion, stream + acumulador, sizeof(size_t));
    	acumulador += sizeof(size_t);
    	r_instruccion = malloc(size_instruccion);
    	// Despues deserializo cada instruccion en si y la guardo en un array de strings
    	memcpy(r_instruccion, stream + acumulador, size_instruccion);
    	acumulador += size_instruccion;
    	printf("Instruccion recibida: %s \n", r_instruccion);
    	string_array_push(instrucciones, r_instruccion);
    }
    uint8_t r_tamanio;
    // Finalmente deserializo el tamanio del programa para memoria
    memcpy(&r_tamanio, stream + acumulador, sizeof(uint8_t));
    *tamanio = r_tamanio;
}

static void* serializar_instrucciones(size_t* size_stream_inst, char** instrucciones, size_t* length_instrucciones) {
	void * stream = malloc(sizeof(void*));
	size_t size_instruccion;

	for(uint8_t i = 0; instrucciones[i] != NULL; i++){
		// Primero serializo la longitud de cada instruccion
		size_instruccion = strlen(instrucciones[i]) + 1;
		memcpy(stream + *size_stream_inst, &size_instruccion, sizeof(size_t));
		*size_stream_inst += sizeof(size_t);
		// Despues serializo la instruccion en si
		memcpy(stream + *size_stream_inst, instrucciones[i], size_instruccion);
		*size_stream_inst += size_instruccion;
		*length_instrucciones += 1;
	}

    return stream;
}
