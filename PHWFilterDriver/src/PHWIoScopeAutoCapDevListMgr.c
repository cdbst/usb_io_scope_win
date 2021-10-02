#include "PHWIoScopeAutoCapDevListMgr.h"
#include "PHWIoScope.h"

VOID PHWIoScopeAutoCapDevListMgr_InitList(PAUTO_CAP_DEV_LIST pAutoCapDevList){
	KeInitializeSpinLock(&pAutoCapDevList->autoCapDevListSpinLock);
	pAutoCapDevList->pListHead = NULL;
	pAutoCapDevList->pListTail = NULL;
	pAutoCapDevList->numOfAutoCapDevNodes = 0;
}

VOID PHWIoScopeAutoCapDevListMgr_ReleaseList(PAUTO_CAP_DEV_LIST pAutoCapDevList){
	PAUTO_CAP_DEV_NODE pTraversalDevNodePtr;
	PAUTO_CAP_DEV_NODE pTmpDevNodePtr;
	pTraversalDevNodePtr = pAutoCapDevList->pListHead;

	while(pTraversalDevNodePtr != NULL){
		pTmpDevNodePtr = pTraversalDevNodePtr->pNext;
		PHWIoScopeAutoCapDevListMgr_DeleteAutoCapDevNodeFromList(pAutoCapDevList, pTraversalDevNodePtr);
		pTraversalDevNodePtr = pTmpDevNodePtr;
	}
}

VOID PHWIoScopeAutoCapDevListMgr_InsertAutoCapDevNodeIntoList(PAUTO_CAP_DEV_LIST pAutoCapDevList, PAUTO_CAP_DEV_NODE pAutoCapDevNode){
	KIRQL oldIrql;
	
	KeAcquireSpinLock(&pAutoCapDevList->autoCapDevListSpinLock, &oldIrql);

	if(pAutoCapDevList->numOfAutoCapDevNodes == 0){
		pAutoCapDevList->pListHead = pAutoCapDevNode;
		pAutoCapDevList->pListTail = pAutoCapDevNode;
	}else{
		pAutoCapDevList->pListTail->pNext = pAutoCapDevNode;
		pAutoCapDevNode->pPrev = pAutoCapDevList->pListTail;
		pAutoCapDevList->pListTail = pAutoCapDevNode;
	}
	
	pAutoCapDevList->numOfAutoCapDevNodes++;
	KeReleaseSpinLock(&pAutoCapDevList->autoCapDevListSpinLock, oldIrql);
}

PAUTO_CAP_DEV_NODE PHWIoScopeAutoCapDevListMgr_CreateAutoCapDevNode(PWCHAR pHardwareID, const unsigned int hardwareIDSize){
	PAUTO_CAP_DEV_NODE pNewAutoCapDevNode;

	pNewAutoCapDevNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(AUTO_CAP_DEV_NODE), PHW_POOL_TAG);

	if(pNewAutoCapDevNode == NULL){
		return NULL;
	}

	pNewAutoCapDevNode->pHardwareID = ExAllocatePoolWithTag(NonPagedPool, hardwareIDSize, PHW_POOL_TAG);

	if(pNewAutoCapDevNode->pHardwareID == NULL){
		return NULL;
	}
	
	RtlCopyMemory(pNewAutoCapDevNode->pHardwareID, pHardwareID, hardwareIDSize);
	pNewAutoCapDevNode->pNext = NULL;
	pNewAutoCapDevNode->pPrev = NULL;

	return pNewAutoCapDevNode;
}
VOID PHWIoScopeAutoCapDevListMgr_DeleteAutoCapDevNodeFromList(PAUTO_CAP_DEV_LIST pAutoCapDevList, PAUTO_CAP_DEV_NODE pAutoCapDevNode){
	PAUTO_CAP_DEV_NODE pPrevNode;
	PAUTO_CAP_DEV_NODE pNextNode;
	KIRQL oldIrql;

	if(pAutoCapDevList == NULL){
		return;
	}

	KeAcquireSpinLock(&pAutoCapDevList->autoCapDevListSpinLock, &oldIrql);

	pPrevNode = pAutoCapDevNode->pPrev;
	pNextNode = pAutoCapDevNode->pNext;

	if(pAutoCapDevNode == pAutoCapDevList->pListHead){
		pAutoCapDevList->pListHead = pNextNode;
	}
	
	if(pAutoCapDevNode == pAutoCapDevList->pListTail){
		pAutoCapDevList->pListTail = pPrevNode;
	}

	if(pPrevNode != NULL){
		if(pNextNode != NULL){
			pPrevNode->pNext = pNextNode;
			pNextNode->pPrev = pPrevNode;
		}else{
			pPrevNode->pNext = NULL;
		}
	}else{
		if(pNextNode != NULL){
			pNextNode->pPrev = NULL;
		}
	}

	if(pAutoCapDevNode->pHardwareID != NULL){
		ExFreePool(pAutoCapDevNode->pHardwareID);
		pAutoCapDevNode->pHardwareID = NULL;
	}

	if(pAutoCapDevNode != NULL){
		ExFreePool(pAutoCapDevNode);
		pAutoCapDevNode = NULL;
	}
	
	pAutoCapDevList->numOfAutoCapDevNodes--;
	
	KeReleaseSpinLock(&pAutoCapDevList->autoCapDevListSpinLock, oldIrql);
}

PAUTO_CAP_DEV_NODE PHWIoScopeAutoCapDevListMgr_FindSpecificAutoCapDevNodeFromList(PAUTO_CAP_DEV_LIST pAutoCapDevList, PWCHAR pHardwareID){
	PAUTO_CAP_DEV_NODE pTraversalAutoCapDevNodePtr;

	pTraversalAutoCapDevNodePtr = pAutoCapDevList->pListHead;

	while(pTraversalAutoCapDevNodePtr != NULL){
		if(!wcscmp(pTraversalAutoCapDevNodePtr->pHardwareID, pHardwareID)){
			return pTraversalAutoCapDevNodePtr;
		}
		pTraversalAutoCapDevNodePtr = pTraversalAutoCapDevNodePtr->pNext;
	}
	
	return NULL;
}




