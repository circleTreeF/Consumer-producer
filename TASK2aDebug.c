//
// Created by Yizirui Fang on 2020/12/11.
/*
 * Dear Producers and Consumers:
 *
 * 你们要乖 爸爸给你们上锁了 你们不要偷偷自己开锁哦
 */

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

processNode *readyQueueHead = NULL;
processNode *readyQueueTail = NULL;

#define processData(processNode) ((processChara *) processNode->pData)

sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;
int *inBufferThreadCounter = 0;
double totalResponseTime = 0.0;
double totalTurnaroundTime = 0.0;
int processIndex = 0;
int consumedProcessNumber = 0;
int consumingProcessNumber = 0;
int activeConsumer = 0;

pthread_t producerThreads[NUMBER_OF_PRODUCERS];
pthread_t consumerThreads[NUMBER_OF_CONSUMERS];

void swap(struct element *a, struct element *b);

void sortProcessesBySJF();

void *consumer(void *consumerIndex);

void *producer(void *producerIndex);

int main(int argc, char const *argv[]) {
    sem_init(&emptyBufferNum, 0, MAX_BUFFER_SIZE);
    sem_init(&accessSync, 0, 1);
    sem_init(&fullBufferNum, 0, 0);


    int producerIndexArr[NUMBER_OF_PRODUCERS];
    int consumerIndexArr[NUMBER_OF_CONSUMERS];
    for (int producerIndex = 0; producerIndex < NUMBER_OF_PRODUCERS; ++producerIndex) {
        producerIndexArr[producerIndex] = producerIndex;
        pthread_create(&producerThreads[producerIndex], NULL, producer, (void *) &producerIndexArr[producerIndex]);
    }
    for (int consumerIndex = 0; consumerIndex < NUMBER_OF_CONSUMERS; ++consumerIndex) {
        activeConsumer++;
        consumerIndexArr[consumerIndex] = consumerIndex;
        pthread_create(&consumerThreads[consumerIndex], NULL, consumer,
                       (void *) &consumerIndexArr[consumerIndex]);
    }

    //TODO: add a lock by pthread_mutex_lock() to solve the mass between consumer and producer

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
    pthread_exit(NULL);
}

/**
 * terminates when MAX_NUMBER_OF_JOBS have been consumed
 */
void *consumer(void *consumerIndex) {
    int currentConsumerIndex = *((int *) consumerIndex);
    while (1) {
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);
        //TODO: can add a terminate globale vairable to terminate
        if (((consumedProcessNumber + activeConsumer) > MAX_NUMBER_OF_JOBS)) {
            sem_post(&accessSync);
            sem_post(&fullBufferNum);
//            int testSem;
//            printf("Consumer sync %d\n", sem_getvalue(&accessSync, &testSem));
//            printf("Consumer full buffer %d\n", sem_getvalue(&fullBufferNum, &testSem));
//            printf("Consumer empty buffer %d\n", sem_getvalue(&emptyBufferNum, &testSem));
//            printf("consumer ends\n");
            break;
        }
//        sem_post(&accessSync);


//        sem_wait(&accessSync);
//        consumingProcessNumber++;


        processChara *runningProcess = (processChara *) removeFirst(&readyQueueHead, &readyQueueTail);
        consumedProcessNumber++;
//        inBufferThreadCounter--;
        sem_post(&accessSync);


        struct timeval processStartTime;
        struct timeval processEndTime;
        runNonPreemptiveJob(runningProcess, &processStartTime, &processEndTime);

        long int currentResponse = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processStartTime);
        long int currentTurnaround = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processEndTime);
        //TODO:这个accessSync锁可分离成针对多个variable的锁
        sem_wait(&accessSync);
        totalResponseTime += (double) currentResponse;
        totalTurnaroundTime += (double) currentTurnaround;


//        consumingProcessNumber--;
        sem_post(&accessSync);
        //TODO: 之前144没有被提前consume是因为emptyBufferNum在runNonPreemtiveJob之前就post解锁了 produce就能跑了
        sem_post(&emptyBufferNum);
        printf("Consumer = %d, Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turnaround Time = %ld\n",
               currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPreviousBurstTime,
               runningProcess->iRemainingBurstTime, currentResponse, currentTurnaround);

    }
    pthread_exit(NULL);
}

void *producer(void *producerIndex) {
    int currentProducerIndex = *((int *) producerIndex);
    while (1) {
        sem_wait(&emptyBufferNum);
        processChara *newProcess = generateProcess();
        sem_wait(&accessSync);
        if (processIndex == MAX_NUMBER_OF_JOBS) {
            sem_post(&accessSync);
            sem_post(&emptyBufferNum);
//            printf("producer ends\n");
//            sem_getvalue(&accessSync, &testSem);
//            printf("sync %d\n", testSem);
//            sem_getvalue(&fullBufferNum, &testSem);
//            printf("full buffer %d\n", testSem);
            break;
        }


        addLast(newProcess, &readyQueueHead, &readyQueueTail);
        sortProcessesBySJF();
//        inBufferThreadCounter++;
        processIndex++;
        printf("Producer = %d, Items Produced = %d, New Process Id = %d, Burst Time = %d\n", currentProducerIndex,
               processIndex,
               newProcess->iProcessId, newProcess->iInitialBurstTime);
        sem_post(&accessSync);
        sem_post(&fullBufferNum);
    }
    pthread_exit(NULL);
}


//void *consumerWatcher(){
//    if (consumedProcessNumber == MAX_NUMBER_OF_JOBS){
//        pthread_cancel
//    }
//}

/* Bubble sort the given linked list */
void sortProcessesBySJF() {
    int swapped;
    processNode *minNode;
    processNode *lptr = NULL;

    /* Checking for emptyBufferNum list */
    if (readyQueueHead == NULL)
        return;

    do {
        swapped = 0;
        minNode = readyQueueHead;

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