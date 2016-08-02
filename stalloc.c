#include <stdlib.h>
#include <string.h>

#include "stalloc.h"

struct st_alloc_t {
    int nb_pools;
    char ** mem_pools;
    size_t * pools_size;
};

struct st_alloc_t * st_init() {
    struct st_alloc_t * r = (struct st_alloc_t *) malloc(sizeof(struct st_alloc_t));
    
    if (r) {
        r->nb_pools = 0;
        r->mem_pools = 0;
        r->pools_size = 0;
    }
    
    return r;
}

#define POOL_SIZE 16384

static int create_pool(struct st_alloc_t * s) {
    s->nb_pools++;
    // No error checking, because it'd be a pain to handle.
    s->mem_pools = (char **) realloc(s->mem_pools, sizeof(char *) * s->nb_pools);
    s->pools_size = (size_t *) realloc(s->pools_size, sizeof(size_t) * s->nb_pools);
    s->mem_pools[s->nb_pools - 1] = malloc(POOL_SIZE);
    s->pools_size[s->nb_pools - 1] = 0;
    
    return s->nb_pools - 1;
}

static int search_pool(struct st_alloc_t * s, size_t siz) {
    int i;
    
    // This algorithm could use a little bit more of cleverness, but... *shrug*
    // the whole purpose is to be used for small static strings, so...
    for (i = 0; i < s->nb_pools; i++) {
        if (siz <= (POOL_SIZE - s->pools_size[i]))
            return i;
    }
    
    return -1;
}

void * st_alloc(struct st_alloc_t * s, size_t siz) {
    int pool;
    
    if (siz > POOL_SIZE) {
        pool = create_pool(s);
        s->mem_pools[pool] = realloc(s->mem_pools[pool], siz);
        s->pools_size[pool] = siz;
        return s->mem_pools[pool];
    }
    
    pool = search_pool(s, siz);
    if (pool < 0)
        pool = create_pool(s);
    
    s->pools_size[pool] += siz;
    
    return s->mem_pools[pool] + s->pools_size[pool] - siz;
}

char * st_strdup(struct st_alloc_t * s, const char * src) {
    int siz = strlen(src) + 1;
    char * r = (char *) st_alloc(s, siz);
    memcpy(r, src, siz);
    
    return r;
}

void st_destroy(struct st_alloc_t * s) {
    int i;
    
    for (i = 0; i < s->nb_pools; i++) {
        free(s->mem_pools[i]);
    }
    free(s->mem_pools);
    free(s->pools_size);
    
    free(s);
}
