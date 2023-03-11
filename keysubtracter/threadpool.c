#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "threadpool.h"

static void* workerThread(void * pArg)
{
	printf("Inside worker Thread %ld\n", pthread_self());
	ThreadPool* pThreadPool = (ThreadPool *)pArg;
	while(1)
	{
		//printf("Inside while loop\n");
		pthread_mutex_lock(&(pThreadPool->workMutex));
		//printf("workMutex Lock acquired\n");
		while(!pThreadPool->bStop && isEmpty(pThreadPool->pHead, pThreadPool->pTail, &(pThreadPool->queueMutex)))
		{
			//printf("Waiting %ld\n", pthread_self());
			pthread_cond_wait(&(pThreadPool->workCond), &(pThreadPool->workMutex));
			//printf("Waiting end %ld\n", pthread_self());
		}

		pthread_mutex_unlock(&(pThreadPool->workMutex));
		
		//printf("step out from empty waiting\n");

		if(pThreadPool->bStop)
		{
			//printf("Thread coming out from its loop\n");
			break;
		}
		

		QueueNode* pNode = popFrontNode(&(pThreadPool->pHead), &(pThreadPool->pTail), &(pThreadPool->queueMutex));
		//printf("Get worker node %p\n", pNode);
		pThreadPool->nWorkCount--;

		if(pNode != NULL)
		{
			//printf("Execute worker node %p\n", pNode);
			pNode->func(pNode->pArgs);
			destroyWorkNode(pNode);
			//printf("destroy worker node %p\n", pNode);
		}
		pthread_mutex_lock(&(pThreadPool->threadPoolMutex));
		if(pThreadPool->bStop && isEmpty(pThreadPool->pHead, pThreadPool->pTail, &(pThreadPool->queueMutex)))
		{
			//printf("Signaling working condition\n");
			pthread_cond_signal(&(pThreadPool->workingCond));
		}

		pthread_mutex_unlock(&(pThreadPool->threadPoolMutex));
	}

	pthread_mutex_lock(&(pThreadPool->threadPoolMutex));
	pthread_cond_broadcast(&(pThreadPool->workCond));
	pthread_cond_broadcast(&(pThreadPool->workingCond));
	pThreadPool->nThreadCount--;
	pthread_mutex_unlock(&(pThreadPool->threadPoolMutex));

	printf("Thread destoryed %ld\n", pthread_self());

	return NULL;
}

ThreadPool* initThreadPool() {
	int numOfCPU = sysconf(_SC_NPROCESSORS_ONLN);
	printf("number of cpu %d\n", numOfCPU);
	ThreadPool* pThreadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
	
	pThreadPool->nThreadCount = 0;
	pThreadPool->pHead = NULL;
	pThreadPool->pTail = NULL;
	pThreadPool->bStop = false;
	pThreadPool->nWorkCount = 0;

	pthread_mutex_init(&(pThreadPool->workMutex), NULL);
	pthread_mutex_init(&(pThreadPool->queueMutex), NULL);
	pthread_mutex_init(&(pThreadPool->threadPoolMutex), NULL);
	pthread_cond_init(&(pThreadPool->workCond), NULL);
	pthread_cond_init(&(pThreadPool->workingCond), NULL);

	pthread_t thread;
	for(int i = 0; i < numOfCPU; ++i)
	{
		//printf("Thread %d creating\n", i);
		pthread_create(&thread, NULL, workerThread, (void *)pThreadPool);
		pthread_detach(thread);
		pThreadPool->nThreadCount++;
		//printf("Thread %d created\n", i);
	}
	return pThreadPool;
}

QueueNode* createWorkNode(thread_func_t func, void *pArgs)
{
	QueueNode* pQueueNode = (QueueNode* )malloc(sizeof(QueueNode));
	pQueueNode->func = func;
	pQueueNode->pArgs = pArgs;
	pQueueNode->pNext = NULL;
	pQueueNode->pPrev = NULL;
	return pQueueNode;
}

void destroyWorkNode(QueueNode* pNode)
{
	pNode->pNext = NULL;
	pNode->pPrev = NULL;
	pNode->func = NULL;
	pNode->pArgs = NULL;
	//printf("Freeing worker node %p\n", pNode);
	free(pNode);
	//printf("Freed worker node %p\n", pNode);
}

void waitThreadPool(ThreadPool* pThreadPool)
{
	if(pThreadPool == NULL)
	{
		return;
	}

	pthread_mutex_lock(&(pThreadPool->workMutex));
	//printf("Stopping threadpool and broadcasting signal\n");
	pThreadPool->bStop = true;
	pthread_cond_broadcast(&(pThreadPool->workCond));
	pthread_mutex_unlock(&(pThreadPool->workMutex));
	
	//printf("broadcasting is done and thread count %lu\n", pThreadPool->nThreadCount);
	pthread_mutex_lock(&(pThreadPool->threadPoolMutex));
	while(1)
	{
		//printf("Threads remaining %lu\n", pThreadPool->nThreadCount);
		if(pThreadPool->nThreadCount != 0)
		{
			pthread_cond_wait(&(pThreadPool->workingCond), &(pThreadPool->threadPoolMutex));
		}
		else
		{
			break;
		}

	}

	pthread_mutex_unlock(&(pThreadPool->threadPoolMutex));
}

void destroyThreadPool(ThreadPool* pThreadPool)
{
	if(pThreadPool == NULL)
		return;

	//pthread_mutex_lock(&(pThreadPool->workMutex));
	//pthread_cond_broadcast(&(pThreadPool->workCond));
	//pthread_mutex_unlock(&(pThreadPool->workMutex));
	
	waitThreadPool(pThreadPool);

	//pthread_mutex_destory(&(pThreadPool->workMutex));
	//pthread_cond_destory(&(pThreadPool->workCond));
	//phread_cond_destroy(&(pThreadPool->workingCond));
	
	printf("Destroying threadpool %p\n", pThreadPool);
	free(pThreadPool);
}

void worker(void *arg)
{
    int *val = arg;
    int  old = *val;

    *val += 1000;
    printf("tid=%ld, old=%d, val=%d\n", pthread_self(), old, *val);

    if (*val%2)
        usleep(100000);
}

/*
int main(int argc, char** argv)
{
	//printf("Before creating threadpool\n");
	ThreadPool* pThreadPool = initThreadPool();
	//printf("After creating threadpool\n");
	int     *vals = (int *)malloc(sizeof(int)* 8);
	for(int i = 0; i < 8; ++i)
	{
		vals[i] = i;
		//printf("Creating worker node: %d\n", i);
		QueueNode* pNode = createWorkNode(worker, vals + i);
		//printf("pushing worker node: %d\n", i);
		pushNode(pNode, &(pThreadPool->pHead), &(pThreadPool->pTail), &(pThreadPool->queueMutex), &(pThreadPool->workCond));
	}
	
	//printf("Waiting for threadpool to complete.\n");
	waitThreadPool(pThreadPool);

	//printf("threadpool to completed.\n");
	for (int i=0; i<8; i++) {
        	printf("%d\n", vals[i]);
    	}
	free(vals);

	//printf("destroying threadpool.\n");
	destroyThreadPool(pThreadPool);
	
	printf("Threadpool completed.\n");
	return 0;
}*/
