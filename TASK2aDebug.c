//
// Created by Yizirui Fang on 2020/12/11.
//

#include <pthread.h>
#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>


typedef struct process processChara;
typedef struct element processNode;

typedef struct {
    processNode **queueHead;
    processNode **queueTail;
    int index;
} threadArgs;

#define processData(processNode) ((processChara *) processNode->pData)

sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;
int *inBufferThreadCounter = 0;
double totalResponseTime = 0.0;
double totalTurnaroundTime = 0.0;
int processIndex = 0;

void swap(struct element *a, struct element *b);

void sortProcessesBySJF(processNode *start);

void *consumer(void *args);

void *producer(void *args) ;

int main(int argc, char const *argv[]) {
    sem_init(&emptyBufferNum, 0, MAX_BUFFER_SIZE);
    sem_init(&accessSync, 0, 1);
    sem_init(&fullBufferNum, 0, 0);
    processNode *workingQueueHead = NULL;
    processNode *workingQueueTail = NULL;

    pthread_t producerThreads[NUMBER_OF_PRODUCERS];
    pthread_t consumerThreads[NUMBER_OF_CONSUMERS];
    for (int producerIndex = 0; producerIndex < NUMBER_OF_PRODUCERS; ++producerIndex) {
        //TODO: could declare with malloc
        threadArgs *currentProducerArg;
        currentProducerArg->queueHead = &workingQueueHead;
        currentProducerArg->queueTail = &workingQueueTail;
        currentProducerArg->index = producerIndex;
        pthread_create(&producerThreads[producerIndex], NULL, (void *(*)(void *)) producer,
                       (void *) currentProducerArg);
    }
    for (int consumerIndex = 0; consumerIndex < NUMBER_OF_CONSUMERS; ++consumerIndex) {
        //TODO: could declare with malloc
        threadArgs *currentProducerArg;
        currentProducerArg->queueHead = &workingQueueHead;
        currentProducerArg->queueTail = &workingQueueTail;
        currentProducerArg->index = consumerIndex;
        pthread_create(&consumerThreads[consumerIndex], NULL, (void *(*)(void *)) consumer,
                       (void *) currentProducerArg);
    }

//    for (processIndex = 0; processIndex < MAX_NUMBER_OF_JOBS; processIndex++) {
//
//    }

    return 0;
}

/**
 * terminates when MAX_NUMBER_OF_JOBS have been consumed
 */
void *consumer(void *args) {
    threadArgs *currentArgs = (threadArgs *) args;
    while (1) {
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);
        struct timeval processStartTime;
        struct timeval processEndTime;
        processChara *runningProcess = removeFirst(currentArgs->queueHead, currentArgs->queueTail);
        runNonPreemptiveJob(runningProcess, &processStartTime, &processEndTime);
        inBufferThreadCounter--;
        sem_post(&accessSync);
        sem_post(&emptyBufferNum);
    }
}

void *producer(void *args) {
    threadArgs *currentArgs = (threadArgs *) args;
    while (1) {
        sem_wait(&emptyBufferNum);
        sem_wait(&accessSync);
        processChara *newProcess = generateProcess();
        addLast(newProcess, currentArgs->queueHead, currentArgs->queueTail);
        sortProcessesBySJF(*(currentArgs->queueHead));
        inBufferThreadCounter++;
        processIndex++;
        printf("Producer = %d, Items Produced = %d, New Process Id = %d, Burst Time = %d", currentArgs->index, processIndex,
               newProcess->iProcessId, newProcess->iInitialBurstTime);
        sem_post(&accessSync);
        sem_post(&fullBufferNum);

    }
}

/* Bubble sort the given linked list */
void sortProcessesBySJF(processNode *start) {
    int swapped;
    processNode *minNode;
    processNode *lptr = NULL;

    /* Checking for emptyBufferNum list */
    if (start == NULL)
        return;

    do {
        swapped = 0;
        minNode = start;

        while (minNode->pNext != lptr) {
            if (((processChara *) (minNode->pData))->iRemainingBurstTime >
                ((processChara *) (minNode->pNext->pData))->iRemainingBurstTime) {
                swap(minNode, minNode->pNext);
                swapped = 1;
            }
            minNode = minNode->pNext;
        }
        lptr = minNode;
    } while (swapped);
}


/* function to swap data of two nodes a and b*/
void swap(struct element *a, struct element *b) {
    processNode *tempNode = a->pData;
    a->pData = b->pData;
    b->pData = tempNode;
}