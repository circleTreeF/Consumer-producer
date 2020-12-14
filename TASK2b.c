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


sem_t emptyBufferNum;
sem_t accessSync;
sem_t fullBufferNum;
//the semaphore to sync the variable consumedProcessNumber between consumers, which counts the number of processes been consumed
//sem_t producerIndexSync;
sem_t consumedProcessSync;

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

int main(int argc, char const *argv[]) {
    sem_init(&emptyBufferNum, 0, MAX_BUFFER_SIZE);
    sem_init(&accessSync, 0, 1);
    sem_init(&fullBufferNum, 0, 0);
    sem_init(&consumedProcessSync, 0, 1);

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

    return 0;
}

void *producer(void *producerIndex) {
    int currentProducerIndex = *((int *) producerIndex);
    while (1) {
        /** critical section start*/
        sem_wait(&emptyBufferNum);
        sem_wait(&accessSync);
        if (processIndex == MAX_NUMBER_OF_JOBS) {
            sem_post(&accessSync);
            sem_post(&emptyBufferNum);
            break;
        }

        processChara *newProcess = generateProcess();

        addLast(newProcess, &priorityReadyQueueHeadArray[newProcess->iPriority],
                &priorityReadyQueueTailArray[newProcess->iPriority]);
        processIndex++;
        printf("Producer = %d, Items Produced = %d, New Process Id = %d, Burst Time = %d\n", currentProducerIndex,
               processIndex,
               newProcess->iProcessId, newProcess->iInitialBurstTime);
        sem_post(&accessSync);
        sem_post(&fullBufferNum);
        /** critical section ends*/
    }

    return NULL;
}


void *consumer(void *consumerIndex) {
    int currentConsumerIndex = *((int *) consumerIndex);
    while (1) {
        /** critical section to check if all process consumed start*/
        sem_wait(&consumedProcessSync);
        if (((consumedProcessNumber + activeConsumer) >= MAX_NUMBER_OF_JOBS)) {
            activeConsumer--;
            sem_post(&consumedProcessSync);
            break;
        }
        sem_post(&consumedProcessSync);
        /** critical section to check if all process consumed ends*/
        /** critical section for add process into the buffer, round robin and print the process information starts*/
        sem_wait(&fullBufferNum);
        sem_wait(&accessSync);

        processChara *runningProcess = (processChara *) findProcess();
        sem_post(&accessSync);
        /** critical section for add process into the buffer ends*/



        struct timeval processStartTime;
        struct timeval processEndTime;
        runPreemptiveJob(runningProcess, &processStartTime, &processEndTime);
        /** critical section for round robin and print the process information starts*/
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
                free(runningProcess);
                sem_post(&accessSync);
                sem_post(&emptyBufferNum);
                /** critical section for round robin and print the process information ends, while the process is fully executed*/
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
                /** critical section for round robin and print the process information ends, while the process is not fully executed*/
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
                free(runningProcess);
                sem_post(&accessSync);
                sem_post(&emptyBufferNum);
                /** critical section for round robin and print the process information ends, while the process is fully executed*/
            } else {
                printf("Consumer = %d, Process Id = %d, Priority = %d, Previous Burst Time = %d, New Burst Time = %d\n",
                       currentConsumerIndex, runningProcess->iProcessId, runningProcess->iPriority,
                       runningProcess->iPreviousBurstTime, runningProcess->iRemainingBurstTime);
                addLast(runningProcess, &priorityReadyQueueHeadArray[runningProcess->iPriority],
                        &priorityReadyQueueTailArray[runningProcess->iPriority]);
                sem_post(&accessSync);
                sem_post(&fullBufferNum);
                /** critical section for round robin and print the process information ends, while the process is not fully executed*/
            }
        }

    }
    return NULL;
}


/**
 * find the process in the buffer with the smallest priority
 * @return
 */
void *findProcess() {
    for (int priorityIndex = 0; priorityIndex < MAX_PRIORITY; ++priorityIndex) {
        if (priorityReadyQueueHeadArray[priorityIndex] != NULL) {
            return removeFirst(&priorityReadyQueueHeadArray[priorityIndex],
                               &priorityReadyQueueTailArray[priorityIndex]);
        }
    }
}
