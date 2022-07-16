#include "memoria_swap.h"

char* swap_path = "/home/utnso/swap";

void inicializar_swap(){
	swap_path = config_get_string_value(config,"PATH_SWAP");
	retardo_swap = config_get_int_value(config,"RETARDO_SWAP");
}

// crear archivo --> devuelve el file descriptor
int crear_archivo_swap(uint16_t pid, uint32_t tamanio_en_bytes){
	char* path_to_file = string_from_format("%s/%d.swap",swap_path, pid);
	
	if (access(path_to_file, F_OK) == 0) // si por alguna razon ya existe el archivo, borralo
		remove(path_to_file);
	int fd = open(path_to_file, O_RDWR|O_TRUNC|O_CREAT|O_EXCL,S_IROTH|S_IWOTH);
	if (fd == -1){
		printf("[ERROR] No se pudo crear el archivo! :(\n");
		printf("[ERROR] %s", strerror(errno));
		exit(-1);
	}
	ftruncate(fd, 0);
	ftruncate(fd, tamanio_en_bytes);
	log_info(logger,"Archivo %s creado.", path_to_file);
	free(path_to_file);
	return fd;
}

// borrar archivo
void borrar_archivo_swap(uint16_t pid, int fd){
	char* path_to_file = string_from_format("%s/%d.swap",swap_path, pid);
	close(fd);
	remove(path_to_file);
	log_info(logger,"Archivo %s eliminado.", path_to_file);
	free(path_to_file);
}

// actualizar marco en archivo
void actualizar_marco_en_swap(int fd, uint32_t nro_marco, void* marco, uint32_t tamanio_marcos){
	struct stat sb;
	if (fstat(fd,&sb) == -1){
		log_error(logger,"No se pudo obtener el tamaño del archivo :(");
		exit(-1);
	}
	void* datos_archivo = mmap(NULL, sb.st_size, PROT_WRITE, MAP_SHARED,fd,0);
	memcpy(datos_archivo + tamanio_marcos * nro_marco, marco, tamanio_marcos);
	munmap(datos_archivo, sb.st_size);
	free(marco);
}

// leer marco
void* leer_marco_en_swap(int fd, uint32_t nro_marco, uint32_t tamanio_marcos){
	void* marco = malloc(tamanio_marcos);
	struct stat sb;
	if (fstat(fd,&sb) == -1){
		log_error(logger, "No se pudo obtener el tamaño del archivo :(");
		exit(-1);
	}
	void* datos_archivo = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE,fd,0);
	if (datos_archivo == MAP_FAILED){ // si fallo el mmap
		log_error(logger, "Fallo de mmap\n[ERROR] %s",strerror(errno));
		if (errno == EINVAL) // si el error es de argumentos invalidos
			log_error(logger, "Parametros pasados: tam %d fd %d offset %d",(int) sb.st_size, fd, nro_marco * tamanio_marcos);
		exit(-1);
	}
	memcpy(marco, datos_archivo + nro_marco * tamanio_marcos, tamanio_marcos);
	munmap(datos_archivo, sb.st_size);
	usleep(retardo_swap * 1000);
	return marco;
}
