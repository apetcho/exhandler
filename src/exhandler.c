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

// -----------------------------------------------------------------
// exhthrow_signal() :: 'throw' exception caused by signal
// -----------------------------------------------------------------
static void exhthrow_signal(int num){
    ObjectRef objref;
    exhprint_debug(cptr, "exhthrow_signal");
    switch(num){
    case SIGABRT:
        objref = AbnormalTerminationError;
        break;
    case SIGFPE:
        objref = ArithmethicError;
        break;
    case SIGILL:
        objref = IllegalInstructionError;
        break;
    case SIGSEGV:
        objref = SegmentationError;
        break;
#ifdef SIGBUS
    case SIGBUS:
        objref = BusError;
        break;
#endif
    }

    signal(num, exhthrow_signal);
    objref->signum = num;
    exhthrow(NULL, objref, NULL, "?", 0);
}

// -----------------------------------------------------------------
// exhinstall_handlers() :: Install signal/trap handler if needed
// -----------------------------------------------------------------
static int exhinstall_handlers(Context *context){
    int stored = 0;
    if(context->stack == NULL){
        EXHANDLER_THREAD_MUTEX_FUNC(1);
        if(EXHANDLER_MULTI_THREADING && EXHANDLER_SHARE && numThreadsTry++ ==0){
            shared_abort_handlerfn = signal(SIGABRT, exhthrow_signal);
            shared_fpe_handlerfn = signal(SIGFPE, exhthrow_signal);
            shared_ill_handlerfn = signal(SIGILL, exhthrow_signal);
            shared_segv_handlerfn = signal(SIGSEGV, exhthrow_signal);
#ifdef SIGBUS
            shared_bus_handlerfn = signal(SIGBUS, exhthrow_signal);
#endif
            stored = 1;
        }else if(!EXHANDLER_MULTI_THREADING || !EXHANDLER_SHARE){
            context->aborthandler = signal(SIGABRT, exhthrow_signal);
            context->fpehandler = signal(SIGFPE, exhthrow_signal);
            context->illhandler = signal(SIGILL, exhthrow_signal);
            context->segvhandler = signal(SIGSEGV, exhthrow_signal);
#ifdef SIGBUS
            context->bushandler = signal(SIGSEGV, exhthrow_signal);
#endif
            stored = 1;
        }
        EXHANDLER_THREAD_MUTEX_FUNC(0);
    }

    return stored;
}

// -----------------------------------------------------------------
// exhrestore_handlers() :: restore signal/trap handlers if needed
// -----------------------------------------------------------------
static int exhresore_handlers(Context *context){
    int restored = 0;
    EXHANDLER_THREAD_MUTEX_FUNC(1);
    if(EXHANDLER_MULTI_THREADING && EXHANDLER_SHARE && --numThreadsTry == 0){
        signal(SIGABRT, shared_abort_handlerfn);
        signal(SIGFPE, shared_fpe_handlerfn);
        signal(SIGILL, shared_ill_handlerfn);
        signal(SIGSEGV, shared_segv_handlerfn);
#ifdef SIGBUS
        signal(SIGBUS, shared_bus_handlerfn);
#endif
        restored = 1;
    }else if(!EXHANDLER_MULTI_THREADING || !EXHANDLER_SHARE){
        signal(SIGABRT, context->aborthandler);
        signal(SIGFPE, context->fpehandler);
        signal(SIGILL, context->illhandler);
        signal(SIGSEGV, context->segvhandler);
#ifdef SIGBUS
        signal(SIGBUS, context->bushandler);
#endif
        restored = 1;
    }
    EXHANDLER_THREAD_MUTEX_FUNC(0);

    return restored;
}

// -- 42
void exhthread_cleanup(int tid){
#if EXHANDLER_MULTI_THREADING
    exh_validate(tid != EXHANDLER_ID_FUNC(), EXH_NOTHING);
    if(tid == -1){
        tid = EXHANDLER_THREAD_ID_FUNC();
    }
    EXHANDLER_THREAD_MUTEX_FUNC(1);
    if(contextDict != NULL){
        Context *context;
        context = dict_get(contextDict, tid);
        if(context != NULL){
            exhresore_handlers(context);
            if(context->except->checklist != NULL){
                list_delete_with_data(context->except->checklist);
            }
            free(dict_remove(contextDict, tid));
        }
    }
    EXHANDLER_THREAD_MUTEX_FUNC(0);
#endif
}

// -- 43
void exhtry(Context *context, char *filename, int lineno){
#if EXHANDLER_MULTI_THREADING
    EXHANDLER_THREAD_MUTEX_FUNC(1);
    if(contextDict == NULL){
        contextDict = dict_new();
    }
    EXHANDLER_THREAD_MUTEX_FUNC(0);
#endif
    int first;
    if(first = (context == NULL)){ context = exhget_context(NULL);}
    if(context == NULL){ context = exhnew_context(); }

    exhinstall_handlers(context);
    stack_push(
        context->stack, context->except=calloc(1, sizeof(ExceptionType))
    );
    context->except->first = first;
    context->except->tryfile = filename;
    context->except->trylineno = lineno;

    exhprint_debug(context, "exhtry");
}

// -- 44
void exhthrow(
    Context *context, void *exceptObj, void *data, char *filename, int lineno
){
    exhprint_debug(context, "exhthrow");
    if(context == NULL){
        context = exhget_context(NULL);
    }

    if(context==NULL || context->stack==NULL || stack_len(context->stack)==0){
        fprintf(
            stderr, "%s lost: file \"%s\", line %d.\n",
            ((ObjectRef)exceptObj)->name, filename, lineno
        );
        return;
    }
    if(((ObjectRef)exceptObj)->norethrow){
        context->except->class = (ObjectRef)exceptObj;
        context->except->data = data;
        context->except->filename = filename;
        context->except->lineno = lineno;
        context->except->get_description = exhget_description;
        context->except->get_data = exhget_data;
        context->except->print_stacktrace = exhprint_stacktrace;
    }
    context->except->state = PENDING_STATE;
    switch(context->except->scope){
    case TRY_SCOPE:
        exhprint_debug(context, "longjmp(throwbuf)");
        EXH_LONGJMP(context->except->throwbuf, 1);
    case CATCH_SCOPE:
        exhprint_debug(context, "longjmp(finalbuf)");
        EXH_LONGJMP(context->except->finalbuf, 1);
    case FINALLY_SCOPE:
        exhprint_debug(context, "longjmp(finalbuf)");
        EXH_LONGJMP(context->except->finalbuf, 1);
    }
}

// --
static int exhis_derived(ObjectRef objref, ObjectRef base){
    while(objref->parent != NULL && objref != base){
        objref = objref->parent;
    }

    return objref == base;
}

// -- 45
int exhcatch(Context *context, ObjectRef object){
    exhprint_debug(context, "exhcatch");
    if(context == NULL){
        context = exhget_context(NULL);
    }
    if(context->except->state == PENDING_STATE &&
        exhis_derived(context->except->class, object)
    ){
        context->except->state = CAUGHT_STATE;
    }

    return context->except->state == CAUGHT_STATE;
}

// -- 46
int exhfinally(Context *context){
    ExceptionType *except;
    ExceptionType self;

    if(context == NULL){ context = exhget_context(NULL); }

    self = *(except = stack_pop(context->stack));
    free(except);
    context->except=stack_len(context->stack) ? stack_peek(context->stack) : 0;
    if(stack_len(context->stack) == 0){
        int restored = exhresore_handlers(context);
        if(self.state = PENDING_STATE){
            if(self.class == FailedAssertionError){
                exhhandle_assertion(
                    context, EXH_ABORT, self.data, self.filename, self.lineno
                );
            }
            else if(exhis_derived(self.class, RuntimeError) && restored){
                stack_delete(context->stack);
                if(EXHANDLER_MULTI_THREADING){
                    EXHANDLER_THREAD_MUTEX_FUNC(1);
                    free(dict_remove(contextDict, EXHANDLER_THREAD_ID_FUNC()));
                    EXHANDLER_THREAD_MUTEX_FUNC(0);
                }else{
                    context->stack = NULL;
                }
                raise(self.class->signum);
            }else if(self.class == ReturnEvent){
                stack_delete(context->stack);
                if(EXHANDLER_MULTI_THREADING){
                    EXHANDLER_THREAD_MUTEX_FUNC(1);
                    free(dict_remove(contextDict, EXHANDLER_THREAD_ID_FUNC()));
                    EXHANDLER_THREAD_MUTEX_FUNC(0);
                }else{
                    context->stack = NULL;
                }
                EXH_LONGJMP(*(EXH_JMP_BUF*)self.data, 1);
            }else{
                fprintf(
                    stderr, "%s lost: file \"%s\", line %d.\n",
                    self.class->name, self.filename, self.lineno
                );
            }
        }
        stack_delete(context->stack);
        if(EXHANDLER_MULTI_THREADING){
            EXHANDLER_THREAD_MUTEX_FUNC(1);
            free(dict_remove(contextDict, EXHANDLER_THREAD_ID_FUNC()));
            EXHANDLER_THREAD_MUTEX_FUNC(0);
        }else{ context->stack = NULL; }
    }else{
        if(self.state == PENDING_STATE){
            if(self.class == ReturnEvent && self.first){
                EXH_LONGJMP(*(EXH_JMP_BUF*)self.data, 1);
            }else{
                exhthrow(
                    context, self.class, self.data, self.filename, self.lineno
                );
            }
        }
    }

    return 0;
}

// -- 47
void exhreturn(Context *context){
    exhprint_debug(context, "exhreturn");
    if(context == NULL){
        context = exhget_context(NULL);
    }
    context->except->class = ReturnEvent;
    context->except->state = PENDING_STATE;
    exhprint_debug(context, "longjmp(finalbuf)");

    EXH_LONGJMP(context->except->finalbuf, 1);
}

//  -- 48
int exhcheck_begin(
    Context *context, int *checked, char *filename, int lineno
){
    exhprint_debug(context, "exhcheck_begin");

    if(context->except->checklist == NULL && !*checked){
        context->except->checklist = list_new();
    }else{
        if(!*checked){
            if(list_len(context->except->checklist) == 0){
                fprintf(
                    stderr, "Warning: No catch clause(s): file \"%s\", "
                    "line %d.\n", filename, lineno
                );
            }
            list_delete_with_data(context->except->checklist);
            context->except->checklist = NULL;
            *checked = 1;
        }
    }

    return *checked;
}

// -- 49
int exhcheck(
    Context *context, int *checked, ObjectRef object, char *filename, int lineno
){
    typedef struct Check{
        ObjectRef objref;
        int lineno;
    } Check;
    exhprint_debug(context, "exhcheck");
    if(!*checked){
        Check *check;
        check = list_get_head(context->except->checklist);
        while(check != NULL){
            if(object == check->objref){
                fprintf(
                    stderr,
                    "Duplicate catch/%s): file \"%s\", line %d; "
                    "already caught at line %d.\n",
                    object->name, filename, lineno, check->lineno
                );
                break;
            }
            if(exhis_derived(object, check->objref)){
                fprintf(
                    stderr, "Superfluous catch(%s): file \"%s\", line %d; "
                    "already caught by %s at line %d.\n",
                    object->name, lineno, check->objref->name, check->lineno
                );
                break;
            }
            check = list_get_next(context->except->checklist);
        }

        if(check == NULL){
            check = malloc(sizeof(check));
            check->objref = object;
            check->lineno = lineno;
            list_append(context->except->checklist, check);
        }
    }

    return *checked;
}

