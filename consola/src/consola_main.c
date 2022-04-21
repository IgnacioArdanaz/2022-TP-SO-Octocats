#include "consola_main.h"

int main(int argc, char** argv){
	if(argc < 3) {
		return EXIT_FAILURE;
	}
	//Recibe primero el tamanio y despues el path
	int tamanio = atoi(argv[1]);
	char* path = argv[2];

	char* ip;
	char* puerto;
	int conexion;

	conexion = crear_conexion(ip, puerto);
}
