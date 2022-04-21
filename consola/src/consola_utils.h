#ifndef CONSOLA_UTILS_H_
#define CONSOLA_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>

int crear_conexion(char* ip, char* puerto);

#endif /* CONSOLA_UTILS_H_ */
