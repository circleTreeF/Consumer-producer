//
// Created by Yizirui Fang on 2020/12/10.
//


#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>


typedef struct process processChara;
typedef struct element processNode;

//make a new structure as the node of the list
//or store the time in the 2 new lists
//typedef struct {
//    processChara
//};

#define processData(processNode) ((processChara *) processNode->pData)

// if a process is completed
#define isProcessCompleted(process) (processData(process)->iRemainingBurstTime == 0)

//if a process is run before
#define isProcessStart(process) ((processData(process))->iInitialBurstTime != (processData(process))->iRemainingBurstTime)



void processGenerator(processNode **pHead, processNode **pTail);

void swap(processNode *a, processNode *b);

processNode *sortProcessesByPriority(processNode **pHead);

void
dispatchSamePriorityProcesses(processNode **readyQueueHead, processNode **readyQueueTail,
                              processNode **workingQueueHead,
                              processNode **workingQueueTail);

void printProcessList(processNode *readyQueueHead, processNode *readyQueueTail, FILE *outputFile);

void printCompleteProcess(processNode *currentNode, long int *totalTurnaroundTime, FILE *outputFile);

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart, FILE *outputFile);

void
printCompeteProcessInSingleSlide(processNode *currentNode, long int *totalResponseTime, long int *totalTurnaroundTime,
                                 struct timeval onProcessStart, FILE *outputFile);

void printRunningProcess(processNode *currentNode, FILE *outputFile);

int main(int argc, char const *argv[]) {
    processNode *readyQueueHead = NULL;
    processNode *readyQueueTail = NULL;
    processNode *workingQueueHead = NULL;
    processNode *workingQueueTail = NULL;
    FILE *outFile = fopen("TASK1bOut.txt", "w+");
    processGenerator(&readyQueueHead, &readyQueueTail);
    sortProcessesByPriority(&readyQueueHead);
    printProcessList(readyQueueHead, readyQueueTail, outFile);
    long int totalResponseTime = 0;
    long int totalTurnaroundTime = 0;
    dispatchSamePriorityProcesses(&readyQueueHead, &readyQueueTail, &workingQueueHead, &workingQueueTail);
    while (1) {
        int completedProcess = 0;
        int sizeOfWorkingQueue = 0;
        processNode *currentNode = workingQueueHead;
        while (currentNode != NULL) {
//            completedProcess = 0;
//            sizeOfWorkingQueue = 0;
//            printf("\n%d Created Time %d\n", processData(currentNode)->iRemainingBurstTime,
//                   processData(currentNode)->oTimeCreated);
            if (!isProcessCompleted(currentNode)) {
                struct timeval onProcessStart;
                struct timeval onProcessEnd;
                if (isProcessStart(currentNode)) {
                    runPreemptiveJob((processChara *) currentNode->pData, &onProcessStart, &onProcessEnd);
                    if (isProcessCompleted(currentNode)) {
                        printCompleteProcess(currentNode, &totalTurnaroundTime, outFile);
                        completedProcess++;
                    } else {
                        printRunningProcess(currentNode, outFile);
                    }
                } else {
                    runPreemptiveJob(processData(currentNode), &onProcessStart, &onProcessEnd);
//                    addLast((void *) getDifferenceInMilliSeconds(createdTime(currentNode), *onProcessStart),
//                            (struct element **) responseTimeHead, (struct element **) responseTimeHead);
                    if (isProcessCompleted(currentNode)) {
                        printCompeteProcessInSingleSlide(currentNode, &totalResponseTime, &totalTurnaroundTime,
                                                         onProcessStart, outFile);
                        completedProcess++;
                    } else {
//                        printf("before start time %d\n", onProcessStart);
                        printStartedProcess(currentNode, &totalResponseTime, onProcessStart, outFile);

                    }
                }

            } else {
                completedProcess++;
            }
            currentNode = currentNode->pNext;
            sizeOfWorkingQueue++;
        }
        if (completedProcess == sizeOfWorkingQueue) {
            if (readyQueueHead == NULL) {
                break;
            } else {
                dispatchSamePriorityProcesses(&readyQueueHead, &readyQueueTail, &workingQueueHead, &workingQueueTail);
                continue;
            }
        }
    }
    printf("Average response time = %lf", ((double) totalResponseTime)/NUMBER_OF_PROCESSES);
    printf("Average turn around time = ", ((double) totalTurnaroundTime)/NUMBER_OF_PROCESSES);
    return 0;
}

/*
 * Generate the pre-defined amount, NUMBER_OF_PROCESSES of processes
 */
void processGenerator(processNode **pHead, processNode **pTail) {
    int numNode = 0;
    while (numNode < NUMBER_OF_PROCESSES) {
        processChara *newProcess = generateProcess();
        addLast(newProcess, pHead, pTail);
        numNode++;
    }
}

/**
 * sort the processes in the priority order
 * @param pHead
 * @return
 */
processNode *sortProcessesByPriority(processNode **pHead) {
    int swapped, i;
    processNode *currentNode;
    processNode *priorNode = NULL;

    /* Checking for empty list */
    if (pHead == NULL)
        return NULL;

    do {
        swapped = 0;
        currentNode = *pHead;

        while (currentNode->pNext != priorNode) {
            if (((processChara *) (currentNode->pData))->iPriority >
                ((processChara *) (currentNode->pNext->pData))->iPriority) {
                swap(currentNode, currentNode->pNext);
                swapped = 1;
            }
            currentNode = currentNode->pNext;
        }
        priorNode = currentNode;
    } while (swapped);
}

/* function to swap data of two nodes a and b*/
void swap(struct element *a, struct element *b) {
    processNode *tempNode = a->pData;
    a->pData = b->pData;
    b->pData = tempNode;
}


void
dispatchSamePriorityProcesses(processNode **readyQueueHead, processNode **readyQueueTail,
                              processNode **workingQueueHead,
                              processNode **workingQueueTail) {
    processNode *currentNode = *readyQueueHead;
    int currentPriority = processData(currentNode)->iPriority;
    while (currentNode != NULL) {
        if (((processChara *) currentNode->pData)->iPriority == currentPriority) {
            addLast(processData(currentNode), workingQueueHead, workingQueueTail);
//            printf("Id %d\n", processData(currentNode)->iProcessId);
            currentNode = currentNode->pNext;

            removeFirst(readyQueueHead, readyQueueTail);
        } else {
            return;
        }
    }
}


void printProcessList(processNode *readyQueueHead, processNode *readyQueueTail, FILE *outputFile) {
    printf("PROCESS LIST:\n");
    processNode *currentNode = readyQueueHead;
    while (currentNode != NULL) {
        printf("Priority %d\n", processData(currentNode)->iPriority);
        while (1) {
            printf("             Process Id = %d, Priority = %d, Initial Burst Time = %d, Remaining Burst Time = %d\n",
                   processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
                   processData(currentNode)->iInitialBurstTime, processData(currentNode)->iRemainingBurstTime);
            //check if the next process is on the same priority
            if (currentNode->pNext != NULL) {
                if (processData(currentNode)->iPriority != processData(currentNode->pNext)->iPriority) {
                    currentNode = currentNode->pNext;
                    break;
                }
            } else {
                currentNode = currentNode->pNext;
                break;
            }
            currentNode = currentNode->pNext;
        }
    }
    printf("END\n");
}

void printCompleteProcess(processNode *currentNode, long int *totalTurnaroundTime, FILE *outputFile) {
    long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated, processData(currentNode)->oMostRecentTime);
    *totalTurnaroundTime += currentProcessTurnaroundTime;
    printf("Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n",
           processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
           processData(currentNode)->iPreviousBurstTime,
           processData(currentNode)->iRemainingBurstTime, currentProcessTurnaroundTime);
}

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart, FILE *outputFile) {
    //FIXME: wrong response time, wrong in onProcessStart
//    printf("in function create %d, start %d\n", processData(currentNode)->oTimeCreated,
//           onProcessStart);
    long int currentProcessResponseTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated,
            onProcessStart);
//    printf("response %d, create %d, start %d\n", currentProcessResponseTime, processData(currentNode)->oTimeCreated,
//           onProcessStart);
    *totalResponseTime += currentProcessResponseTime;
    printf("Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n",
           processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
           processData(currentNode)->iPreviousBurstTime, processData(currentNode)->iRemainingBurstTime,
           currentProcessResponseTime);
}

void
printCompeteProcessInSingleSlide(processNode *currentNode, long int *totalResponseTime, long int *totalTurnaroundTime,
                                 struct timeval onProcessStart, FILE *outputFile) {
    long int currentProcessResponseTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated,
            onProcessStart);
    long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated, processData(currentNode)->oMostRecentTime);
    *totalResponseTime += currentProcessResponseTime;
    *totalTurnaroundTime += currentProcessTurnaroundTime;
    printf("Process Id = %d, Previous Burst Time: %d Remaining Burst Time = %d,Response Time = %d Turnaround Time: %d\n",
           processData(currentNode)->iProcessId, processData(currentNode)->iPreviousBurstTime,
           processData(currentNode)->iRemainingBurstTime,
           currentProcessResponseTime,
           currentProcessTurnaroundTime);
}

void printRunningProcess(processNode *currentNode, FILE *outputFile) {
    printf("Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",
           processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
           processData(currentNode)->iPreviousBurstTime, processData(currentNode)->iRemainingBurstTime);
}

//void runRoundRobin(processNode **workingQueueHead, processNode **workingQueueTail) {
//
//}