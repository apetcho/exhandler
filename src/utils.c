
#include "exhandler.h"

#define EXH_STACK_DEFAULT_SIZE      32
#define EXH_STACK_SIZE_INCREMENT    32

/********************************************************************/
/*               Stack Data Structure Implementation                */
/********************************************************************/
// -- 01
Stack* stack_new(void){
    Stack *stack;
    stack = malloc(sizeof(*stack));
    stack->data = malloc(EXH_STACK_DEFAULT_SIZE * sizeof(void*));
    stack->size = EXH_STACK_DEFAULT_SIZE;
    stack->index = 0;
    return stack;
}

// -- 02
void stack_delete(Stack *stack){
    assert(stack != NULL);
    free(stack->data);
    stack->data = NULL;
    free(stack);
    stack = NULL;
}

// -- 03
void stack_delete_with_data(Stack *stack){
    assert(stack != NULL);
    while(stack->index > 0){
        free(stack->data[stack->index--]);
    }
    free(stack->data);
    stack->data = NULL;
    free(stack);
    stack = NULL;
}

// -- 04
void stack_push(Stack *stack, void *object){
    assert(stack != NULL && object != NULL);
    if(stack->index == stack->size){
        stack->size += EXH_STACK_SIZE_INCREMENT;
        stack->data = realloc(stack->data, stack->size*sizeof(void*));
    }
    stack->data[stack->index++] = object;
}


void* stack_pop(Stack *stack){}
void* stack_peek(Stack *stack, int n){}
int stack_len(Stack *stack){}

