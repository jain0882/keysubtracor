#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

typedef void (*thread_func_t)(void *arg);
typedef struct QueueNode_ {
	struct QueueNode_* pPrev;
	struct QueueNode_* pNext;
	void* pArgs;
	thread_func_t func;
}QueueNode;


bool isEmpty(QueueNode* pHead, QueueNode* pTail, pthread_mutex_t* pQueueMutex);
void pushNode(QueueNode* pNode, QueueNode** ppHead, QueueNode** ppTail, pthread_mutex_t* pQueueMutex, pthread_cond_t* pCond);
QueueNode* checkFrontNode(QueueNode* pHead, QueueNode* pTail);
QueueNode* popFrontNode(QueueNode** ppHead, QueueNode** ppTail, pthread_mutex_t* pQueueMutex);
#endif //_QUEUE_H_
