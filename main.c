
#define MAX_STACK_SIZE 256
/* number of objects at which I start the first GC */
#define GC_TRIGGER_NUMBER 5

#include <assert.h>
#include <stdlib.h>

typedef enum {
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

typedef struct sObject {
    /* The next object in the list of all objects */
    struct sObject* next;

    unsigned char marked;
    /* int or pair */
    ObjectType type;

    union {
        /* int */
        int value;

        /* pair */
        struct {
            struct sObject* head;
            struct sObject* tail;
        };
    };
} Object;

/* store the vars that are currently in scope */
typedef struct VM {
    /* The total number of currently allocated objects */
    int numOfObjects;

    /* The number of objects required to trigger the garbage collector */
    int maxObjects;

    /* The first object in the list of all objects */
    Object* firstObject;

    Object* stack[MAX_STACK_SIZE];
    int stackSize;
} VM;

void gc(VM* vm);

VM* newVM() {
    VM* vm = malloc(sizeof(VM));
    vm->stackSize = 0;
    vm->firstObject = NULL;

    vm->numOfObjects = 0;
    vm->maxObjects = GC_TRIGGER_NUMBER;

    return vm;
}

void push(VM* vm, Object* value) {
    assert(vm->stackSize < MAX_STACK_SIZE, "Stack overflow");
    vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm) {
    assert(vm->stackSize > 0, "Nothing to pop");
    return vm->stack[--vm->stackSize];
}

Object* newObject(VM* vm, ObjectType type) {
    if (vm->numOfObjects == vm->maxObjects) gc(vm);

    Object* object = malloc(sizeof(Object));
    object->type = type;
    object->marked = 0;

    /* Insert it into the list of allocated objects */
    object->next = vm->firstObject;
    vm->firstObject = object;
    vm->numOfObjects++;

    return object;
}

void pushInt(VM* vm, int intValue) {
    Object* intObj = newObject(vm, OBJ_INT);
    intObj->value = intValue;
    push(vm, intObj);
}

Object* pushPair(VM* vm) {
    Object* pairObj = newObject(vm, OBJ_PAIR);
    pairObj->tail = pop(vm);
    pairObj->head = pop(vm);

    push(vm, pairObj);
    return pairObj;
}


/* MARKING PHASE */

/* mark every reachable object */
void mark(Object* object) {
    /* If the object already marked im done.
     * avoid recursive cycles*/
    if(object->marked) return;

    /* mark object as reachable */
    object->marked = 1;

    if (object->type == OBJ_PAIR) {
        mark(object->tail);
        mark(object->head);
    }
}

void markAll(VM* vm) {
    for (int i = 0; i < vm->stackSize; ++i) {
        mark(vm->stack[i]);
    }
}


/* SWEEPING PHASE */
void sweep(VM* vm) {
    Object** object = &vm->firstObject;
    while (*object) {

        if (!(*object)->marked) {
            /* object wasn't reached, so free it */
            Object* unreachable = *object;
            *object = (*object)->next;

            vm->numOfObjects--;
            free(unreachable);
        }
        else {
            /* make sure that all objects need to be re evaluated
             * to set reachability */
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }

}


/* GC */
void gc(VM* vm) {
    markAll(vm);
    sweep(vm);

    /* if the number of objects grows, so is the max heap */
    vm->maxObjects = vm->numOfObjects * 2;
}




