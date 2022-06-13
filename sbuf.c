#include <stdio.h>
#include "csapp.h"
// define productor-consumer pattern data structure
typedef struct {
    int *buf;       // buffer array
    int n;          // maximum number of slots
    int front;      // buf[(front+1)%n] is the first item
    int rear;       // buf[rear % n] is the last item
    sem_t mutex;    // protects accesses to bufs
    sem_t slots;    // counts available slots
    sem_t items;    // counts available items
} sbuf_t;

/* create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

/* clean up buffer sp*/ 
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

/* insert item onto the rear of shared buffer sp*/
void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
} 

int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

