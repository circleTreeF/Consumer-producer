//
// Created by Yizirui Fang on 2020/12/11.
//

#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>
#include <stdlib.h>


typedef struct process processChara;
typedef struct element processNode;


#define processData(processNode) ((processChara *) processNode->pData)

// if a process is completed
#define isProcessCompleted(process) (processData(process)->iRemainingBurstTime == 0)

//if a process is run before
#define isProcessStart(process) ((processData(process))->iInitialBurstTime != (processData(process))->iRemainingBurstTime)


void processGenerator(processNode **pHead, processNode **pTail);

void swap(processNode *a, processNode *b);

void sortProcessesByPriority(processNode *pHead);

void
dispatchSamePriorityProcesses(processNode **readyQueueHead, processNode **readyQueueTail,
                              processNode **workingQueueHead,
                              processNode **workingQueueTail);

void printProcessList(processNode *readyQueueHead, processNode *readyQueueTail, FILE *outputFile);

void printCompleteProcess(processNode *currentNode, long int *totalTurnaroundTime, FILE *outputFile);

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart,
                         FILE *outputFile);

void
printCompeteProcessInSingleSlide(processNode *currentNode, long int *totalResponseTime, long int *totalTurnaroundTime,
                                 struct timeval onProcessStart, FILE *outputFile);

void printRunningProcess(processNode *currentNode, FILE *outputFile);

void freeWorkingQueueProcess(processNode **workingQueueHead, processNode **workingQueueTail);

int main(int argc, char const *argv[]) {
    processNode *readyQueueHead = NULL;
    processNode *readyQueueTail = NULL;
    processNode *workingQueueHead = NULL;
    processNode *workingQueueTail = NULL;
    FILE *outFile = fopen("TASK1b.txt", "w+");
    processGenerator(&readyQueueHead, &readyQueueTail);
    sortProcessesByPriority(readyQueueHead);
    printProcessList(readyQueueHead, readyQueueTail, outFile);
    long int totalResponseTime = 0;
    long int totalTurnaroundTime = 0;
    dispatchSamePriorityProcesses(&readyQueueHead, &readyQueueTail, &workingQueueHead, &workingQueueTail);
    while (1) {
        int completedProcess = 0;
        int sizeOfWorkingQueue = 0;
        processNode *currentNode = workingQueueHead;
        while (currentNode != NULL) {
            if (!isProcessCompleted(currentNode)) {
                struct timeval onProcessStart;
                struct timeval onProcessEnd;
                //check if the process has been started
                if (isProcessStart(currentNode)) {
                    runPreemptiveJob((processChara *) currentNode->pData, &onProcessStart, &onProcessEnd);
                    //check if this process is completed
                    if (isProcessCompleted(currentNode)) {
                        printCompleteProcess(currentNode, &totalTurnaroundTime, outFile);
                        completedProcess++;
                    } else {
                        //print information
                        printRunningProcess(currentNode, outFile);
                    }
                } else {
                    runPreemptiveJob(processData(currentNode), &onProcessStart, &onProcessEnd);
                    //check if this process is completed
                    if (isProcessCompleted(currentNode)) {
                        //print information for the process starts and complete in the single time slide
                        printCompeteProcessInSingleSlide(currentNode, &totalResponseTime, &totalTurnaroundTime,
                                                         onProcessStart, outFile);
                        completedProcess++;
                    } else {
                        //print information for the process starts but not complete in the single time slide
                        printStartedProcess(currentNode, &totalResponseTime, onProcessStart, outFile);

                    }
                }

            } else {
                completedProcess++;
            }
            currentNode = currentNode->pNext;
            sizeOfWorkingQueue++;
        }
        //check if all processes in the working queue are completed
        if (completedProcess == sizeOfWorkingQueue) {
            if (readyQueueHead == NULL) {
                break;
            } else {
                freeWorkingQueueProcess(&workingQueueHead, &workingQueueTail);
                dispatchSamePriorityProcesses(&readyQueueHead, &readyQueueTail, &workingQueueHead, &workingQueueTail);
                continue;
            }
        }
    }
    //print the overall information
    fprintf(outFile,"Average response time = %f\n", ((double) totalResponseTime) / NUMBER_OF_PROCESSES);
    fprintf(outFile,"Average turn around time = %f\n", ((double) totalTurnaroundTime) / NUMBER_OF_PROCESSES);
    fclose(outFile);
    free((workingQueueHead)->pData);
    free(workingQueueHead);
    return 0;
}

/**
 * Generate the pre-defined amount, NUMBER_OF_PROCESSES of processes in the ready queue
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
 * sort the processes in the priority increasing order in the ready queue
 * @param pHead
 * @return
 */
void sortProcessesByPriority(processNode *pHead) {
    int swapped;
    processNode *currentNode;
    processNode *previousNode = NULL;

    /* Checking for empty list */
    if (pHead == NULL)
        return;

    do {
        swapped = 0;
        currentNode = pHead;

        while (currentNode->pNext != previousNode) {
            if (((processChara *) (currentNode->pData))->iPriority >
                ((processChara *) (currentNode->pNext->pData))->iPriority) {
                swap(currentNode, currentNode->pNext);
                swapped = 1;
            }
            currentNode = currentNode->pNext;
        }
        previousNode = currentNode;
    } while (swapped);
}

/** function to swap data of two nodes a and b*/
void swap(struct element *a, struct element *b) {
    processNode *tempNode = a->pData;
    a->pData = b->pData;
    b->pData = tempNode;
}

/**
 * dispatch all the processes whose priorities are minimal to the working queue. The working queue is another linked list
 * @param readyQueueHead
 * @param readyQueueTail
 * @param workingQueueHead
 * @param workingQueueTail
 */
void
dispatchSamePriorityProcesses(processNode **readyQueueHead, processNode **readyQueueTail,
                              processNode **workingQueueHead,
                              processNode **workingQueueTail) {
    processNode *currentNode = *readyQueueHead;
    int currentPriority = processData(currentNode)->iPriority;
    while (currentNode != NULL) {
        if (((processChara *) currentNode->pData)->iPriority == currentPriority) {
            addLast(processData(currentNode), workingQueueHead, workingQueueTail);
            currentNode = currentNode->pNext;
            removeFirst(readyQueueHead, readyQueueTail);
        } else {
            return;
        }
    }
}

/**
 * print the generated processes information
 * @param readyQueueHead
 * @param readyQueueTail
 * @param outputFile
 */
void printProcessList(processNode *readyQueueHead, processNode *readyQueueTail, FILE *outputFile) {
    fprintf(outputFile,"PROCESS LIST:\n");
    processNode *currentNode = readyQueueHead;
    while (currentNode != NULL) {
        fprintf(outputFile,"Priority %d\n", processData(currentNode)->iPriority);
        while (1) {
            fprintf(outputFile,
                    //Here is 1 tab and 1 space to meet the sample format
                    "\t Process Id = %d, Priority = %d, Initial Burst Time = %d, Remaining Burst Time = %d\n",
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
    fprintf(outputFile,"END\n");
    fprintf(outputFile,"\n");
}


/**
 * print the information when the processes are completely executed
 * @param currentNode
 * @param totalTurnaroundTime
 * @param outputFile
 */
void printCompleteProcess(processNode *currentNode, long int *totalTurnaroundTime, FILE *outputFile) {
    long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated, processData(currentNode)->oMostRecentTime);
    *totalTurnaroundTime += currentProcessTurnaroundTime;
   fprintf(outputFile,
            "Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %ld\n",
            processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
            processData(currentNode)->iPreviousBurstTime,
            processData(currentNode)->iRemainingBurstTime, currentProcessTurnaroundTime);
}

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart, FILE *outputFile) {
    //FIXME: wrong response time, wrong in onProcessStart
    long int currentProcessResponseTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated,
            onProcessStart);
    *totalResponseTime += currentProcessResponseTime;
    fprintf(outputFile,
            "Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %ld\n",
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
    fprintf(outputFile,
            "Process Id = %d, Previous Burst Time: %d Remaining Burst Time = %d,Response Time = %ld, Turnaround Time: %ld\n",
            processData(currentNode)->iProcessId, processData(currentNode)->iPreviousBurstTime,
            processData(currentNode)->iRemainingBurstTime,
            currentProcessResponseTime,
            currentProcessTurnaroundTime);
}

void printRunningProcess(processNode *currentNode, FILE *outputFile) {
    fprintf(outputFile,"Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",
           processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
           processData(currentNode)->iPreviousBurstTime, processData(currentNode)->iRemainingBurstTime);
}


/**
 * free all elements in the working queue after all nodes in one working queue are executed
 * @param workingQueueHead
 * @param workingQueueTail
 */
void freeWorkingQueueProcess(processNode **workingQueueHead, processNode **workingQueueTail) {
    processNode *currentNode = *workingQueueHead;
    while (currentNode != NULL) {
        processNode *freedNode = currentNode;
        processChara *freedProcess = processData(currentNode);
        currentNode = currentNode->pNext;
        free(freedProcess);
        free(freedNode);
    }
    *workingQueueHead = *workingQueueTail = NULL;
}
