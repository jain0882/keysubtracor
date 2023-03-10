#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

//#include <pthread_mutex.h>
//#include <pthread_cond.h>
#include <stdbool.h>
#include "queue.h"

typedef struct ThreadPool_ {
	QueueNode* pHead;
	QueueNode* pTail;
	pthread_mutex_t workMutex;
	pthread_mutex_t threadPoolMutex;
	pthread_mutex_t queueMutex;
	pthread_cond_t workCond;
	pthread_cond_t workingCond;
	size_t nWorkCount;
	size_t nThreadCount;
	bool bStop;
} ThreadPool;

ThreadPool* initThreadPool();
QueueNode* createWorkNode(thread_func_t func, void* pArgs);
void destroyWorkNode(QueueNode* pNode);
void waitThreadPool(ThreadPool* pThreadPool);
void destroyThreadPool(ThreadPool* pThreadPool);

#endif //_THREAD_POOL_H
