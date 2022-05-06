
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
    stack->pointer = 0;
    return stack;
}

void stack_delete(Stack *stack){}
void stack_delete_with_data(Stack *stack){}
void stack_push(Stack *stack, void *object){}
void* stack_pop(Stack *stack){}
void* stack_peek(Stack *stack, int n){}
int stack_len(Stack *stack){}

