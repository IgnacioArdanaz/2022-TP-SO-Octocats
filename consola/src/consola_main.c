#include "consola_main.h"
//#include "parser.c"

//int main(int argc, char** argv){
//	if(argc < 3) {
//		return EXIT_FAILURE;
//	}
//	//Recibe primero el tamanio y despues el path
//	int tamanio = atoi(argv[1]);
//	char* path = argv[2];

	/*-----------------------------ACA VA EL PARSER------------------------------------------*/
/*
 * TODO Archivo parser agregado, falta parser.h
 * LLamar funcion parser(int argc, char** argv) para iniciar
 * Alejiti & Ziroide <3
 */

int main(){
	/*---------------------------------------------------------------------------------------*/

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

	log_destroy(logger);
	config_destroy(config);
	close(conexion);
}
