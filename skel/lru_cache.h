/*
 * Copyright (c) 2024, <>
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdbool.h>


typedef struct dll_node_t dll_node_t;
struct dll_node_t
{
    void* data;  // void* pebtru a putea stoca orice tip de date
    dll_node_t *prev, *next;
};

typedef struct info info;
struct info {
	void *key;
	void *value;
	dll_node_t *point;
};


typedef struct doubly_linked_list_t doubly_linked_list_t;
struct doubly_linked_list_t
{
    dll_node_t* head;
    unsigned int data_size;
    unsigned int size;
};

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
	doubly_linked_list_t **buckets; /* Array de liste simplu-inlantuite. */
	/* Nr. total de noduri existente curent in toate bucket-urile. */
	unsigned int size;
	unsigned int hmax; /* Nr. de bucket-uri. */
	/* (Pointer la) Functie pentru a calcula valoarea hash asociata cheilor. */
	unsigned int (*hash_function)(void*);
	/* (Pointer la) Functie pentru a compara doua chei. */
	int (*compare_function)(void*, void*);
};

typedef struct lru_cache {
    /* TODO */
	unsigned int cache_capacity;
    hashtable_t *cache;
	doubly_linked_list_t *waitingroom;
} lru_cache;

doubly_linked_list_t* dll_create(unsigned int data_size);

dll_node_t* dll_get_nth_node(doubly_linked_list_t* list, unsigned int n);

void dll_add_nth_node(doubly_linked_list_t* list,
					  unsigned int n, const void* data);

dll_node_t* dll_remove_nth_node(doubly_linked_list_t* list, unsigned int n);

void dll_free(doubly_linked_list_t** pp_list);

void dll_print_int_list(doubly_linked_list_t* list);

void dll_print_string_list(doubly_linked_list_t* list);


int compare_function_ints(void *a, void *b);

int compare_function_strings(void *a, void *b);

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*));

int ht_has_key(lru_cache *cache, void *key);

void *ht_get(hashtable_t *ht, void *key);

void ht_remove_entry(lru_cache *cache, void *key);

void ht_put(lru_cache *ht, void *key, void *value);

void ht_free(hashtable_t *ht);

unsigned int ht_get_size(lru_cache *cache);

unsigned int ht_get_hmax(hashtable_t *ht);

lru_cache *init_lru_cache(unsigned int cache_capacity);

bool lru_cache_is_full(lru_cache *cache);

void free_lru_cache(lru_cache **cache);

/**
 * lru_cache_put() - Adds a new pair in our cache.
 * 
 * @param cache: Cache where the key-value pair will be stored.
 * @param key: Key of the pair.
 * @param value: Value of the pair.
 * @param evicted_key: The function will RETURN via this parameter the
 *      key removed from cache if the cache was full.
 * 
 * @return - true if the key was added to the cache,
 *      false if the key already existed.
 */
bool lru_cache_put(lru_cache *cache, void *key, void *value,
                   void **evicted_key);

/**
 * lru_cache_get() - Retrieves the value associated with a key.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
 * 
 * @return - The value associated with the key,
 *      or NULL if the key is not found.
 */
void *lru_cache_get(lru_cache *cache, void *key);

/**
 * lru_cache_remove() - Removes a key-value pair from the cache.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
*/
void lru_cache_remove(lru_cache *cache, void *key);

#endif /* LRU_CACHE_H */
