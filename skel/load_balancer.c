/*
 * Copyright (c) 2024, <>
 */

#include "load_balancer.h"
#include "server.h"

unsigned int get_bonito(unsigned int hash1,
                        unsigned int hash2, unsigned int hash3) {
    unsigned int correct_hash;
    if (hash2 < hash1 && hash3 <= hash2) {
        correct_hash = hash2;
    } else if (hash2 < hash1 && hash3 <= hash1) {
        correct_hash = hash1;
    } else if (hash2 < hash1 && hash3 > hash1) {
        correct_hash = hash2;
    } else if (hash2 >= hash1 && hash3 < hash1) {
        correct_hash = hash1;
    } else if (hash2 >= hash1 && hash3 < hash2) {
        correct_hash = hash2;
    } else if (hash2 >= hash1 && hash3 >= hash2) {
        correct_hash = hash1;
    }
    return correct_hash;
}

void free_node(dll_node_t **node) {
    free((*(info*)(*node)->data).key);
    free((*(info*)(*node)->data).value);
    free((*node)->data);
    free(*node);
    *node = NULL;
}

void removing_connections(dll_node_t **curr) {
    dll_node_t* next = (*curr)->next;
    dll_node_t* prev = (*curr)->prev;
    (*curr)->next = NULL;
    (*curr)->prev = NULL;
    next->prev = prev;
    prev->next = next;
}

void distribute_docs(server* source_s, server* dest_s, int remove_server)
{
    // calculez hashurile pentru serverul sursa si destinatie
    unsigned int hash_dest = hash_uint(&(dest_s->server_id));
    unsigned int hash_source = hash_uint(&(source_s->server_id));
    dll_node_t *curr = source_s->memoryroom->head;
    for (unsigned int i = source_s->memoryroom->size; i > 0; i--) {
        // calculez hashul de verificare pentru a respecta conditiile cerintei
        unsigned int verification = get_bonito(hash_source, hash_dest,
        hash_string((*(info*)curr->data).key));
        if ((verification == hash_dest) || remove_server) {
            if (ht_has_key(source_s->cache,
                (char *)((*(info*)curr->data).key))) {
                // sterg din cache ul severului sursa
                lru_cache_remove(source_s->cache,
                                 (char *)((*(info*)curr->data).key));
            }
            info data;
            data.key = malloc(strlen((char *)((*(info*)curr->data).key)) + 1);
            data.value =
            malloc(strlen((char *)((*(info*)curr->data).value)) + 1);
            strcpy((char *)data.key, (char *)((*(info*)curr->data).key));
            strcpy((char *)data.value, (char *)((*(info*)curr->data).value));
            // copiez si adaug in memorie
            dll_add_nth_node(dest_s->memoryroom, 0, &data);
            // sterg nodul
            dll_node_t *totnext = curr->next;
            removing_connections(&curr);
            if (i == source_s->memoryroom->size) {
                source_s->memoryroom->head = totnext;
            }
            source_s->memoryroom->size--;
            free_node(&curr);
            curr = totnext;
        } else {
            curr = curr->next;
        }
    }
}

load_balancer *init_load_balancer(bool enable_vnodes) {
    if (!enable_vnodes) {
        load_balancer *main = (load_balancer *)malloc(sizeof(load_balancer));
        main->hashring = malloc(MAX_SERVERS * sizeof(int));
        main->hash_function_servers = &hash_uint;
        main->servers = malloc(MAX_SERVERS * sizeof(server *));
        main->hash_function_docs = &hash_string;
        main->number_of_servers = 0;
        return main;
    } else {
	    load_balancer *main = (load_balancer *)malloc(sizeof(load_balancer));
        main->hashring = malloc(MAX_SERVERS * sizeof(int));
        main->hash_function_servers = &hash_uint;
        main->servers = malloc(MAX_SERVERS * sizeof(server *));
        main->hash_function_docs = &hash_string;
        main->number_of_servers = 0;
        return main;
    }
    return 0;
}

void reorganize(load_balancer** main, int j, int remove_server) {
    if (remove_server) {
        for (int i = j; i < (*main)->number_of_servers - 1; i++) {
            (*main)->hashring[i] = (*main)->hashring[i + 1];
        }
    } else {
        for (int i = (*main)->number_of_servers - 2; i >= j; i--) {
                (*main)->hashring[i + 1] = (*main)->hashring[i];
        }
    }
}

void loader_add_server(load_balancer* main, int server_id, int cache_size) {
    // initializez server nou
    main->servers[server_id] = init_server(cache_size, server_id);
    main->hashring[main->number_of_servers] = server_id;
    main->number_of_servers++;
    int source_id = main->hashring[0];
    // umblu in vectorul de servere si verific conditia impusa de cerinta
    for (int i = 0; i < main->number_of_servers - 1; i++) {
        if (hash_uint(&server_id) <
            hash_uint(&(main->hashring[i])) ||
            (hash_uint(&server_id) == hash_uint(&(main->hashring[i]))
            && server_id < main->hashring[i])) {
            source_id = main->hashring[i];
            // reorganizez hashringul si pun serverele in noua ordine
            reorganize(&main, i, 0);
            main->hashring[i] = server_id;
            break;
        }
    }
    if (source_id - server_id != 0) {
        // fac taskurile si distribui documentele
        dotask(main->servers[source_id % (MAX_SERVERS + 1)], source_id);
        distribute_docs(main->servers[source_id],
                               main->servers[server_id], 0);
    }
}

void loader_remove_server(load_balancer* main, int server_id) {
    int destination_id = main->hashring[0];
    // am initializat serverul destinatar cu primul server din hashring
    for (int i = 0; i < main->number_of_servers - 1; i++) {
        if (server_id == main->hashring[i]) {
            // reorganizez hashringul
            reorganize(&main, i, 1);
            destination_id = main->hashring[i];
            break;
        }
    }
    int if_state = destination_id - server_id;
    if (if_state) {
        // fac taskurile si distribui documentele
        dotask(main->servers[server_id], server_id);
        distribute_docs(main->servers[server_id],
                               main->servers[destination_id], 1);
    }
    main->number_of_servers--;
    free_server(&(main->servers[server_id]));
}

int get_server_request(load_balancer* lb, unsigned int hash) {
    int returner = lb->hashring[0];
    int cnt = lb->number_of_servers;
    int i = 0;
    // caut serverul pentru document precum in cerinta
    while (i != cnt) {
        if (hash <= hash_uint(&(lb->hashring[i]))) {
            returner = lb->hashring[i];
            break;
        }
        i++;
    }
    return returner;
}

response *loader_forward_request(load_balancer* main, request *req) {
    unsigned int hash = hash_string(req->doc_name);
    // calculez hashul documentului si verific pe ce server trebuie pus
    int server_id = get_server_request(main, hash);
    return server_handle_request(main->servers[server_id], req, server_id);
}

void free_load_balancer(load_balancer** main) {
    for (int i = 0; i < (*main)->number_of_servers; i++) {
        free_server(&(*main)->servers[(*main)->hashring[i]]);
    }
    free((*main)->hashring);
    free((*main)->servers);
    free(*main);
    *main = NULL;
}
