#ifndef __STALLOC_H__
#define __STALLOC_H__

struct st_alloc_t;

struct st_alloc_t * st_init();
void * st_alloc(struct st_alloc_t * s, size_t siz);
char * st_strdup(struct st_alloc_t * s, const char * src);
void st_destroy(struct st_alloc_t * s);

#endif
