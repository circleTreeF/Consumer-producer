//
// Created by Yizirui Fang on 2020/12/10.
//
typedef struct process processChara;
typedef struct element processNode;

void processGenerator(processNode **pHead, processNode **pTail);

void sortProcessesBySJF(processNode *start);

void swap(processNode *a, processNode *b);
