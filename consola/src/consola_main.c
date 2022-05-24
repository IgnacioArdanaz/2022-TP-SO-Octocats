#include "consola_main.h"

int main(int argc, char** argv){
	if(argc < 3) {
		return EXIT_FAILURE;
	}

	//Recibe primero el tamanio y despues el path
	uint16_t tamanio = atoi(argv[1]);
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
	uint32_t resultado;

	t_log* logger = log_create("consola.log", "consola", 1, LOG_LEVEL_INFO);;
	t_config* config = config_create("consola.config");

	ip_kernel = config_get_string_value(config, "IP_KERNEL");
	puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

	log_info(logger, "IP: %s | Puerto: %s", ip_kernel, puerto_kernel);

	conexion = crear_conexion(logger, "KERNEL", ip_kernel, puerto_kernel);

	if (send_programa(conexion, tabla, tamanio)){
		log_info(logger,"Programa enviado al kernel correctamente!");
		recv(conexion, &resultado, sizeof(uint32_t), MSG_WAITALL); // Cada consola espera respuesta del kernel, puede recibir 0 (resultOK) o -1 (resultError)
		log_info(logger,"resultado -> %d\n", resultado);
	}
	else{
		log_error(logger,"Por desgracia no se pudo enviar el programa :(");
	}


	string_array_destroy(tabla);
//	log_destroy(logger);   //ESTO DA EL SGMENTATION FAULT POR ALGUNA RAZON ...?
//	config_destroy(config);
	liberar_conexion(&conexion);

	return 0;

}
