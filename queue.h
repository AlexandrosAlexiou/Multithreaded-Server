//
// Created by Alexandros Alexiou on 2019-03-10.
//

#ifndef MULTITHREADED_SERVER_QUEUE_H
#define MULTITHREADED_SERVER_QUEUE_H

#endif //MULTITHREADED_SERVER_QUEUE_H

#include<stdlib.h>
#include <time.h>

/*
 * This is the type of variable that will be stored in the queue(request)
 */
typedef struct{
    int fd;
    struct timeval tv;
}qElement;
/*
 * This is the main queue structure
 */
typedef struct{
    int capacity;
    int size;
    int front;
    int rear;
    qElement *elements;
}Queue;


/*
 * Creates a queue with maxElement number of elements and returns the address
 */
Queue* createQueue(int maxElements){
    Queue *Q;
    Q = (Queue *)malloc(sizeof(Queue));

    Q->elements = (qElement *)malloc(sizeof(qElement)*maxElements);
    Q->size = 0;
    Q->capacity = maxElements;
    Q->front = 0;
    Q->rear = -1;
    /*Return the pointer*/
    return Q;
}

/*
 *  returns 1 if queue is empty, and 0 if not empty
 */
int empty(Queue *Q){
    return (Q->size == 0);
}

/*
 *  returns 1 if queue is full, and 0 if not full
 */
int full(Queue *Q){
    return (Q->size == Q->capacity);
}

/* Pops an element off the queue*/
void pop(Queue *Q){
    if(Q->size != 0){
        Q->size--;
        Q->front++;

        if(Q->front == Q->capacity){
            Q->front = 0;
        }
    }
}

/*Pushes a new element onto the queue*/
void push(Queue *Q, qElement element){
    if(Q->size != Q->capacity){
        Q->size++;
        Q->rear = Q->rear + 1;

        if(Q->rear == Q->capacity){
            Q->rear = 0;
        }

        Q->elements[Q->rear] = element;
    }
}
/* Returns the  front element of the queue*/
qElement peek(Queue *Q){
    return Q->elements[Q->front];
}