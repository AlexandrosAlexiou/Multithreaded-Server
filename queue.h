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
}qelement;
/*
 * This is the main queue structure
 */
typedef struct{
    int capacity;
    int size;
    int front;
    int rear;
    qelement *elements;
}queue;


/*
 * This method creates a queue with maxElement number of elements and returns it
 */
queue* createQueue(int maxElements){
    queue *Q;
    Q = (queue *)malloc(sizeof(queue));

    Q->elements = (qelement *)malloc(sizeof(qelement)*maxElements);
    Q->size = 0;
    Q->capacity = maxElements;
    Q->front = 0;
    Q->rear = -1;
    /*Return the pointer*/
    return Q;
}

/*
 * This method returns 1 if queue is empty, and 0 if not empty
 */
int empty(queue *Q){
    return (Q->size == 0);
}

/*This method pops an element off the queue*/
void pop(queue *Q){
    if(Q->size != 0){
        Q->size--;
        Q->front++;

        if(Q->front == Q->capacity){
            Q->front = 0;
        }
    }
}

/*Pushes a new element onto the queue*/
void push(queue *Q, qelement element){
    if(Q->size != Q->capacity){
        Q->size++;
        Q->rear = Q->rear + 1;

        if(Q->rear == Q->capacity){
            Q->rear = 0;
        }

        Q->elements[Q->rear] = element;
    }
}
/* Returns the element which is at the front of the queue*/
qelement peek(queue *Q){
    return Q->elements[Q->front];
}