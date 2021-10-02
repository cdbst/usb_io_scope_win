#include "PHWIoScopeFltDevListMgr.h"
#include "PHWIoScope.h"

VOID PHWIoScopeFltDevListMgr_InitList(PFLT_DEVOBJ_LIST pFltDevObjList){
	KeInitializeSpinLock(&pFltDevObjList->fltDevObjListSpinLock);
	pFltDevObjList->pListHead = NULL;
	pFltDevObjList->pListTail = NULL;
	pFltDevObjList->numOfFltDevObj = 0;
}
VOID PHWIoScopeFltDevListMgr_ReleaseList(PFLT_DEVOBJ_LIST pFltDevObjList){
	PFLT_DEVOBJ_NODE pTraversalDevNodePtr;
	PFLT_DEVOBJ_NODE pTmpDevNodePtr;
	pTraversalDevNodePtr = pFltDevObjList->pListHead;

	while(pTraversalDevNodePtr != NULL){
		pTmpDevNodePtr = pTraversalDevNodePtr->pNext;
		PHWIoScopeFltDevListMgr_DeleteFltDevNodeFromList(pFltDevObjList, pTraversalDevNodePtr);
		pTraversalDevNodePtr = pTmpDevNodePtr;
	}
}

VOID PHWIoScopeFltDevListMgr_InsertFltDevNodeIntoList(PFLT_DEVOBJ_LIST pFltDevObjList, PFLT_DEVOBJ_NODE pFltDevObjNode, LIST_INSERT_POS insertPos){
	KIRQL oldIrql;

	KeAcquireSpinLock(&pFltDevObjList->fltDevObjListSpinLock, &oldIrql);

	if(pFltDevObjList->numOfFltDevObj == 0){
		pFltDevObjList->pListHead = pFltDevObjNode;
		pFltDevObjList->pListTail = pFltDevObjNode;
	}else if(insertPos == LIST_INSERT_POS_HEAD){
		pFltDevObjList->pListHead->pPrev = pFltDevObjNode;
		pFltDevObjNode->pNext = pFltDevObjList->pListHead;
		pFltDevObjList->pListHead = pFltDevObjNode;
	}else if(insertPos == LIST_INSERT_POS_TAIL){
		pFltDevObjList->pListTail->pNext = pFltDevObjNode;
		pFltDevObjNode->pPrev = pFltDevObjList->pListTail;
		pFltDevObjList->pListTail = pFltDevObjNode;
	}

	pFltDevObjList->numOfFltDevObj++;

	KeReleaseSpinLock(&pFltDevObjList->fltDevObjListSpinLock, oldIrql);
}

PFLT_DEVOBJ_NODE PHWIoScopeFltDevListMgr_CreateFltDevNode(PDEVICE_OBJECT pFltDevObj){
	PFLT_DEVOBJ_NODE pNewFltDevObj;

	pNewFltDevObj = ExAllocatePoolWithTag(NonPagedPool, sizeof(FLT_DEVOBJ_NODE), PHW_POOL_TAG);

	if(pNewFltDevObj == NULL){
		return NULL;
	}

	pNewFltDevObj->pFltDevObj = pFltDevObj;
	pNewFltDevObj->pNext = NULL;
	pNewFltDevObj->pPrev = NULL;

	return pNewFltDevObj;
}

VOID PHWIoScopeFltDevListMgr_DeleteFltDevNodeFromList(PFLT_DEVOBJ_LIST pFltDevObjList, PFLT_DEVOBJ_NODE pFltDevObjNode){
	PFLT_DEVOBJ_NODE pPrevNode;
	PFLT_DEVOBJ_NODE pNextNode;
	KIRQL oldIrql;

	if(pFltDevObjNode == NULL){
		return;
	}

	KeAcquireSpinLock(&pFltDevObjList->fltDevObjListSpinLock, &oldIrql);

	pPrevNode = pFltDevObjNode->pPrev;
	pNextNode = pFltDevObjNode->pNext;

	if(pFltDevObjNode == pFltDevObjList->pListHead){
		pFltDevObjList->pListHead = pNextNode;
	}
	
	if(pFltDevObjNode == pFltDevObjList->pListTail){
		pFltDevObjList->pListTail = pPrevNode;
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
	
	ExFreePool(pFltDevObjNode);
	pFltDevObjNode = NULL;
	
	pFltDevObjList->numOfFltDevObj--;
	
	KeReleaseSpinLock(&pFltDevObjList->fltDevObjListSpinLock, oldIrql);

}

PFLT_DEVOBJ_NODE PHWIoScopeFltDevListMgr_FindSpecificDevNodeFromList(PFLT_DEVOBJ_LIST pFltDevObjList, PVOID pFindInfo, DEV_NODE_FIND_METHOD findMethod){
	PFLT_DEVOBJ_NODE pTraversalDevNodePtr;
	PDEVICE_OBJECT targetDevNodeFidoAddr;
	PWCHAR targetDevNodePdoName;
	PDEVICE_EXTENSION pTraversalDevNodeFidoExt;

	pTraversalDevNodePtr = pFltDevObjList->pListHead;
	targetDevNodeFidoAddr = NULL;
	targetDevNodePdoName = NULL;
	pTraversalDevNodeFidoExt = NULL;

	if(findMethod == DEV_NODE_FIND_METHOD_PDO_NAME){
		targetDevNodePdoName = (PWCHAR) pFindInfo;

		while(pTraversalDevNodePtr != NULL){
			
			pTraversalDevNodeFidoExt = (PDEVICE_EXTENSION)pTraversalDevNodePtr->pFltDevObj->DeviceExtension;
			
			if(!wcscmp(pTraversalDevNodeFidoExt->pPdoName, targetDevNodePdoName)){
				return pTraversalDevNodePtr;
			}

			pTraversalDevNodePtr = pTraversalDevNodePtr->pNext;
		}
		
	}else if(findMethod == DEV_NODE_FIND_METHOD_FIDO_ADDR){
		targetDevNodeFidoAddr = (PDEVICE_OBJECT) pFindInfo;
			
		while(pTraversalDevNodePtr != NULL){
			
			if(pTraversalDevNodePtr->pFltDevObj == targetDevNodeFidoAddr){
				return pTraversalDevNodePtr;
			}
			
			pTraversalDevNodePtr = pTraversalDevNodePtr->pNext;
		}
	}

	return NULL;
}

VOID PHWIoScopeFltDevListMgr_GetPdoNameListCaptureModeActivatedDev(PFLT_DEVOBJ_LIST pFltDevObjList, PCAP_SPECIFIC_DEV_INFO pCapSpecificDevInfo){
	PFLT_DEVOBJ_NODE pTraversalDevNodePtr;
	PDEVICE_EXTENSION pTraversalDevNodeFidoExt;
	unsigned int specificDevPdoNameListCurrentIdx;
	unsigned int pdoNameLen;
		
	pTraversalDevNodePtr = pFltDevObjList->pListHead;
	
	pCapSpecificDevInfo->numOfSpecificCapDev = 0;
	specificDevPdoNameListCurrentIdx = 0;
	pdoNameLen = 0;

	while(pTraversalDevNodePtr != NULL){
		pTraversalDevNodeFidoExt = (PDEVICE_EXTENSION)pTraversalDevNodePtr->pFltDevObj->DeviceExtension;
		
		if(pTraversalDevNodeFidoExt->isCapturePacket){
			pdoNameLen = wcslen(pTraversalDevNodeFidoExt->pPdoName);
			
			RtlCopyMemory(pCapSpecificDevInfo->specificDevPdoNameList + specificDevPdoNameListCurrentIdx,
				pTraversalDevNodeFidoExt->pPdoName,
				sizeof(WCHAR) * pdoNameLen);
			
			specificDevPdoNameListCurrentIdx += pdoNameLen;
			pCapSpecificDevInfo->header[pCapSpecificDevInfo->numOfSpecificCapDev++] = pdoNameLen;
		}
		
		pTraversalDevNodePtr = pTraversalDevNodePtr->pNext;
	}	
}


