/*
 * Copyright (c) 2024, <>
 */

#include <stdio.h>
#include "server.h"
#include "lru_cache.h"
#include "utils.h"

queue_t *
q_create(unsigned int data_size, unsigned int max_size)
{
	/* TODO */
	queue_t *queue = malloc(sizeof(queue_t));
	queue->buff = malloc(max_size *sizeof(void *));
	queue->max_size = max_size;
	queue->data_size = data_size;
	queue->size = 0;
	queue->read_idx = 0;
	queue->write_idx = 0;
	for(unsigned int i = 0; i < max_size; i++) {
		queue->buff[i] = malloc(data_size);
	}
	return queue;
}

/*
 * Functia intoarce numarul de elemente din coada al carei pointer este trimis
 * ca parametru.
 */
unsigned int
q_get_size(queue_t *q)
{
	/* TODO */
	return q->size;
}

/*
 * Functia intoarce 1 daca coada este goala si 0 in caz contrar.
 */
unsigned int
q_is_empty(queue_t *q)
{
	/* TODO */
	if (!q_get_size(q))
		return 1;
	return 0;
}

void *
q_front(queue_t *q)
{
	/* TODO */
    int last = q->read_idx;
	return q->buff[last];
}

int
q_dequeue(queue_t *q)
{
	if (q_is_empty(q)) {
		return 0;
	} else {
	q->read_idx = (q->read_idx + 1) % q->max_size;
	q->size--;
	return 1;
	}
	return 0;
}

/* 
 * Functia introduce un nou element in coada. Se va intoarce 1 daca
 * operatia s-a efectuat cu succes (nu s-a atins dimensiunea maxima) 
 * si 0 in caz contrar.
 */
int
q_enqueue(queue_t *q, void *new_data)
{
	/* TODO */
	if (q->size == q->max_size) {
		return 0;
	} else {
	q->size++;
	memcpy(q->buff[q->write_idx], new_data, q->data_size);
	q->write_idx = (q->write_idx + 1) % q->max_size;
	return 1;
	}
	return 0;
}

/*
 * Functia elibereaza toata memoria ocupata de coada.
 */
void
q_free(queue_t *q)
{
	/* TODO */
	unsigned int i = 0;
	while (!q_is_empty(q)) {
		request *req = (request *)q_front(q);
		free(req->doc_name);
		free(req->doc_content);
		q_dequeue(q);
	}
	for(i = 0; i < q->max_size; i++) {
		free(q->buff[i]);
	}
	free(q->buff);
	free(q);
}

response *malloc_response(int server_id) {
	response *res = malloc(sizeof(response));
	res->server_log = malloc(MAX_LOG_LENGTH);
	res->server_response = malloc(MAX_RESPONSE_LENGTH);
	res->server_id = server_id;
	return res;
}

static response
*server_edit_document(server *s, char *doc_name,
					  char *doc_content, int server_id) {
	/* TODO */
	// fac return la o structura response cu detaliile din cerinta
	request req;
	req.doc_name = malloc(strlen(doc_name) + 1);
	strcpy(req.doc_name, doc_name);
	req.doc_content = malloc(strlen(doc_content) + 1);
	strcpy(req.doc_content, doc_content);
	q_enqueue(s->task_queue, &req);
	response *res = malloc_response(server_id);
	sprintf(res->server_log, LOG_LAZY_EXEC, s->task_queue->size);
	sprintf(res->server_response, MSG_A, EDIT_REQUEST, doc_name);
	return res;
}

int is_in_server(doubly_linked_list_t *memory, void *key) {
	// verific daca este in server
	dll_node_t *curr = memory->head;
	for(unsigned int i = 0; i < memory->size; i++) {
		if (strcmp((char *)key, (char *)((*(info*)curr->data).key)) == 0) {
			return 1;
		}
		curr = curr->next;
	}
	return 0;
}

void *get_from_server(doubly_linked_list_t *memory, char *doc_name) {
	// iau continut din server
	dll_node_t *curr = memory->head;
	for(unsigned int i = 0; i < memory->size; i++) {
		char *key = (char *)((*(info*)curr->data).key);
		if (strcmp((char *)doc_name, key) == 0) {
			 return (*(info*)curr->data).value;
		}
		curr = curr->next;
	}
	return 0;
}

void updating_memory(doubly_linked_list_t *memory,
					 char *doc_name, char *doc_content) {
	dll_node_t *curr = memory->head;
	for(unsigned int i = 0; i < memory->size; i++) {
		if (strcmp((char *)doc_name, (char *)((*(info*)curr->data).key)) == 0) {
			info *data = (info *)curr->data;
			free(data->value);
			data->value = malloc(strlen((char *)doc_content) + 1);
			memcpy(data->value, doc_content, strlen((char *)doc_content) + 1);
			break;
		}
		curr = curr->next;
	}
}

void evicted(void *evicted, request *req, response *res) {
	// se face verificarea cheii evictate si se alege mesaj corespunzator
	if(evicted) {
		sprintf(res->server_log, LOG_EVICT, req->doc_name, (char *)evicted);
		free(evicted);
	} else {
		sprintf(res->server_log, LOG_MISS, req->doc_name);
	}
}

void case1(server *s, response *res, request *req) {
	updating_memory(s->memoryroom, req->doc_name, req->doc_content);
	void *evicted_key;
	lru_cache_remove(s->cache, req->doc_name);
	lru_cache_put(s->cache, req->doc_name, req->doc_content, &evicted_key);
	sprintf(res->server_log, LOG_HIT, req->doc_name);
	sprintf(res->server_response, MSG_B, req->doc_name);
}

void case2(server *s, response *res, request *req) {
	updating_memory(s->memoryroom, req->doc_name, req->doc_content);
	void *evicted_key;
	lru_cache_put(s->cache, req->doc_name, req->doc_content, &evicted_key);
	evicted(evicted_key, req, res);
	sprintf(res->server_response, MSG_B, req->doc_name);
}

void case3(server *s, response *res, request *req) {
	info data;
	data.key = malloc(strlen(req->doc_name) + 1);
	data.value = malloc(strlen(req->doc_content) + 1);
	memcpy(data.key, req->doc_name, strlen(req->doc_name) + 1);
	memcpy(data.value, req->doc_content, strlen(req->doc_content) + 1);
	dll_add_nth_node(s->memoryroom, 0, &data);
	void *evicted_key;
	lru_cache_put(s->cache, req->doc_name, req->doc_content, &evicted_key);
	evicted(evicted_key, req, res);
	sprintf(res->server_response, MSG_C, req->doc_name);
}

void dotask(server *s, int server_id) {
	// am urmarit arborele din cerinta si am facut functie pentru fiecare caz
	while(q_is_empty(s->task_queue) == 0) {
		request *req = (request *)q_front(s->task_queue);
		response *res = malloc_response(server_id);
		if(ht_has_key(s->cache, req->doc_name)) {
			case1(s, res, req);
		} else if (is_in_server(s->memoryroom, req->doc_name)) {
			case2(s, res, req);
		} else {
			case3(s, res, req);
		}
		PRINT_RESPONSE(res);
		free(req->doc_name);
		free(req->doc_content);
		q_dequeue(s->task_queue);
	}
}

void evicted_get(server *s, void *evicted, response *res, char *doc_name) {
	if(evicted) {
		sprintf(res->server_log, LOG_EVICT, doc_name, (char *)evicted);
		char *content = (char *)get_from_server(s->memoryroom, doc_name);
		if(content) {
			strcpy(res->server_response, content);
		}
		free(evicted);
	} else {
		sprintf(res->server_log, LOG_MISS, doc_name);
		char *content = (char *)get_from_server(s->memoryroom, doc_name);
		if(content) {
			strcpy(res->server_response, content);
		}
	}
}

void caseget1(server *s, response *resp, char *doc_name) {
	sprintf(resp->server_log, LOG_HIT, doc_name);
	char *content = (char *)lru_cache_get(s->cache, doc_name);
	memcpy(resp->server_response, content, strlen(content) + 1);
	lru_cache_remove(s->cache, doc_name);
	void *evicted_key;
	lru_cache_put(s->cache, doc_name, resp->server_response, &evicted_key);
}

void caseget2(server *s, response *resp, char *doc_name) {
	void *evicted_key;
	void *content = get_from_server(s->memoryroom, doc_name);
	lru_cache_put(s->cache, doc_name, content, &evicted_key);
	evicted_get(s, evicted_key, resp, doc_name);
}

void caseget3(response *resp, char *doc_name) {
	sprintf(resp->server_log, LOG_FAULT, doc_name);
	sprintf(resp->server_response, MSG_D);
}

static response
*server_get_document(server *s, char *doc_name, int server_id) {
	response *resp = malloc_response(server_id);
// alocam o variabila de tip response deoarece trebuie sa returnam un raspuns
	if (ht_has_key(s->cache, doc_name)) {
		// am urmarit acel arbore din cerinta si am facut functie pentru cazuri
		caseget1(s, resp, doc_name);
	} else if (is_in_server(s->memoryroom, doc_name)) {
		caseget2(s, resp, doc_name);
	} else {
		caseget3(resp, doc_name);
	}
	return resp;
}

server *init_server(unsigned int cache_size, int server_id) {
	/* TODO */
	server *s = malloc(sizeof(server));
	s->cache = init_lru_cache(cache_size);
	s->task_queue = q_create(sizeof(request), TASK_QUEUE_SIZE);
	s->memoryroom = dll_create(sizeof(info));
	s->server_id = server_id;
	return s;
}

response *server_handle_request(server *s, request *req, int server_id) {
	/* TODO */
	if(req->type == GET_DOCUMENT) {
		// precum scrie in cerinta mai intai facem toate taskurile
		dotask(s, server_id);
		return server_get_document(s, req->doc_name, server_id);
	} else if (req->type == EDIT_DOCUMENT) {
		return server_edit_document(s, req->doc_name, req->doc_content, server_id);
	}
	return NULL;
}

void free_server(server **s) {
	/* TODO */
	for(unsigned int i = 0; i < (*s)->cache->cache->hmax; i++) {
		free((*s)->cache->cache->buckets[i]);
	}
	q_free((*s)->task_queue);
	dll_free(&(*s)->memoryroom);
	dll_free(&(*s)->cache->waitingroom);
	free((*s)->cache->cache->buckets);
	free((*s)->cache->cache);
	free((*s)->cache);
	free((*s));
}
