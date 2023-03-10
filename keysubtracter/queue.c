#include "queue.h"
#include <stdio.h>


bool isEmpty(QueueNode* pHead, QueueNode* pTail, pthread_mutex_t* pQueueMutex) {
	pthread_mutex_lock(pQueueMutex);
	bool bResult = pHead == NULL && pTail == NULL;
	pthread_mutex_unlock(pQueueMutex);
	//printf("IS Queue empty %d\n", bResult);
	return bResult;
}

void pushNode(QueueNode* pNode, QueueNode** ppHead, QueueNode** ppTail, pthread_mutex_t* pQueueMutex, pthread_cond_t* pCond) {
	pthread_mutex_lock(pQueueMutex);
	if(*ppHead == NULL && *ppTail == NULL)
	{
		*ppHead = *ppTail = pNode;
		
		pthread_cond_broadcast(pCond);
		pthread_mutex_unlock(pQueueMutex);
		
		return;
	}
	(*ppTail)->pNext = pNode;
	pNode->pPrev = *ppTail;
	*ppTail = pNode;

	pthread_cond_broadcast(pCond);
	pthread_mutex_unlock(pQueueMutex);

}

QueueNode* checkFrontNode(QueueNode* pHead, QueueNode* pTail) {
	return pHead;
}

QueueNode* popFrontNode(QueueNode** ppHead, QueueNode** ppTail, pthread_mutex_t* pQueueMutex) {
	//printf("Inside popFrontNode \n");
	pthread_mutex_lock(pQueueMutex);
	if(*ppHead == NULL && *ppTail == NULL)
	{
		//printf("Queue is empty \n");
		pthread_mutex_unlock(pQueueMutex);
		return NULL;
	}

	if(*ppHead == *ppTail) // only one node
	{
		//printf("Only single node \n");
		QueueNode* pTemp = *ppHead;
		*ppHead = *ppTail = NULL;
		pthread_mutex_unlock(pQueueMutex);
		return pTemp;
	}

	QueueNode* pTemp = *ppHead;
	*ppHead = (*ppHead)->pNext;
        (*ppHead)->pPrev = NULL;
        pTemp->pNext = NULL;
	pthread_mutex_unlock(pQueueMutex);
        return pTemp;
}

