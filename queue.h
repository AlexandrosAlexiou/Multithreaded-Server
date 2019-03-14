//
// Created by Alex Alexiou on 2019-03-14.
//

#ifndef MULTITHREADED_SERVER_QUEUE_H
#define MULTITHREADED_SERVER_QUEUE_H

#endif //MULTITHREADED_SERVER_QUEUE_H

#include<stdlib.h>
#include <time.h>
/*
 * This is the main queue structure
 */

typedef struct qelement
{
    int newfd;
    // time_t begin;
}qelement;

typedef struct queue
{
    int capacity;
    int size;
    int front;
    int rear;
    qelement *elements;
}queue;

/*
 * This is the type of variable that will be stored in the queue
 */

/*
 * This method creates a queue with maxElement number of elements and returns it
 */
queue* createQueue(int maxElements)
{
    /*Create a Queue*/
    queue *Q;
    Q = (queue *)malloc(sizeof(queue));

    /* Initialize it's properties */
    Q->elements = (qelement *)malloc(sizeof(qelement)*maxElements);
    Q->size = 0;
    Q->capacity = maxElements;
    Q->front = 0;
    Q->rear = -1;

    /*Return the pointer*/
    return Q;
}

/*
 * This method returns 1 if empty, and 0 if not empty
 */
int empty(queue *Q)
{
    if(Q->size == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*This method pops an element off the queue*/
void pop(queue *Q)
{
    if(Q->size != 0)
    {
        Q->size--;
        Q->front++;

        if(Q->front == Q->capacity)
        {
            Q->front = 0;
        }
    }
}

/*Pushes a new element onto the queue*/
void push(queue *Q, qelement element)
{
    if(Q->size != Q->capacity)
    {
        Q->size++;
        Q->rear = Q->rear + 1;

        if(Q->rear == Q->capacity)
        {
            Q->rear = 0;
        }

        Q->elements[Q->rear] = element;
    }
}
/*Returns the top most element of the queue*/
qelement peek(queue *Q)
{
    /* Return the element which is at the front*/
    return Q->elements[Q->front];
}