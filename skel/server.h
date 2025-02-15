/*
 * Copyright (c) 2024, <>
 */

#ifndef SERVER_H
#define SERVER_H

#include "utils.h"
#include "constants.h"
#include "lru_cache.h"

#define TASK_QUEUE_SIZE         1000
#define MAX_LOG_LENGTH          1000
#define MAX_RESPONSE_LENGTH     4096

typedef struct queue_t queue_t;
struct queue_t
{
	/* Dimensiunea maxima a cozii */
	unsigned int max_size;
	/* Dimensiunea cozii */
	unsigned int size;
	/* Dimensiunea in octeti a tipului de date stocat in coada */
	unsigned int data_size;
	/* Indexul de la care se vor efectua operatiile de front si dequeue */
	unsigned int read_idx;
	/* Indexul de la care se vor efectua operatiile de enqueue */
	unsigned int write_idx;
	/* Bufferul ce stocheaza elementele cozii */
	void **buff;
};

typedef struct server {
    lru_cache *cache;
    queue_t *task_queue;
    doubly_linked_list_t *memoryroom;
    int server_id;
    /* TODO: add needed fields */
} server;

typedef struct request {
    request_type type;
    char *doc_name;
    char *doc_content;
} request;

typedef struct response {
    char *server_log;
    char *server_response;
    int server_id;
} response;


queue_t *q_create(unsigned int data_size, unsigned int max_size);

unsigned int q_get_size(queue_t *q);

unsigned int q_is_empty(queue_t *q);

void *q_front(queue_t *q);

int q_dequeue(queue_t *q);

int q_enqueue(queue_t *q, void *new_data);

void q_clear(queue_t *q);

void q_free(queue_t *q);

void dotask(server *s, int server_id);



server *init_server(unsigned int cache_size, int server_id);

/**
 * @brief Should deallocate completely the memory used by server,
 *     taking care of deallocating the elements in the queue, if any,
 *     without executing the tasks
 */
void free_server(server **s);

/**
 * server_handle_request() - Receives a request from the load balancer
 *      and processes it according to the request type
 * 
 * @param s: Server which processes the request.
 * @param req: Request to be processed.
 * 
 * @return response*: Response of the requested operation, which will
 *      then be printed in main.
 * 
 * @brief Based on the type of request, should call the appropriate
 *     solver, and should execute the tasks from queue if needed (in
 *     this case, after executing each task, PRINT_RESPONSE should
 *     be called).
 */
response *server_handle_request(server *s, request *req, int server_id);

#endif  /* SERVER_H */
