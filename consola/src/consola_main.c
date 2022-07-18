#include "consola_main.h"

int main(int argc, char** argv){
	t_log* logger = log_create("consola.log", "CONSOLA", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("consola.config");
	if(argc < 3) {
		log_error(logger,"Uso incorrecto :(\n\tEjemplo: ./a.out un_tamanio ruta_de_pseudocodigo");
		return EXIT_FAILURE;
	}

	//Recibe primero el tamanio y despues el path
	uint32_t tamanio = atoi(argv[1]);
	char* path = string_duplicate(argv[2]);

	t_list* codigo = list_create();
	parser(path, codigo);
	free(path);
	int cant = list_size(codigo);
	for(int i=0; i<cant; i++){
		instruccion_t* inst = list_get(codigo,i);
		log_info(logger, "Instruccion numero %d: %c %d %d",i,inst->operacion, inst->arg1, inst->arg2);
	}

	char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
	char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
	int conexion = crear_conexion(logger, "KERNEL", ip_kernel, puerto_kernel);
	op_code resultado = EXIT;

	if (send_programa(conexion, codigo, tamanio)){
		log_info(logger,"Programa enviado al kernel correctamente!");
		recv(conexion, &resultado, sizeof(op_code), MSG_WAITALL); // Cada consola espera respuesta del kernel, puede recibir 0 (resultOK) o -1 (resultError)
		if (resultado == EXIT){
			log_info(logger,"El proceso se finalizó exitosamente :)");
		} else{
			log_error(logger,"El proceso NO se finalizó exitosamente (co op %d):(", resultado);
		}
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
