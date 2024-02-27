#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <threads.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "queue.h"
void initQueue(void);
void destroyQueue(void);
void enqueue(void*);
void* dequeue(void);
bool tryDequeue(void**);
size_t size(void);
size_t waiting(void);
size_t visited(void);
//gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 -pthread -c queue.c

typedef struct Node {
    struct Node *next;
    void *val;
} Node;


typedef struct thread{
    struct thread *next;
    void *val;
    cnd_t cond;
} thread;


Node *head = NULL; //head of items in the queue
Node *tail = NULL;//tail of items in the queue
thread *first = NULL;//head of threads in the queue
thread *last = NULL;//tail of threads in the queue
atomic_size_t siz; //amount of items in the queue
atomic_size_t wai; //amount of threads waiting for the queue to fill
atomic_size_t vis; //amount of items that have passed inside the queue
atomic_size_t sle; //amount of sleeping threads
mtx_t lock;


void initQueue(void){
    mtx_lock(&lock);
    head = NULL;
    tail = NULL;
    first = NULL;
    last = NULL;
    siz = 0;
    wai = 0;
    vis = 0;
    sle = 0;
}

void destroyQueue(void){
    //Free Nodes (items)
    Node *curr = head;
    Node *tmp = NULL;
    while (curr != NULL){
        tmp = curr;
        curr = curr-> next;
        free(tmp);
    }
    head = NULL;
    tail = NULL;
    //Free threads
    thread *t = first;
    thread *t_tmp = NULL;
    while (t != NULL){
        t_tmp = t;
        t = t->next;
        free(t_tmp);
    }
    first = NULL;
    last = NULL;
    mtx_destroy(&lock);
}

void enqueue(void* p){
    mtx_lock(&lock);
    if (sle == 0){
        //that means that there is no sleeping thread
        Node *n = (Node*) malloc(sizeof(Node));
        n->val = p;
        n->next = NULL;
        if (head == NULL){
            head = n;
            tail = head;
        }
        else{
            tail->next = n;
            tail = n;
        }
    }
    else {
        //that means that a there's a thread waiting
        thread *curr;
        curr = first;
        if (sle == 1){
            first = NULL;
            last = NULL;
        }
        else{
            first = first->next;
        }
        curr->val = p;
        curr->next = NULL;
        cnd_signal(&curr->cond);
        sle -- ;
    }
    siz ++;
    mtx_unlock(&lock);
}

void* dequeue(void){
    mtx_lock(&lock);
    void * returned_void;
    Node* curr;
    thread *thr;
    cnd_t cnd;
    curr = head;
    if (curr != NULL){
        //for this thread there's a node than can be freed
        returned_void = curr->val;
        if (head ->next == NULL){
            tail = NULL;
        }
        head = curr->next;
        free(curr);
    }
    else {
        //Need to create new thread because there's no elements in the queue
        thr = (thread*) malloc(sizeof(thread));
        //Initalize condition
        cnd_init(&cnd);
        //update the current thread
        thr->cond = cnd;
        thr->next = NULL;
        thr->val = NULL;
        if (sle == 0){
            //queue of threads is empty
            first = thr;
        }
        else {
            last->next=thr;
        }
        last= thr;
        sle ++;
        wai ++;
        cnd_wait(&thr->cond,&lock);//the thread waits
        returned_void = thr->val;
        cnd_destroy(&thr->cond);
        free(thr);
        wai --;
    }
    siz -- ;
    vis --;
    mtx_unlock(&lock);
    return returned_void;
}

bool tryDequeue(void** p){
    mtx_lock(&lock);
    Node *curr;
    if (head == NULL){
        // There is no item so the thread can do nothing
        mtx_unlock(&lock);
        return false;
    }
    else {
        p = head->val;
        curr = head;
        if (head->next == NULL){
            tail = NULL;
        }
        head = head->next;
        free(curr);
        siz --;
        vis ++;
    }
    mtx_unlock(&lock);
    return true;
}

size_t size(void){
    return q_size;
}

size_t waiting(void){
    return q_waiting;
}

size_t visited(void){
    return q_visited;
}
