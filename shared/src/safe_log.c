#include "safe_log.h"


//IMPLEMENTAR PARA QUE RECIBAN LOS MISMOS ARGUMENTOS QUE LOG_INFO/ERROR
//void safe_log_info(safe_log* safe_logger,char* message, ...){
//	pthread_mutex_lock(safe_logger->mx);
//	log_info(safe_logger->logger,message);
//	pthread_mutex_unlock(safe_logger->mx);
//}
//
//void safe_log_error(safe_log* safe_logger,const char* message, ...){
//	pthread_mutex_lock(safe_logger->mx);
//	log_error(safe_logger->logger,message);
//	pthread_mutex_unlock(safe_logger->mx);
//}

safe_log* safe_log_create(t_log* logger){
	pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	safe_log* safe_logger = malloc(sizeof(safe_logger));
	safe_logger->logger = logger;
	safe_logger->mx = &mx;
	return safe_logger;
}

void safe_log_destroy(safe_log* safe_logger,bool destroy_logger){
	pthread_mutex_destroy(safe_logger->mx);
	if (destroy_logger){
		log_destroy(safe_logger->logger);
	}
	free(safe_logger);
}

