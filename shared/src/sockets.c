#include "sockets.h"

int crear_conexion(t_log* logger, const char* server_name, char* ip, char* puerto) {
    struct addrinfo hints, *serv_info;

    // Init de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe addrinfo
    getaddrinfo(ip, puerto, &hints, &serv_info);

    // Crea un socket con la informacion recibida (del primero, suficiente)
    int socket_cliente = socket(serv_info->ai_family,
    		serv_info->ai_socktype,
			serv_info->ai_protocol);

    // Fallo en crear el socket
    if(socket_cliente == -1) {
        //log_info(logger, "Error creando el socket para %s:%s\n", ip, puerto);
    	printf("Error creando el socket para %s:%s \n", ip, puerto);
        return 0;
    }

    // Error conectando
    if(connect(socket_cliente, serv_info->ai_addr, serv_info->ai_addrlen) == -1) {
    	//log_info(logger, "Error al conectar (a %s)\n", server_name);
    	printf("Error al conectar (a %s)\n", server_name);
    	freeaddrinfo(serv_info);
        return 0;
    }

    //log_info(logger, "Cliente conectado en %s:%s (a %s)\n", ip, puerto, server_name);
    printf("Cliente conectado en %s:%s (a %s)\n", ip, puerto, server_name);

    freeaddrinfo(serv_info);

    return socket_cliente;
}

int iniciar_servidor(t_log* logger, const char* name, char* ip, char* puerto) {
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    // Inicializando hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe los addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    bool conecto = false;

    // Itera por cada addrinfo devuelto
    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_servidor == -1) // fallo de crear socket
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            // Si entra aca fallo el bind
            close(socket_servidor);
            continue;
        }
        // Ni bien conecta uno nos vamos del for
        conecto = true;
        break;
    }

    if(!conecto) {
        free(servinfo);
        return 0;
    }

    listen(socket_servidor, SOMAXCONN); // Escuchando (hasta SOMAXCONN conexiones simultaneas)

    // Aviso al logger
    //log_info(logger, "Escuchando en %s:%s (%s)\n", ip, puerto, name);
    printf("Escuchando en %s:%s (%s)\n", ip, puerto, name);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

// ESPERAR CONEXION DE CLIENTE EN UN SERVER ABIERTO
int esperar_cliente(t_log* logger, const char* name, int socket_servidor) {
    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    //log_info(logger, "%s listo para recibir al cliente\n", name);
    printf("%s listo para recibir al cliente \n", name);

    int* socket_cliente = malloc(sizeof(int));
    *socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

    //log_info(logger, "Cliente conectado (a %s)\n", name);
    printf("Cliente conectado (a %s)\n", name);
    return *socket_cliente;
}

// CERRAR CONEXION
void liberar_conexion(int* socket_cliente) {
    close(*socket_cliente);
    *socket_cliente = -1;
}
