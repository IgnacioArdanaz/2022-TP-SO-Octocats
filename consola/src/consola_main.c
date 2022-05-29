#include "consola_main.h"

int main(int argc, char** argv){
	if(argc < 3) {
		printf("Uso incorrecto :(\n\tEjemplo: ./a.out un_tamanio ruta_de_pseudocodigo\n");
		return EXIT_FAILURE;
	}

	//Recibe primero el tamanio y despues el path
	uint16_t tamanio = atoi(argv[1]);
	char* path = string_duplicate(argv[2]);

	t_list* codigo = list_create();
	parser(path, codigo);
	printf("PASE EL PARSER!!!!!!\n");
	int cant = list_size(codigo);
	for(int i=0; i<cant; i++){
		instruccion_t* inst = list_get(codigo,i);
		printf("Instruccion numero %d: %c %d %d\n",i,inst->operacion, inst->arg1, inst->arg2);
	}
	int conexion;
	char* ip_kernel;
	char* puerto_kernel;
	int32_t resultado = 5;

	t_log* logger = log_create("consola.log", "CONSOLA", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("consola.config");

	ip_kernel = config_get_string_value(config, "IP_KERNEL");
	puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

	log_info(logger, "IP: %s | Puerto: %s", ip_kernel, puerto_kernel);

	conexion = crear_conexion(logger, "KERNEL", ip_kernel, puerto_kernel);

	printf("hola conexion %d \n", conexion);

	if (send_programa(conexion, codigo, tamanio)){
		log_info(logger,"Programa enviado al kernel correctamente!");
		recv(conexion, &resultado, sizeof(int32_t), MSG_WAITALL); // Cada consola espera respuesta del kernel, puede recibir 0 (resultOK) o -1 (resultError)
		log_info(logger,"resultado -> %d\n", resultado);
	}
	else{
		log_error(logger,"Por desgracia no se pudo enviar el programa :(");
	}

	list_destroy_and_destroy_elements(codigo,free);
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(&conexion);

	return 0;

}
