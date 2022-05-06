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
                "Exhandler internal error: thread attempts to unlock without "
                "holding lock\n"
            );
        }
    }
}
#endif

// -- 40
Scope exhget_scope(Context *cptr){}

// -- 41
Context* exhget_context(Context *cptr){}
void exhthread_cleanup(int tid){}
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

