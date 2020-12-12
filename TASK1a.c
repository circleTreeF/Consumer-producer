#include "coursework.h"
#include "linkedlist.h"
#include <stdio.h>

typedef struct process processChara;
typedef struct element processNode;

void processGenerator(processNode **pHead, processNode **pTail);

void sortProcessesBySJF(processNode *start);

void swap(processNode *a, processNode *b);


int main(int argc, char const *argv[]) {

    FILE *outFile = fopen("TASK1a.txt", "w+");
    processNode *processesHead = NULL;
    processNode *processTail = NULL;

    processGenerator(&processesHead, &processTail);
    sortProcessesBySJF(processesHead);
    int completedProcess = 0;
    long int totalResponseTime = 0;
    long int totalTurnaroundTime = 0;
    while (completedProcess < NUMBER_OF_PROCESSES) {
        processNode *currentNode = processesHead;
        processChara *currentProcess = (processChara *) currentNode->pData;
        struct timeval processStartTime;
        struct timeval processEndTime;
        runNonPreemptiveJob((processChara *) currentProcess, &processStartTime, &processEndTime);
        long int processResponseTime = getDifferenceInMilliSeconds(currentProcess->oTimeCreated, processStartTime);
        long int processTurnaroundTime = getDifferenceInMilliSeconds(currentProcess->oTimeCreated, processEndTime);
        totalResponseTime += processResponseTime;
        totalTurnaroundTime += processTurnaroundTime;
        completedProcess++;
        fprintf(outFile,
                "Process Id = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %ld, Turnaround Time = %ld\n",
                currentProcess->iProcessId, currentProcess->iPreviousBurstTime,
                currentProcess->iRemainingBurstTime, processResponseTime, processTurnaroundTime);
        removeFirst(&processesHead, &processTail);
    }
    //FIXME:the expect: Average response time = 319.100000
    //Average turn around time = 416.600000
    //Actual:
    // Average response time = 319.100006
    //Average turnaround time = 416.600006
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


/* Bubble sort the given linked list */
void sortProcessesBySJF(processNode *start) {
    int swapped;
    processNode *minNode;
    processNode *lptr = NULL;

    /* Checking for empty list */
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