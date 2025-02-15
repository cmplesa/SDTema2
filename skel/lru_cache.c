/*
 * Copyright (c) 2024, <>
 */

#include <stdio.h>
#include <string.h>
#include "lru_cache.h"
#include "utils.h"
#include "load_balancer.h"

doubly_linked_list_t* dll_create(unsigned int data_size)
{
	/* TODO */
	doubly_linked_list_t *newlist = malloc(sizeof(doubly_linked_list_t));
	newlist->head = NULL;
	newlist->size = 0;
	newlist->data_size = data_size;
	return newlist;
}

dll_node_t *alocarenod(const void *data)
{
	dll_node_t *nodulet = malloc(sizeof(dll_node_t));
	nodulet->data = malloc(sizeof(info));
	memcpy(nodulet->data, data, sizeof(info));
	return nodulet;
}

dll_node_t
*dll_get_nth_node(doubly_linked_list_t *list, unsigned int n)
{
	if (!list)
		return NULL;
	dll_node_t *dorit = list->head;
	if (n == 0)
		return dorit;
	if (n < list->size / 2) {
		for (unsigned int i = 0; i < n; i++) {
			dorit = dorit->next;
		}
	} else {
		for (unsigned int i = 0; i < list->size - n; i++) {
			dorit = dorit->prev;
		}
	}
	return dorit;
}

void
dll_add_nth_node(doubly_linked_list_t* list,
                 unsigned int n, const void* new_data)
{
	/* TODO */
	dll_node_t *newfirst = alocarenod(new_data);
	newfirst->next = NULL;
	newfirst->prev = NULL;

	if(list->size == 0) {
		list->head = newfirst;
		newfirst->next = newfirst->prev = newfirst;
		list->size++;
		return;
	}

	dll_node_t *nnewfirst = dll_get_nth_node(list, n);
	dll_node_t *bnewfirst = nnewfirst->prev;
	bnewfirst->next = newfirst;
	nnewfirst->prev = newfirst;
	newfirst->next = nnewfirst;
	newfirst->prev = bnewfirst;
	if(n == 0) {
		list->head = newfirst;
	}
	list->size++;
}

unsigned int
dll_get_size(doubly_linked_list_t* list)
{
	/* TODO */
	return list->size;
}

void dll_free(doubly_linked_list_t** pp_list)
{
    unsigned int n = (*pp_list)->size;
    dll_node_t *aux = (*pp_list)->head;
	for(unsigned int i = 0 ; i < n; i++){
        dll_node_t *notdeleted = aux->next;
        removing_connections(&aux);
        free_node(&aux);
        aux = notdeleted;
    }
    free(*pp_list);
    pp_list = NULL;
}

int compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
unsigned int hash_function_int(void *a)
{
	/*
	 * Credits: https://stackoverflow.com/a/12996028/7883884
	 */
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_string(void *a)
{
	/*
	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
	 */
	unsigned char *puchar_a = (unsigned char*) a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

	return hash;
}

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*))
{
	/* TODO */
    hashtable_t *hasht = (hashtable_t *)malloc(sizeof(hashtable_t));
    hasht->hmax = hmax;
    hasht->size = 0;
    hasht->hash_function = hash_function;
    hasht->compare_function = compare_function;
    hasht->buckets = malloc(hmax * sizeof(doubly_linked_list_t *));
    for (unsigned int i = 0; i < hmax; i++) {
        hasht->buckets[i] = dll_create(sizeof(info));
    }
    return hasht;
}

int ht_has_key(lru_cache *cache, void *key)
{
	/* TODO */
    unsigned int index = cache->cache->hash_function(key) % cache->cache->hmax;
    doubly_linked_list_t *bucket = cache->cache->buckets[index];
    dll_node_t *curr =  bucket->head;
    int found = 0;
    for (int i = cache->cache->hmax; i > 0; i--) {
        if (curr && cache->cache->compare_function
            (key, (*(info*)curr->data).key) == 0) {
            found = 1;
            break;
        }
        index = (index + 1) % cache->cache->hmax;
        bucket = cache->cache->buckets[index];
        curr = bucket->head;
    }
	if (found) {
        return 1;
    } else {
        return 0;
    }
    return 0;
}

void *ht_get(hashtable_t *ht, void *key)
{
	/* TODO */
    unsigned int index = ht->hash_function(key) % ht->hmax;
    doubly_linked_list_t *bucket = ht->buckets[index];
    dll_node_t *curr =  bucket->head;
    for (int i = ht->hmax; i > 0; i--) {
        if (curr && ht->compare_function(key, (*(info*)curr->data).key) == 0) {
            void *content = (*(info*)curr->data).value;
            return content;
        } else {
        index = (index + 1) % ht->hmax;
        bucket = ht->buckets[index];
        curr = bucket->head;
        }
    }
	return NULL;
}

void copy(info *data, void *key, void *value)
{
    data->key = malloc(strlen((char *)key) + 1);
    data->value = malloc(strlen((char *)value) + 1);
    strcpy((char *)data->key, (char *)key);
    strcpy((char *)data->value, (char *)value);
}

void ht_put(lru_cache *cache, void *key, void *value)
{
	/* TODO */
    unsigned int index = cache->cache->hash_function(key) % cache->cache->hmax;
    doubly_linked_list_t *bucket = cache->cache->buckets[index];
    info data;
    // copiem toata
    copy(&data, key, value);
    dll_add_nth_node(cache->waitingroom, 0, &data);
    // acum se verifica daca cache-ul este full
    if (lru_cache_is_full(cache)) {
        dll_node_t *last = cache->waitingroom->head->prev;
        ht_remove_entry(cache, (*(info*)last->data).key);
    }

    int put = 0;
    while (!put) {
        if (!bucket->head){
            put = 1;
            bucket->head = cache->waitingroom->head;
        } else {
            index = (index + 1) % cache->cache->hmax;
            bucket = cache->cache->buckets[index];
        }
    }
    cache->cache->size++;
}


void ht_remove_entry(lru_cache *cache, void *key)
{
	/* TODO */
    unsigned int index = cache->cache->hash_function(key) % cache->cache->hmax;
    doubly_linked_list_t *bucket = cache->cache->buckets[index];
    dll_node_t *curr =  bucket->head;
    int cnt = cache->cache->hmax;
    while (cnt) {
        if (curr && cache->cache->compare_function
            (key, (*(info*)curr->data).key) == 0) {
            dll_node_t* deleted = curr;
            dll_node_t* stillnext = deleted->next;
            removing_connections(&deleted);
            if (cache->waitingroom->head == deleted)
                cache->waitingroom->head = stillnext;
            bucket->head = NULL;
            cache->waitingroom->size--;
            cache->cache->size--;
            free_node(&deleted);
            return;
        }
        index = (index + 1) % cache->cache->hmax;
        bucket = cache->cache->buckets[index];
        curr = bucket->head;
        cnt--;
    }
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable,
 * dupa care elibereaza si memoria folosita pentru a stoca structura hashtable.
 */
void ht_free(hashtable_t *ht)
{
    /* TODO */
    dll_node_t *aux;
	for (unsigned int i = 0; i < ht->hmax; i++) {
		if (ht->buckets[i]->head) {
			aux = ht->buckets[i]->head;
			while (aux) {
				free((*(info*)aux->data).key);
				free((*(info*)aux->data).value);
				aux = aux->next;
			}
		}
		dll_free(&ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
}

unsigned int ht_get_size(lru_cache *cache)
{
	return cache->cache->size;
}

unsigned int ht_get_hmax(hashtable_t *ht)
{
	return ht->hmax;
}

lru_cache *init_lru_cache(unsigned int cache_capacity) {
    /* TODO */
    // initializez lru ul cu specificatiile date
    lru_cache *cache = (lru_cache *)malloc(sizeof(lru_cache));
    cache->cache_capacity = cache_capacity;
    cache->cache = ht_create(cache_capacity,
                             &hash_string, &compare_function_strings);
    cache->waitingroom = dll_create(sizeof(info));
    return cache;
}

bool lru_cache_is_full(lru_cache *cache) {
    /* TODO */
    if (cache->cache->size == cache->cache_capacity) {
        return true;
    }
    return false;
}

void free_lru_cache(lru_cache **cache) {
    /* TODO */
    dll_free(&(*cache)->waitingroom);
    if (*cache) {
        ht_free((*cache)->cache);
        free(*cache);
    }
}

bool lru_cache_put(lru_cache *cache, void *key, void *value,
                   void **evicted_key) {
    /* TODO */
    if (ht_has_key(cache, key)) {
        // verific daca exista deja in cache
        return false;
    }
    // verific daca este full
    if (!lru_cache_is_full(cache)) {\
        // daca nu inseamna ca evicted_key este NULL si facem put
        *evicted_key = NULL;
        ht_put(cache, key, value);
        return true;
    } else {
        // daca nu este full aflam ultimul element adaugat si facem put
        dll_node_t *last = cache->waitingroom->head->prev;
        *evicted_key = malloc(strlen((char *)((*(info*)last->data).key)) + 1);
        memcpy(*evicted_key, (*(info*)last->data).key,
               strlen((char *)((*(info*)last->data).key)) + 1);
        ht_put(cache, key, value);
        return true;
    }
    return true;
}

void *lru_cache_get(lru_cache *cache, void *key) {
    /* TODO */
    return ht_get(cache->cache, key);
}

void lru_cache_remove(lru_cache *cache, void *key) {
    /* TODO */
    ht_remove_entry(cache, key);
}
