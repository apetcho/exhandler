#include "exhandler.h"

/********************************************************************/
/*                 Allocation routines Implementation               */
/********************************************************************/
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

/********************************************************************/
/*                  Assertion routine Implementation                */
/********************************************************************/
// -- 39
void exhhandle_assertion(
    Context *cptr, int flag, char *expr, char *filename, int lineno
){
    Scope scope = exhget_scope(cptr);
    if(scope == TRY_SCOPE || scope == CATCH_SCOPE || scope == FINALLY_SCOPE){
        if(cptr == NULL){
            cptr = exhget_context(NULL);
        }
        exhthrow(cptr, FailedAssertionError, expr, filename, lineno);
    }else{
        fprintf(
            stderr, "Assertion Failure %s: %s, file \"%s\", line %d.\n",
            flag ? "" : "(no abort)", expr, filename, lineno
        );
        if(flag){ abort(); }
    }
}
