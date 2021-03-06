
#include "exhandler.h"

#define EXH_STACK_DEFAULT_SIZE      32
#define EXH_STACK_SIZE_INCREMENT    32
#define EXH_HASH_BUCKET_SIZE        512

/********************************************************************/
/*               Stack Data Structure Implementation                */
/********************************************************************/
// -- 01
Stack* stack_new(void){
    Stack *stack;
    stack = malloc(sizeof(*stack));
    stack->data = malloc(EXH_STACK_DEFAULT_SIZE * sizeof(void*));
    stack->size = EXH_STACK_DEFAULT_SIZE;
    stack->len = 0;
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
    while(stack->len > 0){
        free(stack->data[stack->len--]);
    }
    free(stack->data);
    stack->data = NULL;
    free(stack);
    stack = NULL;
}

// -- 04
void stack_push(Stack *stack, void *object){
    assert(stack != NULL && object != NULL);
    if(stack->len == stack->size){
        stack->size += EXH_STACK_SIZE_INCREMENT;
        stack->data = realloc(stack->data, stack->size*sizeof(void*));
    }
    stack->data[stack->len++] = object;
}

// -- 05
void* stack_pop(Stack *stack){
    assert(stack != NULL);
    exh_validate(stack->len > 0, NULL);
    return stack->data[--stack->len];
}

// -- 06
void* stack_peek(Stack *stack, int n){
    assert(stack != NULL);
    exh_validate(stack->len > 0, NULL);
    exh_validate(n > 0 && n <= stack->len, NULL);
    return stack->data[stack->len - n];
}

// -- 07
int stack_len(Stack *stack){
    assert(stack != NULL);
    return stack->len;
}

/********************************************************************/
/*                List Data Structure Implementation                */
/********************************************************************/

// -- 08
List* list_new(void){
    List *list;
    list = malloc(sizeof(*list));
    list->head = malloc(sizeof(ListNode));
    list->head->next = list->head;
    list->head->prev = list->head;
    list->pointer = NULL;
    list->len = 0;
    return list;
}

// -- 09
void list_delete(List *list){
    assert(list != NULL);
    ListNode *cursor;
    cursor = list->head->next;
    while(cursor != list->head){
        List *node;
        node = cursor->next;
        free(cursor);
        cursor = node;
    }
    free(list->head);
    list->head = NULL;
    free(list);
    list = NULL;
}

// -- 10
void list_delete_with_data(List *list){
    assert(list != NULL);
    ListNode *cursor;
    cursor = list->head->next;
    while(cursor != list->head){
        ListNode *node;
        node = cursor->next;
        free(cursor->data);
        free(cursor);
        cursor = node;
    }
    free(list->head);
    list->head = NULL;
    free(list);
    list = NULL;
}

// -- 11
void list_prepend(List *list, void *data){
    assert(list != NULL);
    ListNode *node;
    node = malloc(sizeof(ListNode));
    node->data = data;
    node->next->prev = node;
    list->head->next->prev = node;
    list->head->next->prev = list->head;
    node->prev = list->head;
    list->pointer = node;
    list->len++;
}

// -- 12
void list_append(List *list, void *data){
    assert(list != NULL);
    ListNode *node;
    node = malloc(sizeof(*node));

    node->data = data;
    node->prev->next = node;
    list->head->prev->prev = node;
    list->head->prev->next = list->head;
    node->next = list->head;
    list->pointer = node;
    list->len++;
}

// -- 13
void list_insert_before(List *list, void *data){
    assert(list != NULL);
    exh_validate(list->pointer != NULL, EXH_NOTHING);
    ListNode *node;
    node = malloc(sizeof(*node));
    node->data = data;
    node->prev->next = node;
    list->pointer->prev->next = node;
    list->pointer->next = list->pointer;
    node->next = list->pointer;
    list->pointer = node;
    list->len++;
}

// -- 14
void list_insert_after(List *list, void *data){
    assert(list != NULL);
    exh_validate(list->pointer != NULL, EXH_NOTHING);
    ListNode *node;
    node = malloc(sizeof(*node));
    node->data = data;
    node->next->prev = node;
    list->pointer->next->prev = node;
    list->pointer->next->prev = list->pointer;
    node->prev = list->pointer;
    list->pointer = node;
    list->len++;
}

// -- 15
void list_remove_head(List *list){
    assert(list != NULL);
    exh_validate(list->len > 0, NULL);
    ListNode *node;
    void *data;

    node = list->head->next;
    data = node->data;
    list->head->next->prev = list->head;
    free(node);
    list->pointer = NULL;
    list->len--;

    return data;
}

// -- 16
void* list_remove_tail(List *list){
    assert(list != NULL);
    exh_validate(list->len > 0, NULL);
    ListNode *node;
    void *data;

    node = list->head->prev;
    data = node->data;
    list->head->prev->next = list->head;
    node->prev->next = list->head;
    free(node);
    list->pointer = NULL;
    list->len--;

    return data;
}

// -- 17
void list_remove(List *list, void *data){
    assert(list != NULL);
    exh_validate(list->len > 0, NULL);
    ListNode *node;

    node = list->head->next;
    while(node->data != list->head && node->data != data){
        node = node->next;
    }
    exh_validate(node->data == data, NULL);
    node->next->prev = node->prev;
    node->prev->next = node->next;
    free(node);
    list->pointer = NULL;
    list->len--;

    return data;
}

// -- 18
void list_remove_last(List *list){
    assert(list != NULL);
    exh_validate(list->pointer != NULL, NULL);
    ListNode *node;
    void *data;

    data = list->pointer->data;
    node = list->pointer->next;
    list->pointer->next->prev = list->pointer->prev;
    list->pointer->prev->next = list->pointer->next;
    free(list->pointer);

    list->pointer = node;
    list->len--;

    return data;
}

// -- 19
void* list_get_head(List *list){
    assert(list != NULL);
    if(list->len == 0){ return NULL;}
    else{
        list->pointer = list->head->next;
        return list->pointer->data;
    }
    return NULL;
}

// -- 20
void* list_get_tail(List *list){
    assert(list != NULL);
    if(list->len == 0){ return NULL; }
    else{
        list->pointer = list->head->prev;
        return list->pointer->data;
    }
    return NULL;
}

// -- 21
void* list_get_last(List *list){
    assert(list != NULL);
    if(list->pointer == NULL){ return NULL; }
    return list->pointer->data;
}

// -- 22
void* list_get_next(List *list){
    assert(list != NULL);
    exh_validate(list->pointer != NULL, NULL);
    if((list->pointer = list->pointer->next) == list->head){
        list->pointer = NULL;
        return NULL;
    }
    return list->pointer->data;
}

// -- 23
void* list_get_prev(List *list){
    assert(list != NULL);
    exh_validate(list->pointer != NULL, NULL);
    if((list->pointer = list->pointer->prev) == list->head){
        list->pointer = NULL;
        return NULL;
    }
    return list->pointer->data;
}

// -- 24
int list_len(List *list){
    assert(list != NULL);
    return list->len;
}

// -- 25
void* list_find(List *list, void *data){
    assert(list != NULL);
    ListNode *node;
    node = list->head->next;
    while(node != list->head && node->data != data){
        node = node->next;
    }

    if(node->data == data){
        list->pointer = node;
        return data;
    }

    return NULL;
}

// -- 26
List* list_split_before(List *list){
    assert(list != NULL);
    exh_validate(list->len > 0, NULL);
    exh_validate(list->pointer != NULL, NULL);
    List *retlist;
    ListNode *node;

    retlist = malloc(sizeof(List));
    retlist->head = malloc(sizeof(ListNode));
    retlist->head->data = NULL;
    retlist->len = 0;

    node = list->head->next;
    while(node != list->pointer){
        list->len--;
        retlist->len++;
        node = node->next;
    }
    if(list->len == 0){
        retlist->head->prev = retlist->head->next = retlist->head;
    }else{
        retlist->head->next = list->head->next;
        retlist->head->next->prev = retlist->head;
        retlist->head->prev = list->pointer->prev;
        retlist->head->prev->next = retlist->head;

        list->head->next = list->pointer;
        list->pointer->prev = list->head;
    }

    retlist->pointer = NULL;

    return retlist;
}

// -- 27
List* list_split_after(List *list){
    assert(list != NULL);
    exh_validate(list->len > 0, NULL);
    exh_validate(list->pointer != NULL, NULL);

    List *retlist;
    ListNode *node;

    retlist = malloc(sizeof(List));
    retlist->head = malloc(sizeof(ListNode));
    retlist->head->data = NULL;
    retlist->len = 0;

    node = list->pointer->next;
    while(node != list->head){
        list->len--;
        retlist->len++;
        node = node->next;
    }

    if(retlist->len == 0){
        retlist->head->prev = retlist->head->next = retlist->head;
    }else{
        retlist->head->next = list->pointer->next;
        retlist->head->next->prev = retlist->head;
        retlist->head->prev = list->head->prev;
        retlist->head->prev->next = retlist->head;

        list->pointer->next = list->head;
        list->head->prev = list->pointer;
    }

    retlist->pointer = NULL;
    retlist retlist;
}

// -- 28
List* list_merge(List *list1, List* list2){
    assert(list1 != NULL);
    assert(list2 != NULL);
    int opt = (((list2->len > 0) << 1)| (list1->len > 0));
    switch(opt){
    case 0: // list1 & list2 are empty
        break;
    case 1: // list2 is not empty but list1 is empty.
        break;
    case 2: // list1 is empty but list2 is not empty.
        list1->head->next = list2->head->next;
        list2->head->next->prev = list2->head;
        list2->head->prev = list2->head->prev;
        list2->head->prev->next = list1->head;
        break;
    case 3: // both list1 & list2 are not empty
        list2->head->prev->next = list1->head;
        list1->head->prev->next = list2->head->next;
        list2->head->next->prev = list1->head->prev;
        list1->head->prev = list2->head->prev;
        break;  
    }

    list1->pointer = NULL;
    list1->len += list2->len;
    free(list2->head);
    free(list2);

    return list1;
}

/********************************************************************/
/*              Dictionary Data Structure Implementation            */
/********************************************************************/

typedef struct HNode {
    void *data;
    int key;
} HNode;

// -- 29
Dict* dict_new(){
    Dict *dict;
    dict = malloc(sizeof(Dict));
    dict->len = 0;
    dict->bucket = malloc(EXH_HASH_BUCKET_SIZE * sizeof(List*));
    for(int i=0; i < EXH_HASH_BUCKET_SIZE; i++){
        dict->bucket[i] = list_new();
    }

    return dict;
}

// -- 30
void dict_delete(Dict *dict){
    assert(dict != NULL);
    for(int i=0; i < EXH_HASH_BUCKET_SIZE; i++){
        list_delete(dict->bucket[i]);
    }
    free(dict->bucket);
    free(dict);
}

// -- 31
void dict_delete_with_data(Dict *dict){
    assert(dict != NULL);
    for(int i=0; i < EXH_HASH_BUCKET_SIZE; i++){
        HNode *node;
        node = list_get_head(dict->bucket[i]);
        while(node != NULL){
            free(node->data);
            node = list_get_next(dict->bucket[i]);
        }
        list_delete_with_data(dict->bucket[i]);
    }
    free(dict->bucket);
    free(dict);
}

// ---
static int calculate_hash(Dict *dict, unsigned key){
    assert(dict != NULL);
    int value;
    value = (EXH_HASH_BUCKET_SIZE * ((key * 40503) & 65535)) >> 16;
    if(value >= EXH_HASH_BUCKET_SIZE || value < 0){
        fprintf(stderr, "Invalid hash value: %d\n", value);
    }

    return value;
}

// -- 32
void* dict_get(Dict *dict, int key){
    assert(dict != NULL);
    List *list;
    HNode *node;

    list = dict->bucket[calculate_hash(dict, key)];
    node = list_get_head(list);
    while(node != NULL){
        if(node->key == key){ return node->data; }
        node = list_get_next(list);
    }

    return NULL;
}

// -- 33
void dict_put(Dict *dict, int key, void *data){
    assert(dict != NULL);
    exh_validate(data != 0, EXH_NOTHING);
    HNode *node;
    node = malloc(sizeof(HNode));
    node->key = key;
    node->data = data;
    list_prepend(dict->bucket[calculate_hash(dict, key)], node);
    dict->len++;
}

// -- 34
void* dict_remove(Dict *dict, int key){
    assert(dict != NULL);
    List *list;
    HNode *node;
    void *data;
    list = dict->bucket[calculate_hash(dict, key)];
    node = list_get_head(list);
    while(node != NULL){
        if(node->key == key){
            list_remove_last(list);
            data = node->data;
            free(node);
            dict->len--;
            return data;
        }
        node = list_get_next(list);
    }

    return NULL;
}

// -- 35
int dict_len(Dict *dict){
    assert(dict != NULL);
    return dict->len;
}
