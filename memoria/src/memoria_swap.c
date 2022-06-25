#include "memoria_swap.h"

char* swap_path;
uint16_t retardo_swap;

void inicializar_swap(){
	swap_path = config_get_string_value(config,"PATH_SWAP");
	retardo_swap = config_get_int_value(config,"RETARDO_SWAP");
}

// crear archivo
FILE* crear_archivo_swap(uint16_t pid){
	char* path_to_file = string_from_format("%s/%d.swap",swap_path, pid);
	printf("Archivo %s creado.\n", path_to_file);
	FILE* f = fopen(path_to_file,"wb");
	return f;
}

// abrir archivo
FILE* abrir_archivo_swap(uint16_t pid){
	char* path_to_file = string_from_format("%s/%d.swap",swap_path, pid);
	FILE* f = fopen(path_to_file,"rb");
	return f;
}

// cerrar archivo
void cerrar_archivo_swap(FILE* archivo){
	fclose(archivo);
}

// borrar archivo
void borrar_archivo_swap(uint16_t pid){
	char* path_to_file = string_from_format("%s/%d.swap",swap_path, pid);
	remove(path_to_file);
	printf("Archivo %s eliminado.\n", path_to_file);
}

// agregar marco en archivo, devuelve el nro de marco que representa en swap
uint32_t agregar_marco_en_swap(FILE* archivo, uint32_t tamanio_marcos){
	fseek(archivo, 0, SEEK_END);
	void* marco = malloc(tamanio_marcos);
	fwrite(marco, 1, tamanio_marcos, archivo);
	free(marco);
	return ftell(archivo) / tamanio_marcos - 1;
}

// actualizar marco en archivo
void actualizar_marco_en_swap(FILE* archivo, uint32_t nro_marco, void* marco, uint32_t tamanio_marcos){
	fseek(archivo, nro_marco * tamanio_marcos, SEEK_SET);
	fwrite(marco, 1, tamanio_marcos, archivo);
	usleep(retardo_swap * 1000);
}

// leer marco
void* leer_marco_en_swap(FILE* archivo, uint32_t nro_marco, uint32_t tamanio_marcos){
	void* marco = malloc(tamanio_marcos);
	fseek(archivo, nro_marco * tamanio_marcos, SEEK_SET);
	fread(marco, 1, tamanio_marcos, archivo);
	usleep(retardo_swap * 1000);
	return marco;
}
