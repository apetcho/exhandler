#include "exhandler.h"

// -- 36
void* exhmem_calloc(
    Context *cptr, int num, int size, char *filename, int lineno
){
    void *mem;
    mem = calloc(num, (size_t)size);
    if(mem == NULL){
        exhthrow(cptr, OutOfMemoryError, NULL, filename, lineno);
    }

    return mem;
}

// -- 37
void* exhmem_malloc(
    Context *cptr, int size, char *filename, int lineno
){
    void *mem;
    mem = malloc((size_t)size);
    if(mem == NULL){
        exhthrow(cptr, OutOfMemoryError, NULL, filename, lineno);
    }

    return mem;
}

// -- 38
void* exhmem_realloc(
    Context *cptr, void *mem, int size, char *filename, int lineno
){
    void *segment;
    segment = realloc(mem, size);
    if(segment == NULL){
        exhthrow(cptr, OutOfMemoryError, NULL, filename, lineno);
    }
    return segment;
}
