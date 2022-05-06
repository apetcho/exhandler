#ifndef __EXHANDLER_H_
#define __EXHANDLER_H_
#include<signal.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>

#define TINY_EXHANDLER 1        /* just a marker for the library */

typedef struct Stack Stack;
typedef struct ListNode ListNode;
typedef struct List List;
typedef struct Dict Dict;
typedef struct Type *ObjectRef;
typedef struct Type Object[1];
typedef struct ExceptionType ExceptionType;
typedef struct Context Context;
typedef enum Scope Scope;
typedef enum State State;

// ----------------------------------------------------------------------
//                              STACK API
// ----------------------------------------------------------------------

struct Stack{
    void **data;
    int size;
    int pointer;
};

Stack* stack_new(void);
void stack_delete(Stack *stack);
void stack_delete_with_data(Stack *stack);
void stack_push(Stack *stack, void *object);
void* stack_pop(Stack *stack);
void* stack_peek(Stack *stack, int n);
int stack_len(Stack *);

// ----------------------------------------------------------------------
//                              LIST API
// ----------------------------------------------------------------------
struct ListNode{
    ListNode *next;
    ListNode *prev;
    void *data;
};

struct List{
    ListNode *head;
    ListNode *tail;
    int count;
};

List* list_new(void);
void list_delete(ListNode *list);
void list_delete_with_data(List *data);
void list_prepend(List *list, void *data);
void list_append(List *list, void *data);
void list_insert_before(List *list, void *data);
void list_insert_after(List *list, void *data);
void list_remove_head(List *list);
void list_remove_tail(List *list);
void list_remove_last(List *list);
void list_remove(List *list);
void* list_get_head(List *list);
void* list_get_tail(List *list);
void* list_get_last(List *list);
void* list_get_next(List *list);
void* list_get_prev(List *list);
int list_len();
void* list_find(List *list, void *data);
List* list_split_before(List *list);
List* list_split_after();
List* list_merge(List *list1, List* list2);


// ----------------------------------------------------------------------
//                       DICTIONARY (aka HASHTABLE) API
// ----------------------------------------------------------------------
struct Dict{
    List **lists;
    int count;
};

Dict* dict_new();
void dict_delete(Dict *dict);
void dict_delete_with_data(Dict *dict);
void* dict_get(Dict *dict, int key);
void dict_put(Dict *dict, int key, void *data);
void* dict_remove(Dict *dict, int key);
int dict_len(Dict *dict); 

// ----------------------------------------------------------------------
//           EXCEPTION, EXCETION CONTEXT & EXCEPTION OBJECT API
// ----------------------------------------------------------------------

/** MACROS */

// -- allocation api --
#define exh_mem_new(T) exhmem_calloc(cptr, 1, sizeof(T), __FILE__, __LINE__)
#define exh_mem_calloc(n, size) exhmem_calloc(cptr, n, size, __FILE__, __LINE__)
#define exh_mem_malloc(size) exhmem_malloc(cptr, size, __FILE__, __LINE__)
#define exh_mem_realloc(p, size) exhmem_realloc(\
    cptr, p, size, __FILE__, __LINE__)

void* exhmem_calloc(
    Context *cptr, int num, int size, char *filename, int lineno);
void* exhmem_malloc(
    Context *cptr, int num, int size, char *filename, int lineno);
void* exhmem_realloc(
    Context *cptr, void *mem, int num, int size, char *filename, int lineno);


// -- assertion api --

#ifdef EXH_ASSERT_ABORT
#define EXH_ABORT   1
#else
#define EXH_ABORT   0
#endif

#undef assert
#ifdef DEBUG
#define assert(e)\
    if(e){}     \
    else{exhhandle_assertion(cptr, EXH_ABORT, #e, __FILE__, __LINE__); }
#else
#define assert(e)
#endif

#define exh_validate(e, r)          \
    if(e){}                         \
    else{ assert(e); return r; } 

#define exh_check(e, n) \
    if(e){} \
    else{assert(e); throw(n, NULL); }

void* exhhandle_assertion(
    Context *cptr, int flag, char *expr, char *filename, int lineno);

// --- exception and other api ---

#define EXH_SETJMP(env)         sigsetjmp(env, 1)
#define EXH_LONGJMP(env, val)   siglongjmp(env, val)
#define EXH_JMP_BUF             sigjmp_buf


typedef void (*exh_sighandlerFn)(int);

struct Type{
    int norethrow;
    ObjectRef master;
    int signum;
};

enum Scope{ OUT_SCOPE=-1, IN_SCOPE, TRY_SCOPE, CATCH_SCOPE, FINALLY_SCOPE };
enum State{ EMPTY_STATE, PENDING_STATE, CAUGHT_STATE };

struct ExceptionType{
    int norethrown;
    State state;
    EXH_JMP_BUF throwbuf;
    EXH_JMP_BUF finalbuf;
    ObjectRef object;
    void *data;
    char *erfile;
    int lineno;
    int ready;
    Scope scope;
    int first;
    List *checklist;
    char *tryfile;
    int trylineno;
    ObjectRef (*get_class)(void);
    char* (*describe)(void); // getMessage
    void* (*get_data)(void);
    void (*print_stacktrace)(FILE *);
};

struct Context{
    ExceptionType *except;
    Stack *stack;
    char description[1024];
    exh_sighandlerFn aborthandler;
    exh_sighandlerFn fpehandler;
    exh_sighandlerFn segvhandler;
    exh_sighandlerFn bushandler;
};

extern Context *cptr;
extern Object Throwable;

#define EXH_DECLARE(self, master)   extern Object self
#define EXH_DEFINE(self, master)    Object self = {1, master, #self }

EXH_DECLARE(Exception, Throwable);
EXH_DECLARE(OutOfMemoryError, Exception);
EXH_DECLARE(FailedAssertionError, Exception);
EXH_DECLARE(RuntimeError, Exception);
EXH_DECLARE(AbnormalTerminationError, RuntimeError);    /* SIGABRT */
EXH_DECLARE(ArithmethicError, RuntimeError);            /* SIGFPE */
EXH_DECLARE(IllegalInstructionError, RuntimeError);     /* SIGABRT */
EXH_DECLARE(SegmentationError, RuntimeError);           /* SIGABRT */
EXH_DECLARE(BusError, RuntimeError);                    /* SIGABRT */


#ifdef DEBUG
#define EXH_CHECKED static int checked
#define EXH_CHECK_BEGIN(ptr, flag, file, linenum) \
    exhcheck_begin(ptr, flag, file, linenum)

#define EXH_CHECK(ptr, flag, obj, file, linenum)    \
    exhcheck(ptr, flag, obj, file, linenum)

#define EXH_CHECK_ENND  !checked
#else
#define EXH_CHECKED 
#define EXH_CHECK_BEGIN(ptr, flag, file, linenum)   1
#define EXH_CHECK(ptr, flag, obj, file, linenum)    1
#define EXH_CHECK_END                               0
#endif /* DEBUG */

#define exh_thread_cleanump(tid) exhthread_cleanup(tid)

#define try                                     \
    exhtry(cptr, __FILE__, __LINE__);           \
    while(1){                                   \
        Context *tmpc = exhget_context(cptr);   \
        Context *cptr = tmpc;                   \
        EXH_CHECKED;                            \
        if(EXH_CHECK_BEGIN(cptr, &checked, __FILE__, __LINE__) && \
            cptr->except->ready && EXH_SETJMP(cptr->except->throwbuf)==0) \
        {                                       \
            cptr->except->scope = TRY_SCOPE;    \
            do{

#define catch(obj, e) }while(0);                                    \
    }else if(EXH_CHECK(cptr, &checked, obj, __FILE__, __LINE__) &&  \
        cptr->except->read && exhcatch(cptr, obj))                  \
    {                                                               \
        ExceptionType *except = stack_peek(cptr->stack, 1);         \
        cptr->except->scope = CATCH_SCOPE;                          \
        do{

#define finally     } while(0);     \
        }                               \
        if(EXH_CHECK_END){continue;}    \
        if(!cptr->except->ready && EXH_SETJMP(cptr->except->finalbuf)==0)   \
        {cptr->except->ready = 1; }else{ break; }                           \
    }                                                                       \
    exhget_context(cptr)->except->scope = FINALLY_SCOPE;                    \
    while(exhget_context(cptr)->except->ready > 0|| exhfinally(cptr))       \
        while(exhget_context(cptr)->except->ready-- > 0)

#define throw(obj, data)    \
    exhthrow(cptr, (ObjectRef)obj, data, __FILE__, __LINE__)

#define exh_return(x) {                             \
        if(exhget_scope(cptr) != OUT_SCOPE){            \
            void *data = malloc(sizeof(EXH_JMP_BUF));   \
            exhget_context(cptr)->except->data = data;  \
            if(EXH_SETJMP(*(EXH_JMP_BUF *)data)==0){ exhreturn(cptr);}\
            else{ free(cptr);} \
        }\
        return x; \
    }

#define pending     (exhget_context(cptr)->except->state == PENDING_STATE)

Scope exhget_scope(Context *cptr);
Context* exhget_context(Context *cptr);
void exhthread_cleanup(int tid);
void exhtry(Context *cptr);
void exhthrow(
    Context *cptr, void *except, void *data, char *filename, int lineno);
int exhcatch(
    Context *cptr, ObjectRef object);
int exhfinally(Context *cptr);
void exhreturn(Context *cptr);
int exhcheck_begin(
    Context *cptr, int *checked, char *filename, int lineno);
int exhcheck(
    Context *cptr, int *checked, ObjectRef object, char *filename, int lineno);

#endif /* __EXHANDLER_H_ */
