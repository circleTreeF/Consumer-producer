//
// Created by Yizirui Fang on 2020/12/11.
//

#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>


typedef struct process processChara;
typedef struct element processNode;


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

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart,
                         FILE *outputFile);

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
                    if (isProcessCompleted(currentNode)) {
                        printCompeteProcessInSingleSlide(currentNode, &totalResponseTime, &totalTurnaroundTime,
                                                         onProcessStart, outFile);
                        completedProcess++;
                    } else {
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
    fprintf(outFile, "Average response time = %f\n", ((double) totalResponseTime) / NUMBER_OF_PROCESSES);
    fprintf(outFile, "Average turn around time = %f\n", ((double) totalTurnaroundTime) / NUMBER_OF_PROCESSES);
    fclose(outFile);
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
            currentNode = currentNode->pNext;

            removeFirst(readyQueueHead, readyQueueTail);
        } else {
            return;
        }
    }
}


void printProcessList(processNode *readyQueueHead, processNode *readyQueueTail, FILE *outputFile) {
    fprintf(outputFile, "PROCESS LIST:\n");
    processNode *currentNode = readyQueueHead;
    while (currentNode != NULL) {
        fprintf(outputFile, "Priority %d\n", processData(currentNode)->iPriority);
        while (1) {
            fprintf(outputFile,
                    //Here is 1 tab and 1 space, TODO: manual tab is not tab in written to the file while not recognize by vscode
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
    fprintf(outputFile, "END\n");
    fprintf(outputFile, "\n");
}

void printCompleteProcess(processNode *currentNode, long int *totalTurnaroundTime, FILE *outputFile) {
    long int currentProcessTurnaroundTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated, processData(currentNode)->oMostRecentTime);
    *totalTurnaroundTime += currentProcessTurnaroundTime;
    fprintf(outputFile,
            "Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n",
            processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
            processData(currentNode)->iPreviousBurstTime,
            processData(currentNode)->iRemainingBurstTime, currentProcessTurnaroundTime);
}

void printStartedProcess(processNode *currentNode, long int *totalResponseTime, struct timeval onProcessStart,
                         FILE *outputFile) {
    //FIXME: wrong response time, wrong in onProcessStart
    long int currentProcessResponseTime = getDifferenceInMilliSeconds(
            processData(currentNode)->oTimeCreated,
            onProcessStart);
    *totalResponseTime += currentProcessResponseTime;
    fprintf(outputFile,
            "Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n",
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
            "Process Id = %d, Previous Burst Time: %d Remaining Burst Time = %d,Response Time = %d Turnaround Time: %d\n",
            processData(currentNode)->iProcessId, processData(currentNode)->iPreviousBurstTime,
            processData(currentNode)->iRemainingBurstTime,
            currentProcessResponseTime,
            currentProcessTurnaroundTime);
}

void printRunningProcess(processNode *currentNode, FILE *outputFile) {
    fprintf(outputFile, "Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",
            processData(currentNode)->iProcessId, processData(currentNode)->iPriority,
            processData(currentNode)->iPreviousBurstTime, processData(currentNode)->iRemainingBurstTime);
}
