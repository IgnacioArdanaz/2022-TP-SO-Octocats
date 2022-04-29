#include "consola_main.h"

int main(int argc, char** argv){
	if(argc < 3) {
		return EXIT_FAILURE;
	}
	//Recibe primero el tamanio y despues el path
	int tamanio = atoi(argv[1]);
	char* path = argv[2];

	char** tabla = string_array_new();
	parser(path, &tabla);

	int cant = string_array_size(tabla);
	for(int i=0; i<cant; i++){
		printf("Instruccion numero %d: %s \n",i,tabla[i]);
	}



	int conexion;
	char* ip_kernel;
	char* puerto_kernel;

	t_log* logger;
	t_config* config;

	logger = log_create("consola.log", "consola", 1, LOG_LEVEL_INFO);

	config = config_create("consola.config");

	ip_kernel = config_get_string_value(config, "IP_KERNEL");
	puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

	log_info(logger, "IP: %s | Puerto: %s", ip_kernel, puerto_kernel);

	conexion = crear_conexion(logger, "KERNEL", ip_kernel, puerto_kernel);

	free(tabla);
	log_destroy(logger);
	config_destroy(config);
	close(conexion);

}
