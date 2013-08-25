#define GC_DEBUG 1
#include "gc.h"
#include <stdio.h>

long object_count = 0;
typedef struct _MyObject {
    long *obj;
    struct _MyObject *next, *prev;
} MyObject;

void MyObject_init(MyObject *o)
{
    object_count++;
    o->obj = GC_MALLOC_ATOMIC(128*128);
    memset(o->obj, 0, 128*128);
}

extern void *GC_mark_stack_limit;

void MyObject_finalize(MyObject *o)
{
    object_count--;
    printf("finalizer! %p %p\n", GC_mark_stack_limit, &GC_mark_stack_limit);
}

long list_count = 0;
typedef struct _MyList {
    MyObject *head, *tail;
} MyList;

void MyList_init(MyList *l)
{
    l->head = NULL;
    l->tail = NULL;
}

void MyList_add(MyList *l, MyObject *o)
{
    if (l->head == NULL) {
        l->head = o;
        l->tail = o;
    } else {
        o->prev = l->head;
        l->head->next = o;
        l->head = o;
    }
    list_count++;
}

void MyList_removeTail(MyList *l)
{
    if (l->tail != NULL) {
        if (l->tail->next == NULL) {
            l->tail = NULL;
            l->head = NULL;
        } else {
            MyObject *o = l->tail;
            l->tail = l->tail->next;
            l->tail->prev = NULL;
#if 0
            o->next = NULL;
#endif
        }
        list_count--;
    }
}

void MyList_removeHead(MyList *l)
{
    if (l->head != NULL) {
        if (l->head->prev == NULL) {
            l->tail = NULL;
            l->head = NULL;
        } else {
            l->head = l->head->prev;
            l->head->next = NULL;
        }
        list_count--;
    }
}

void show(MyList *l)
{
    long tm = GC_get_total_bytes();
    long fm = GC_get_free_bytes();
    long ratio = (100*fm)/tm;
    long to = object_count;
    long ao = list_count;
    printf("%ld %ld %ld\n", ratio, to, ao);
}

void finalizer_run(void *o, void *p)
{
    MyObject_finalize((MyObject *) o);
}

void myTest()
{
    MyList *objList;
    int m;
    int initSteps = 16;
    MyObject *o;

    objList = (MyList *) GC_MALLOC(sizeof(struct _MyList));
    MyList_init(objList);
    for (m=0; m<initSteps; m++) {
        o = (MyObject *) GC_MALLOC(sizeof(struct _MyObject));
        /*GC_register_finalizer_no_order(o, finalizer_run, NULL, NULL, NULL);*/
        MyObject_init(o);
        MyList_add(objList, o);
    }
    for (;;) {
        o = (MyObject *) GC_MALLOC(sizeof(struct _MyObject));
        /*GC_register_finalizer_no_order(o, finalizer_run, NULL, NULL, NULL);*/
        MyObject_init(o);
        MyList_add(objList, o);

        MyList_removeTail(objList);
        show(objList);
        GC_generate_random_backtrace();
    }
}

void finalizer_notify()
{
    GC_invoke_finalizers();
}

int main (void)
{
  GC_set_all_interior_pointers(0);
  GC_set_max_heap_size(16000000);
  /*GC_set_java_finalization(1);*/
  GC_INIT();

  myTest();

  return 0;
}
