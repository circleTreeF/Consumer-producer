//
// Created by Yizirui Fang on 2020/12/13.
//

#include <pthread.h>
#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>

typedef struct process processChara;
typedef struct element processNode;

#define processData(processNode) ((processChara *) processNode->pData)

processNode *readyQueueHead = NULL;
processNode *readyQueueTail = NULL;
//processNode *workingQueueHead = NULL;
//processNode *workingQueueTail = NULL;

sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;
int *inBufferThreadCounter = 0;
double totalResponseTime = 0.0;
double totalTurnaroundTime = 0.0;
int processIndex = 0;

pthread_t producerThreads[NUMBER_OF_PRODUCERS];
pthread_t consumerThreads[NUMBER_OF_CONSUMERS];

processNode *priorityReadyQueueHeadArray[MAX_PRIORITY];
processNode *priorityReadyQueueTailArray[MAX_PRIORITY];

int consumedProcessNumber = 0;
int activeConsumer = 0;

void *consumer(void *consumerIndex);

void *producer(void *producerIndex);

void *findProcess();

void swap(processNode *a, processNode *b);

void sortProcessesByPriority();

void dispatchSamePriorityProcesses();

int main(int argc, char const *argv[]) {
    sem_init(&emptyBufferNum, 0, MAX_BUFFER_SIZE);
    sem_init(&accessSync, 0, 1);
    sem_init(&fullBufferNum, 0, 0);

    for (int index = 0; index < MAX_PRIORITY; ++index) {
        priorityReadyQueueHeadArray[index] = NULL;
        priorityReadyQueueTailArray[index] = NULL;
    }

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
    pthread_exit(NULL);
}

void *producer(void *producerIndex) {
    int currentProducerIndex = *((int *) producerIndex);
    while (1) {
        sem_wait(&emptyBufferNum);
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

        processChara *newProcess = generateProcess();

        addLast(newProcess, &priorityReadyQueueHeadArray[newProcess->iPriority ],
                &priorityReadyQueueTailArray[newProcess->iPriority ]);
//        sortProcessesByPriority();
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


void *consumer(void *consumerIndex) {
    int currentConsumerIndex = *((int *) consumerIndex);
    while (1) {
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);
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
        processChara *runningProcess = (processChara *) findProcess();
        sem_post(&accessSync);
//        sem_post(&emptyBufferNum);


        struct timeval processStartTime;
        struct timeval processEndTime;
        runPreemptiveJob(runningProcess, &processStartTime, &processEndTime);

        sem_wait(&accessSync);
        //if the process just start
        if (runningProcess->iPreviousBurstTime == runningProcess->iInitialBurstTime) {
            long int currentProcessResponseTime = getDifferenceInMilliSeconds(
                    runningProcess->oTimeCreated,
                    processStartTime);
            totalResponseTime += (double) currentProcessResponseTime;
            //check if the process become finished
            if (runningProcess->iRemainingBurstTime == 0) {
                long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
                        runningProcess->oTimeCreated, processEndTime);
                totalTurnaroundTime += (double) currentProcessTurnaroundTime;
                consumedProcessNumber++;
                printf("Consumer = %d, Process Id = %d, Priority = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turnaround Time = %ld\n",
                       currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPriority,
                       runningProcess->iPreviousBurstTime, runningProcess->iRemainingBurstTime,
                       currentProcessResponseTime, currentProcessTurnaroundTime);
                sem_post(&accessSync);
                sem_post(&emptyBufferNum);
            } else {
                printf("Consumer = %d, Process Id = %d, Priority = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld\n",
                       currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPriority,
                       runningProcess->iPreviousBurstTime, runningProcess->iRemainingBurstTime,
                       currentProcessResponseTime);

                addLast(runningProcess, &priorityReadyQueueHeadArray[runningProcess->iPriority],
                        &priorityReadyQueueTailArray[runningProcess->iPriority]);
                sem_post(&accessSync);
                //TODO:有可能consume的时候有produce了新的 就会超buffer
                sem_post(&fullBufferNum);
            }
        } else {
            //check if the process become finished
            if (runningProcess->iRemainingBurstTime == 0) {
                long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
                        runningProcess->oTimeCreated, processEndTime);
                totalTurnaroundTime += (double) currentProcessTurnaroundTime;
                printf("Consumer = %d, Process Id = %d, Priority = %d, Previous Burst Time = %d, New Burst Time = %d, Turnaround Time = %ld\n",
                       currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPriority,
                       runningProcess->iPreviousBurstTime, runningProcess->iRemainingBurstTime,
                       currentProcessTurnaroundTime);
                consumedProcessNumber++;
                sem_post(&accessSync);
                sem_post(&emptyBufferNum);
            } else {
                printf("Consumer = %d, Process Id = %d, Priority = %d, Previous Burst Time = %d, New Burst Time = %d\n",
                       currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPriority,
                       runningProcess->iPreviousBurstTime, runningProcess->iRemainingBurstTime);
                addLast(runningProcess, &priorityReadyQueueHeadArray[runningProcess->iPriority ],
                        &priorityReadyQueueTailArray[runningProcess->iPriority ]);
                sem_post(&accessSync);
                sem_post(&fullBufferNum);
            }
        }

    }
    pthread_exit(NULL);
}


//TODO: check if -1 is necessary
void *findProcess() {
    for (int priorityIndex = 0; priorityIndex < MAX_PRIORITY; ++priorityIndex) {
        if (priorityReadyQueueHeadArray[priorityIndex] != NULL) {
            return removeFirst(&priorityReadyQueueHeadArray[priorityIndex ],
                               &priorityReadyQueueTailArray[priorityIndex]);
        }
    }
}


//void sortProcessesByPriority() {
//    int swapped, i;
//    processNode *currentNode;
//    processNode *priorNode = NULL;
//
//    /* Checking for empty list */
//    if (readyQueueHead == NULL)
//        return;
//
//    do {
//        swapped = 0;
//        currentNode = readyQueueHead;
//
//        while (currentNode->pNext != priorNode) {
//            if (((processChara *) (currentNode->pData))->iPriority >
//                ((processChara *) (currentNode->pNext->pData))->iPriority) {
//                swap(currentNode, currentNode->pNext);
//                swapped = 1;
//            }
//            currentNode = currentNode->pNext;
//        }
//        priorNode = currentNode;
//    } while (swapped);
//}
//
///* function to swap data of two nodes a and b*/
//void swap(struct element *a, struct element *b) {
//    processNode *tempNode = a->pData;
//    a->pData = b->pData;
//    b->pData = tempNode;
//}
//
//void dispatchSamePriorityProcesses() {
//    processNode *currentNode = readyQueueHead;
//    int currentPriority = processData(currentNode)->iPriority;
//    while (processData(readyQueueHead)->iPriority != currentPriority) {
////        if (((processChara *) currentNode->pData)->iPriority == currentPriority) {
//            //addLast(processData(currentNode), workingQueueHead, workingQueueTail);
//            addLast(removeFirst(&readyQueueHead, &readyQueueTail), &workingQueueHead, &workingQueueTail);
////            currentNode = currentNode->pNext;
////            removeFirst(readyQueueHead, readyQueueTail);
////        } else {
////            return;
////        }
//    }
//}