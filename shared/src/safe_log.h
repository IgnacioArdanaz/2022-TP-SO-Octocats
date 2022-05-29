#ifndef SAFE_LOG_H_
#define SAFE_LOG_H_

#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>

typedef struct{
	t_log* logger;
	pthread_mutex_t* mx;
}safe_log;

void safe_log_info(safe_log* logger, const char* message, ...);
void safe_log_error(safe_log* logger, const char* message, ...);

safe_log* safe_log_create(t_log* logger);
void safe_log_destroy(safe_log* safe_logger, bool destroy_logger);

#endif
