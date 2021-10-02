#include "PHWIoScopePktMgr.h"
#include "PHWIoScopeURBMgr.h"

void PHWIoScopeCapturePkt(PDEVICE_OBJECT pDevObj, PIRP pIrp, unsigned char pktDirection){

	PQUEUEING_PKT_CONTEXT pQueueingPktContext;
	UNREFERENCED_PARAMETER(pIrp);


	pQueueingPktContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(QUEUEING_PKT_CONTEXT), PHW_POOL_TAG);

	if(pQueueingPktContext == NULL){
		KdPrint(("PHWIoScope) QueueingPktInfo - allocation mem1 is failed\n"));
		return;			
	}

	pQueueingPktContext->pPktInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(PKT_INFO), PHW_POOL_TAG);

	if(pQueueingPktContext->pPktInfo == NULL){
		ExFreePool(pQueueingPktContext);
		KdPrint(("PHWIoScope) QueueingPktInfo - allocation mem2 is failed\n"));
		return;		
	}
	PHWIoScopeExpelMeaningfulData(pDevObj, pIrp, pQueueingPktContext->pPktInfo, pktDirection);
	
	pQueueingPktContext->pIoWorkItem = IoAllocateWorkItem(pDevObj);

	if(pQueueingPktContext->pIoWorkItem == NULL){
		ExFreePool(pQueueingPktContext->pPktInfo);
		ExFreePool(pQueueingPktContext);
		KdPrint(("PHWIoScope) QueueingPktInfo - allocation work_item is failed\n"));
		return;	
	}

	IoQueueWorkItem(pQueueingPktContext->pIoWorkItem, PHWIoScopeQueueingPkt, DelayedWorkQueue, pQueueingPktContext);
}

void PHWIoScopeQueueingPkt(PDEVICE_OBJECT pDevObj, PVOID pContext){
	PQUEUEING_PKT_CONTEXT pQueueingPktContext;

	UNREFERENCED_PARAMETER(pDevObj);
	pQueueingPktContext = (PQUEUEING_PKT_CONTEXT)pContext;
	

	if(PHWIoScopePktCheckPktQueueOverflow()){
		ExAcquireFastMutex(&globals.pktQueue.mutex);
		RtlCopyMemory(&globals.pktQueue.pktInfo[globals.pktQueue.rear],
				pQueueingPktContext->pPktInfo,
				sizeof(PKT_INFO));
		globals.pktQueue.rear = (globals.pktQueue.rear + 1) % MAX_SIZE_PKT_QUEUE;
		globals.pktQueue.remained_pkt++;
		ExReleaseFastMutex(&globals.pktQueue.mutex);
		//KdPrint(("PHWIoScope) Pkt Enqueue %s, %d\n", globals.pktQueue.storage[globals.pktQueue.rear].data, globals.pktQueue.rear));
		
	}else{
		KdPrint(("PHWIoScope) PktQueue is full - %d", globals.pktQueue.remained_pkt));
	}
	
	IoFreeWorkItem(pQueueingPktContext->pIoWorkItem);
	ExFreePool(pQueueingPktContext->pPktInfo);
	ExFreePool(pQueueingPktContext);
}

void PHWIoScopeDequeueingPkt(PDEQD_PKT_INFO pDeqdPktInfo){
	USHORT i;

	if(globals.pktQueue.remained_pkt >= MAX_TRANSFER_AT_ONCE){
		pDeqdPktInfo->numOfPkt = MAX_TRANSFER_AT_ONCE;
	}else{
		pDeqdPktInfo->numOfPkt = globals.pktQueue.remained_pkt;
	}

	for(i = 0; i < pDeqdPktInfo->numOfPkt; i++){
		RtlCopyMemory(&pDeqdPktInfo->pktInfo[i],
				&globals.pktQueue.pktInfo[globals.pktQueue.front],
				sizeof(PKT_INFO));
		
		RtlZeroMemory(&globals.pktQueue.pktInfo[globals.pktQueue.front],
				sizeof(PKT_INFO));
		
		globals.pktQueue.front = (globals.pktQueue.front + 1) % MAX_SIZE_PKT_QUEUE;
		globals.pktQueue.remained_pkt--;
	}
}

int PHWIoScopePktCheckPktQueueOverflow(){
	if( (globals.pktQueue.rear + 1) % MAX_SIZE_PKT_QUEUE == globals.pktQueue.front){
		return 0;
	}
	return 1;
}

int PHWIoScopePktCheckPktQueueUnderflow(){
	if(globals.pktQueue.front % MAX_SIZE_PKT_QUEUE == globals.pktQueue.rear){
		return 0;
	}
	return 1;	
}

void PHWIoScopeExpelMeaningfulData(PDEVICE_OBJECT pDevObj, PIRP pIrp, PPKT_INFO pDestination, unsigned char pktDirection){
	PIO_STACK_LOCATION pIoStackLocation;

	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	pDestination->devObjInfo = (ULONG_PTR)pDevObj;
	pDestination->devObjInfo = (ULONG_PTR)pIrp;
	pDestination->direction = pktDirection;
	pDestination->irpMajorFunction = pIoStackLocation->MajorFunction;
	pDestination->irpMinorFunction = pIoStackLocation->MinorFunction;

	if(pIrp->AssociatedIrp.SystemBuffer != NULL){
		if(pktDirection == 0)
			PHWIoScopeExpelBufferData(pIrp, pDestination, pIrp->AssociatedIrp.SystemBuffer);
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Expel data from SYSTEM BUFF (devobj:0x%x) - MJFC is %d\n", pktDirection, pDevObj, pIoStackLocation->MajorFunction));
	}else if(pIrp->MdlAddress != NULL){
		if(pktDirection == 0)
			PHWIoScopeExpelBufferData(pIrp, pDestination, MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority));
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Expel data from MDL (devobj:0x%x) - MJFC is %d\n", pktDirection, pDevObj, pIoStackLocation->MajorFunction));
	}else if(pIrp->UserBuffer != NULL){
		if(pktDirection == 0)
			PHWIoScopeExpelBufferData(pIrp, pDestination, pIrp->UserBuffer);
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Expel data from USER BUFF (devobj:0x%x) - MJFC is %d\n", pktDirection, pDevObj, pIoStackLocation->MajorFunction));
	}else if( (pIoStackLocation->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
				pIoStackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
				pIoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer != NULL){
		if(pktDirection == 0)		
			PHWIoScopeExpelBufferData(pIrp, pDestination, pIoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer);
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Expel data from Type3InputBuffer (devobj:0x%x) - MJFC is %d\n", pktDirection, pDevObj, pIoStackLocation->MajorFunction));
	}else if((pIoStackLocation->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
				pIoStackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
				pIoStackLocation->Parameters.Others.Argument1 != NULL){
		PHWIoScopeExpelURBData((PURB)pIoStackLocation->Parameters.Others.Argument1, pDestination);
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Expel data from URB - [urbfunc:%d, devobj:%d]\n", pktDirection, pDestination->dataMinorType, pDevObj));
	}
	else{
		KdPrint(("PHWIoScopeExpelMeaningfulData)[%d] Cannot Expel Meaningful Data (devobj:0x%x) - MJFC is %d\n", pktDirection, pDevObj, pIoStackLocation->MajorFunction));
		pDestination->dataSize = 0;
		return;
	}
		
}

void PHWIoScopeExpelBufferData(PIRP pIrp, PPKT_INFO pDestination, PVOID pBuffer){
	PIO_STACK_LOCATION pIoStackLocation;
	
	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	switch(pIoStackLocation->MajorFunction){
	case IRP_MJ_WRITE:
		pDestination->dataSize = pIoStackLocation->Parameters.Write.Length;
		break;
	case IRP_MJ_READ:
		pDestination->dataSize  = pIoStackLocation->Parameters.Read.Length;
		break;
	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
	case IRP_MJ_DEVICE_CONTROL:
		if(pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength > pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength){
			pDestination->dataSize = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
		}else{
			pDestination->dataSize = pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
		}
		break;
	default:
		KdPrint(("PHWIoScopeExpelMeaningfulData) Invalid IRP, So cannot choose data length  - irp : %d,%d\n", pIoStackLocation->MajorFunction, pIoStackLocation->MinorFunction));
		return;
	}
	
	RtlCopyMemory(pDestination->data, pBuffer, pDestination->dataSize);

}



