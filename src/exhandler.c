#include<string.h>
#ifdef EXHANDLER_USE_PTHREAD
#include<pthread.h>
#endif
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

/********************************************************************/
/*                 Allocation routines Implementation               */
/********************************************************************/

Context *cptr = NULL;
Object Throwable = {.norethrow = 1, .parent=NULL, .name="Throwable", };

EXH_DEFINE(Exception, Throwable);
EXH_DEFINE(OutOfMemoryError, Exception);
EXH_DEFINE(FailedAssertionError, Exception);
EXH_DEFINE(RuntimeError, Exception);
EXH_DEFINE(AbnormalTerminationError, RuntimeError);    /* SIGABRT */
EXH_DEFINE(ArithmethicError, RuntimeError);            /* SIGFPE */
EXH_DEFINE(IllegalInstructionError, RuntimeError);     /* SIGABRT */
EXH_DEFINE(SegmentationError, RuntimeError);           /* SIGABRT */
EXH_DEFINE(BusError, RuntimeError);                    /* SIGABRT */

#if defined(EXHANDLER_SHARED_MEMORY) || defined(EXHANDLER_PRIVATE_MEMORY)
#define EXHANDLER_MULTI_THREADING     1
#ifdef EXHANDLER_USE_PTHREAD
#define EXHANDLER_THREAD_ID_FUNC        (int)pthread_self
#define EXHANDLER_THREAD_MUTEX_FUNC     exhmutex
#else
extern int EXHANDLER_THREAD_ID_FUNC(void);
extern int EXHANDLER_MUTEX_FUNC(int mode);
#endif
#else
#define EXHANDLER_MULTI_THREADING       0
#define EXHANDLER_THREAD_MUTEX_FUNC(mode)
#endif

#ifdef EXHANDLER_SHARED_MEMORY
#define EXHANDLER_SHARE     1
#else
#define EXHANDLER_SHARE     0
#endif

static Object ReturnEvent = {.norethrow=1, .parent=NULL, .name="ReturnEvent",};
static Context defaultContext;
static volatile Dict *contextDict;
static volatile int numThreadsTry;
static exh_sighandlerFn shared_abort_handlerfn;
static exh_sighandlerFn shared_fpe_handlerfn;
static exh_sighandlerFn shared_ill_handlerfn;
static exh_sighandlerFn shared_segv_handlerfn;
static exh_sighandlerFn shared_bus_handlerfn;

// ------------------------------------------------------------------
// exhmutex() - lock/unlock for thread shared data access
// ------------------------------------------------------------------
#ifdef EXHANDLER_USE_PTHREAD
// mode = 1 ==> lock
// mode = 0 ==> unlock
static void exhmutex(int mode){
    static volatile pthread_mutex_t mutex;
    static volatile sig_atomic_t initialized;
    static volatile sig_atomic_t ready;
    static volatile int count;
    static volatile pthread_t tid;

    if(!initialized && !initialized++){
        pthread_mutex_init(&mutex, NULL);
        ready = 1;
    }else{
        while(!ready){}
    }

    if(mode == 1){
        if(tid == pthread_self()){ count++; }
        else{
            pthread_mutex_lock(&mutex);
            tid = pthread_self();
            count = 1;
        }
    }else if(mode == 0){
        if(tid == pthread_self()){
            if(--count == 0){
                tid = 0;
                pthread_mutex_unlock(&mutex);
            }
        }
        else if(tid != 0 && tid != pthread_self()){
            fprintf(
                stderr,
                "exhandler internal error: thread attempts to unlock without "
                "holding lock\n"
            );
        }
    }
}
#endif

#ifdef EXHANDLER_DEBUG
static void exhprint_debug(Context *cptr, char *name){
    if(cptr == NULL){
        cptr exhget_context(NULL);
    }
    for(int i=(cptr && cptr->stack) ? stack_len(cptr->stack):0; i != 0; i--){
        fputs(" ", stderr);
    }
    fputs(name, stderr);
    fputc('\n', stderr);
}
#else
#define exhprint_debug(cpr, name)
#endif

// -- 40
Scope exhget_scope(Context *cptr){
    Scope scope;
    if(cptr == NULL){
        cptr = exhget_context(NULL);
    }
    exhprint_debug(cptr, "exhget_scope");
    if(cptr == NULL || cptr->except == NULL){ scope = OUTSITE_SCOPE; }
    else{
        scope = ((ExceptionType*)stack_peek(cptr->stack, 1))->scope;
    }
    return scope;
}

// -- 41
Context* exhget_context(Context *cptr){
#if EXHANDLER_MULTI_THREADING
    EXHANDLER_THREAD_MUTEX_FUNC(1)
    if(cptr == NULL && contextDict != NULL){
        cptr = dict_get(contextDict, EXHANDLER_THREAD_ID_FUNC());
    }
    EXHANDLER_THREAD_MUTEX_FUNC(0)
    return cptr;
#else
    return &defaultContext;
#endif
}

// -----------------------------------------------------------------
// exhnew_context() :: create exception handling context for thread
// -----------------------------------------------------------------
#if EXHANDLER_MULTI_THREADING
static Context* exhnew_context(void){
    Context *context;
    context = calloc(1, sizeof(Context));
    if(context == NULL){
        fprintf(stderr, "exhandler internal error: out of memory.\n");
    }
    EXHANDLER_THREAD_MUTEX_FUNC(1);
    dict_put(contextDict, EXHANDLER_THREAD_ID_FUNC(), context);
    EXHANDLER_THREAD_MUTEX_FUNC(0);
    exhprint_debug(context, "exhnew_conext");

    return context;
}
#else
#define exhnew_context()    NULL
#endif

// -----------------------------------------------------------------
// exhget_description()
// -----------------------------------------------------------------
static char* exhget_description(void){
    Context *context = exhget_context(NULL);
    exhprint_debug(context, "exhget_description");
    sprintf(
        context->description, "%s: file \"%s\", line %d.",
        context->except->class->name, context->except->filename,
        context->except->lineno
    );

    return context->description;
}

// -----------------------------------------------------------------
// exhget_class() :: get current exception class
// -----------------------------------------------------------------
static ObjectRef exhget_class(void){
    Context *context = exhget_context(NULL);
    exhprint_debug(context, "exhget_class");
    return context->except->class;
}

// -----------------------------------------------------------------
// exhget_data() :: get current exception associated data
// -----------------------------------------------------------------
static void* exhget_data(void){
    Context *context = exhget_context(NULL);
    exhprint_debug(context, "exhget_data");
    return context->except->data;
}

// -----------------------------------------------------------------
// exhprind_stacktrace() :: print the nested 'try' trace
// -----------------------------------------------------------------
static void exhprint_stacktrace(FILE *fileptr){
    Context *context = exhget_context(NULL);
    exhprint_debug(context, "exhprint_stacktrace");
    if(fileptr == NULL){ fileptr = stderr; }

#if EXHANDLER_MULTI_THREADING
    fprintf(
        fileptr, "%s occured in thread %d:\n",
        cptr->except->class->name, EXHANDLER_THREAD_ID_FUNC()
    );
#else
    fprintf(fileptr, "%s occured:\n", cptr->except->class->name);
#endif
    for(int i=1; i <= stack_len(cptr->except); i++){
        ExceptionType *except = stack_peek(context->stack, i);
        fprintf(
            fileptr, "      in 'try' at %s:%d\n",
            except->tryfile, except->trylineno
        );
    }
}

// -- 42
void exhthread_cleanup(int tid){

}

// -- 43
void exhtry(Context *cptr){}
void exhthrow(
    Context *cptr, void *except, void *data, char *filename, int lineno
){}
int exhcatch(
    Context *cptr, ObjectRef object){}
int exhfinally(Context *cptr){}
void exhreturn(Context *cptr){}
int exhcheck_begin(
    Context *cptr, int *checked, char *filename, int lineno
){}

int exhcheck(
    Context *cptr, int *checked, ObjectRef object, char *filename, int lineno
){}

