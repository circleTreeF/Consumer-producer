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


processNode *readyQueueHead = NULL;
processNode *readyQueueTail = NULL;

#define processData(processNode) ((processChara *) processNode->pData)

sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;

int isProducerDone = 0;

double totalResponseTime = 0.0;
double totalTurnaroundTime = 0.0;
int processIndex = 0;
int consumedProcessNumber = 0;

int activeConsumer = 0;

pthread_t producerThreads[NUMBER_OF_PRODUCERS];
pthread_t consumerThreads[NUMBER_OF_CONSUMERS];

void *consumer(void *consumerIndex);

void *producer(void *producerIndex);

void insertToQueue(processChara *newProcess);

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

    for (int producerIndex = 0; producerIndex < NUMBER_OF_PRODUCERS; ++producerIndex) {
        pthread_join(producerThreads[producerIndex], NULL);
    }
    for (int consumerIndex = 0; consumerIndex < NUMBER_OF_CONSUMERS; ++consumerIndex) {
        pthread_join(consumerThreads[consumerIndex], NULL);
    }
    printf("Average Response Time = %f\n", totalResponseTime / MAX_NUMBER_OF_JOBS);
    printf("Average Turn Around Time = %f\n", totalTurnaroundTime / MAX_NUMBER_OF_JOBS);
    //release the memory of the head and tail of the buffer linked list
    free(readyQueueHead);
    free(readyQueueTail);
    return 0;
}

/**
 * terminates when MAX_NUMBER_OF_JOBS have been consumed
 */
void *consumer(void *consumerIndex) {
    int currentConsumerIndex = *((int *) consumerIndex);
    while (1) {
        /** critical section to check if all process consumed starts*/
        sem_wait(&accessSync);
        if (((consumedProcessNumber + activeConsumer) > MAX_NUMBER_OF_JOBS)) {
            activeConsumer--;
            sem_post(&accessSync);
            break;
        }
        sem_post(&accessSync);
        /** critical section to check if all process consumed ends*/
        /** critical section to remove one process from the buffer starts*/
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);

        processChara *runningProcess = (processChara *) removeFirst(&readyQueueHead, &readyQueueTail);
        consumedProcessNumber++;
        sem_post(&accessSync);
        /** critical section to remove one process from the buffer ends*/

        struct timeval processStartTime;
        struct timeval processEndTime;
        runNonPreemptiveJob(runningProcess, &processStartTime, &processEndTime);

        long int currentResponse = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processStartTime);
        long int currentTurnaround = getDifferenceInMilliSeconds(runningProcess->oTimeCreated, processEndTime);
        /** critical section to update the shared variables, total response time and turn around time starts*/
        sem_wait(&accessSync);
        totalResponseTime += (double) currentResponse;
        totalTurnaroundTime += (double) currentTurnaround;
        printf("Consumer = %d, Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turnaround Time = %ld\n",
               currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPreviousBurstTime,
               runningProcess->iRemainingBurstTime, currentResponse, currentTurnaround);
        sem_post(&accessSync);
        sem_post(&emptyBufferNum);
        /**critical section to pdate the shared variables, total response time and turn around time ends*/
        //release the memory of running process stored in the data section of the node of the linked list
        free(runningProcess);
    }
    return NULL;
}

void *producer(void *producerIndex) {
    int currentProducerIndex = *((int *) producerIndex);
    while (1) {
        /** critical section start*/
        sem_wait(&emptyBufferNum);
        sem_wait(&accessSync);
        //check if the producers have produced all processes as required
        if (isProducerDone == 1) {
            sem_post(&accessSync);
            break;
        }
        //generate the new process and insert to the buffer
        processChara *newProcess = generateProcess();
        insertToQueue(newProcess);
        processIndex++;
        printf("Producer = %d, Items Produced = %d, New Process Id = %d, Burst Time = %d\n", currentProducerIndex,
               processIndex,
               newProcess->iProcessId, newProcess->iInitialBurstTime);
        //when the producers have produced all processes as required, set the variable to 1 to remain other process to terminate
        if (processIndex >= MAX_NUMBER_OF_JOBS) {
            isProducerDone = 1;
            sem_post(&accessSync);
            sem_post(&fullBufferNum);
            break;
        }
        sem_post(&accessSync);
        sem_post(&fullBufferNum);
        /** critical section ends*/
    }
    return NULL;
}


/**
 * insert the new process into the ready queue in the increasing order of burst time
 * @param newProcess
 */
void insertToQueue(processChara *newProcess) {
    processNode *pNewElement = (processNode *) malloc(sizeof(processNode));
    if (pNewElement == NULL) {
        printf("Malloc Failed\n");
        exit(-1);
    }
    pNewElement->pData = (void *) newProcess;
    pNewElement->pNext = NULL;

    //check if the ready queue is empty
    if (readyQueueHead == NULL) {
        readyQueueTail = readyQueueHead = pNewElement;
    } else {
        processNode *currentElement = readyQueueHead;
        /*
         * traverse the linked list to find the node whose initial burst time of the pData is less than the initial burst time of the new process
         * while the pData of next node of this node has a greater initial burst time
         */
        while (currentElement->pNext != NULL) {
            if ((newProcess->iInitialBurstTime >= processData(currentElement)->iInitialBurstTime) &&
                (newProcess->iInitialBurstTime <= processData(currentElement->pNext)->iInitialBurstTime)) {
                break;
            }
            currentElement = currentElement->pNext;
        }
        //if the currentElement is the last element in the ready queue
        if (currentElement == readyQueueTail) {
            pNewElement->pNext = NULL;
            readyQueueTail->pNext = pNewElement;
            readyQueueTail = pNewElement;
        } else {
            //the current is in the body of the ready queue
            pNewElement->pNext = currentElement->pNext;
            currentElement->pNext = pNewElement;
        }
    }
}

