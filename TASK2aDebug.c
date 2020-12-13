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
} threadArg;
processNode *workingQueueHead = NULL;
processNode *workingQueueTail = NULL;

#define processData(processNode) ((processChara *) processNode->pData)

sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;
int *inBufferThreadCounter = 0;
double totalResponseTime = 0.0;
double totalTurnaroundTime = 0.0;
int processIndex = 0;

void swap(struct element *a, struct element *b);

void sortProcessesBySJF();

void *consumer(void *consumerIndex);

void *producer(void *producerIndex);

int main(int argc, char const *argv[]) {
    sem_init(&emptyBufferNum, 0, MAX_BUFFER_SIZE);
    sem_init(&accessSync, 0, 1);
    sem_init(&fullBufferNum, 0, 0);


    pthread_t producerThreads[NUMBER_OF_PRODUCERS];
    pthread_t consumerThreads[NUMBER_OF_CONSUMERS];

    int producerIndexArr[NUMBER_OF_PRODUCERS];
    int consumerIndexArr[NUMBER_OF_CONSUMERS];
    for (int producerIndex = 0; producerIndex < NUMBER_OF_PRODUCERS; ++producerIndex) {
        producerIndexArr[producerIndex] = producerIndex;
        pthread_create(&producerThreads[producerIndex], NULL, producer, (void *) &producerIndexArr[producerIndex]);
    }
    for (int consumerIndex = 0; consumerIndex < NUMBER_OF_CONSUMERS; ++consumerIndex) {
        consumerIndexArr[consumerIndex] = consumerIndex;
        pthread_create(&consumerThreads[consumerIndex], NULL, consumer,
                       (void *) &consumerIndexArr[consumerIndex]);
    }

    for (int producerIndex = 0; producerIndex < NUMBER_OF_PRODUCERS; ++producerIndex) {
        pthread_join(producerThreads[producerIndex], NULL);
    }
    for (int consumerIndex = 0; consumerIndex < NUMBER_OF_CONSUMERS; ++consumerIndex) {
        pthread_join(consumerThreads[consumerIndex], NULL);
    }
    printf("Average Response Time = %f\n", totalResponseTime / MAX_NUMBER_OF_JOBS);
    printf("Average Turn Around Time = %f\n", totalTurnaroundTime / MAX_NUMBER_OF_JOBS);

//    for (processIndex = 0; processIndex < MAX_NUMBER_OF_JOBS; processIndex++) {
//
//    }

    return 0;
}

/**
 * terminates when MAX_NUMBER_OF_JOBS have been consumed
 */
void *consumer(void *consumerIndex) {
    while (1) {
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);
        if ((workingQueueHead == NULL) && (processIndex == MAX_NUMBER_OF_JOBS)) {
            sem_post(&accessSync);
            sem_post(&emptyBufferNum);
            break;
        }
        struct timeval processStartTime;
        struct timeval processEndTime;
        processChara *runningProcess = (processChara *) removeFirst(&workingQueueHead, &workingQueueTail);
        runNonPreemptiveJob(runningProcess, &processStartTime, &processEndTime);
        long int currentResponse = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processStartTime);
        long int currentTurnaround = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processEndTime);
        totalResponseTime += currentResponse;
        totalTurnaroundTime += currentTurnaround;
        int currentConsumerIndex = *((int *) consumerIndex);
        printf("Consumer = %d, Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turnaround Time = %ld\n",
               currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPreviousBurstTime,
               runningProcess->iRemainingBurstTime, currentResponse, currentTurnaround);
        inBufferThreadCounter--;
        sem_post(&accessSync);
        sem_post(&emptyBufferNum);
    }
    return NULL;
}

void *producer(void *producerIndex) {
    while (1) {
        sem_wait(&emptyBufferNum);
        sem_wait(&accessSync);
        if (processIndex == MAX_NUMBER_OF_JOBS) {
            sem_post(&accessSync);
            sem_post(&emptyBufferNum);
            break;
        }
        int currentProducerIndex = *((int *) producerIndex);
        processChara *newProcess = generateProcess();
        addLast(newProcess, &workingQueueHead, &workingQueueTail);
        sortProcessesBySJF();
        inBufferThreadCounter++;
        processIndex++;
        printf("Producer = %d, Items Produced = %d, New Process Id = %d, Burst Time = %d\n", currentProducerIndex,
               processIndex,
               newProcess->iProcessId, newProcess->iInitialBurstTime);
        sem_post(&accessSync);
        sem_post(&fullBufferNum);
    }
    return NULL;
}

/* Bubble sort the given linked list */
void sortProcessesBySJF() {
    int swapped;
    processNode *minNode;
    processNode *lptr = NULL;

    /* Checking for emptyBufferNum list */
    if (workingQueueHead == NULL)
        return;

    do {
        swapped = 0;
        minNode = workingQueueHead;

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