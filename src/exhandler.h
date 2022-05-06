#ifndef __EXHANDLER_H_
#define __EXHANDLER_H_
#include<signal.h>
#include<setjmp.h>
#include<stdlib.h>
#include<stdio.h>

#define EXH_TINY_EXHANDLER 1        /* just a marker for the library */

#define EXH_NOTHING

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
    int len;
};

/**
 * @brief Allocate memory for a new stack.
 * 
 * The stack buffer size is increased when needed.
 * 
 * @return Stack* Pointer to the stack newly created.
 */
Stack* stack_new(void);

/**
 * @brief Free the stack buffer.
 * 
 * @param stack 
 */
void stack_delete(Stack *stack);

/**
 * @brief Free the stack including user data.
 * 
 * The user data is freeed using free(). This implies that the caller is
 * responsible that all user data is allocated using malloc() or calloc()
 * of C standard library.
 * 
 * @param stack 
 */
void stack_delete_with_data(Stack *stack);

/**
 * @brief Push user data object to stack buffer.
 * 
 * @param stack 
 * @param object 
 */
void stack_push(Stack *stack, void *object);

/**
 * @brief Pop object from stack buffer.
 * 
 * This routine removes an object from the top of the stack buffer.
 * This operation will fail if the stack is empty when DEBUG flag is enabled
 * otherwise will return NULL.
 * 
 * @param stack 
 * @return void* 
 */
void* stack_pop(Stack *stack);

/**
 * @brief Get object from stack buffer at index n.
 * 
 * @note This operation does adhere to the LIFO principle, but it is possible
 * with our implementation since the data buffer is a contiguous memory
 * location where the user data reside.
 * 
 * This operation will will fail if DEBUG flag is enabled, otherwise NULL
 * is returned.
 * 
 * @param stack 
 * @param n 
 * @return void* 
 */
void* stack_peek(Stack *stack, int n);

/**
 * @brief Get the number of object in stack buffer.
 * 
 * @return int 
 */
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
    ListNode *pointer;  // Last accessed node
    int len;
};

/**
 * @brief Create an empty list
 * 
 * @return List* 
 */
List* list_new(void);

/**
 * @brief Free list but not user data.
 * 
 * This routine free the list handle and the nodes, but does not free the user
 * data.
 * 
 * @param list 
 */
void list_delete(List *list);

/**
 * @brief Free list including user data.
 * 
 * The list handle and all nodes in the list are freeed.
 * The client of this routine is responsible that all nodes was allocated
 * with routines (e.g malloc or calloc of stdlib.h) compatible with free.
 * 
 * @param data 
 */
void list_delete_with_data(List *list);

/**
 * @brief Add node to the head of list.
 * 
 * @param list 
 * @param data 
 */
void list_prepend(List *list, void *data);

/**
 * @brief Add node to tail of list.
 * 
 * @param list 
 * @param data 
 */
void list_append(List *list, void *data);

/**
 * @brief Add node before last accessed node.
 * 
 * This routine add the specified data object value in the specified list
 * just before the last accessed node.
 * 
 * @param list 
 * @param data 
 */
void list_insert_before(List *list, void *data);

/**
 * @brief Add node after last access node.
 * 
 * @param list 
 * @param data 
 */
void list_insert_after(List *list, void *data);

/**
 * @brief Remove head node from list.
 * 
 * @param list 
 */
void* list_remove_head(List *list);

/**
 * @brief Remove tail node from list
 * 
 * @param list 
 */
void* list_remove_tail(List *list);


/**
 * @brief Remove a the specified node from list.
 * 
 * @param list 
 * @return void* 
 */
void* list_remove(List *list, void *data);

/**
 * @brief Remove last accessed node from list.
 * 
 * @param list 
 * @return void* 
 */
void* list_remove_last(List *list);

/**
 * @brief Get head data object value
 * 
 * @param list 
 * @return void* 
 */
void* list_get_head(List *list);

/**
 * @brief Get tail data object value
 * 
 * @param list 
 * @return void* 
 */
void* list_get_tail(List *list);

/**
 * @brief Get data object value from the last accessed node.
 * 
 * @param list 
 * @return void* 
 */
void* list_get_last(List *list);

/**
 * @brief Get next data object value
 * 
 * @param list 
 * @return void* 
 */
void* list_get_next(List *list);

/**
 * @brief Get previous data object value.
 * 
 * @param list 
 * @return void* 
 */
void* list_get_prev(List *list);

/**
 * @brief Return the number of nodes in list.
 * 
 * @param list 
 * @return int 
 */
int list_len(List *list);

/**
 * @brief Find data in list.
 * 
 * @param list 
 * @param data 
 * @return void* NULL if data is not in list otherwise return the object data 
 * value;
 */
void* list_find(List *list, void *data);

/**
 * @brief Split list just before last node.
 * 
 * The split is made just before the last accessed list node. A new list is
 * created for the part befor the last accessed node. The last accessed node
 * for the original list is not affected. The last accessed list node for
 * the new list is reset.
 * 
 * @param list 
 * @return List* The new list containing nodes before original last accessed
 * list node.
 */
List* list_split_before(List *list);

/**
 * @brief Split list just after last node
 * 
 * The split is performed just after the last accest node. a new list is
 * created for the part after the last accessed node. The last accessed
 * node of the original list is node affected. The last accessed node for
 * the new list is reset.
 * 
 * @param list 
 * @return List* The list containing nodes after original last accessed node.
 */
List* list_split_after(List *list);

/**
 * @brief Concatenate two list
 * 
 * The second list is appened to first list. The last accest node of the
 * resulting list is reset.
 * 
 * @param list1 
 * @param list2 
 * @return List* The resulting list.
 */
List* list_merge(List *list1, List* list2);


// ----------------------------------------------------------------------
//                       DICTIONARY (aka HASHTABLE) API
// ----------------------------------------------------------------------
struct Dict{
    List **bucket;
    int len;
};

/**
 * @brief Create an empty dictionary (aka Hash table).
 * 
 * @return Dict* 
 */
Dict* dict_new();

/**
 * @brief Free hash table.
 * 
 * @param dict 
 */
void dict_delete(Dict *dict);

/**
 * @brief Free hash table including user data.
 * 
 * This routine free the memory that is occupied by the the hash table. The
 * user data in each entry is also freed using free(). This implies that
 * the caller is responsible that all of this user data was allocated 
 * using routines compatible with free.
 * 
 * @param dict 
 */
void dict_delete_with_data(Dict *dict);

/**
 * @brief Find data node for a given key
 * 
 * @param dict 
 * @param key 
 * @return void* 
 */
void* dict_get(Dict *dict, int key);

/**
 * @brief Add node to hash table.
 * 
 * @param dict 
 * @param key 
 * @param data 
 */
void dict_put(Dict *dict, int key, void *data);

/**
 * @brief Remove node from hash table.
 * 
 * @param dict 
 * @param key 
 * @return void* 
 */
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

#define exh_validate(cond, retval)             \
    if(cond){}                                 \
    else{ assert(e); return (retval); } 

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
