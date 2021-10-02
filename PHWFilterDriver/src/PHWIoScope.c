#include "PHWIoScope.h"
#include "PHWIoScopePnP.h"
#include "PHWIoScopePwr.h"
#include "PHWIoScopeIoctlMgr.h"
#include "PHWIoScopePktMgr.h"
#include "PHWIoScopeFltDevListMgr.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, PHWIoScope_AddDevice)
#pragma alloc_text (PAGE, PHWIoScope_DriverUnload)
#endif

GLOBALS globals;

NTSTATUS DriverEntry(PDRIVER_OBJECT drvObj, PUNICODE_STRING registryPath){

	ULONG i;
	PDRIVER_DISPATCH* dispatchRoutine;

	globals.RegistryPath.MaximumLength = registryPath->Length + sizeof(UNICODE_NULL);
	globals.RegistryPath.Length = registryPath->Length;
	globals.RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool, globals.RegistryPath.MaximumLength, PHW_POOL_TAG);

	if(globals.RegistryPath.Buffer){
		RtlCopyUnicodeString(&globals.RegistryPath, registryPath);
	}else{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	for(i = 0, dispatchRoutine = drvObj->MajorFunction; i <= IRP_MJ_MAXIMUM_FUNCTION; i++, dispatchRoutine++){
		*dispatchRoutine = PHWIoScope_DispatchRoutine;
	}

	drvObj->DriverExtension->AddDevice = PHWIoScope_AddDevice;
	drvObj->DriverUnload = PHWIoScope_DriverUnload;

	KeInitializeEvent(&globals.ioctlLock, SynchronizationEvent, TRUE);
	globals.ctlDevObjCount = 0;
	
	ExInitializeFastMutex(&globals.pktQueue.mutex);
	globals.pktQueue.front = 0;
	globals.pktQueue.rear = 0;
	globals.pktQueue.remained_pkt = 0;

	PHWIoScopeFltDevListMgr_InitList(&globals.fltDevObjList);
	PHWIoScopeAutoCapDevListMgr_InitList(&globals.autoCapDevList);

	return STATUS_SUCCESS;
}


NTSTATUS PHWIoScope_AddDevice(PDRIVER_OBJECT drvObj, PDEVICE_OBJECT pUnderlyingPsclDevObj){

	NTSTATUS status;
	PDEVICE_OBJECT pSelfFilterDevObj;
	PDEVICE_EXTENSION pFilterDevObjExt;
	WCHAR pDvicePropertyBuffer[MAX_LEN_DEVICE_PROPERTY_BUFFER];
	ULONG realLength;
	PFLT_DEVOBJ_NODE pFltDevObjNode;
	PAUTO_CAP_DEV_NODE pAutoCapDevNode;
		
	PAGED_CODE();

	status = STATUS_SUCCESS;
	pSelfFilterDevObj = NULL;
	pFilterDevObjExt = NULL;
	realLength = 0;

	if (!NT_SUCCESS(status)){
        IoDeleteDevice(drvObj->DeviceObject);
        return status;
    }

	if(RtlIsNtDdiVersionAvailable(NTDDI_WINXP)){
        pSelfFilterDevObj = IoGetAttachedDeviceReference(pUnderlyingPsclDevObj);
        ObDereferenceObject(pSelfFilterDevObj);
	}

    status = IoCreateDevice (
                drvObj,
                sizeof (DEVICE_EXTENSION),
                NULL,
                FILE_DEVICE_UNKNOWN,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                &pSelfFilterDevObj);

	if(!NT_SUCCESS(status)){
		return status;
	}

	pFilterDevObjExt = (PDEVICE_EXTENSION) pSelfFilterDevObj->DeviceExtension;

	pFilterDevObjExt->pNextLowerDevObj = IoAttachDeviceToDeviceStack(pSelfFilterDevObj, pUnderlyingPsclDevObj);

	if(pFilterDevObjExt->pNextLowerDevObj == NULL){
		IoDeleteDevice(pSelfFilterDevObj);
		return STATUS_UNSUCCESSFUL;
	}

	status = IoRegisterDeviceInterface (
		pUnderlyingPsclDevObj,
		(LPGUID)&GUID_PHWFLT_INTERFACE,
		NULL,
		&pFilterDevObjExt->interfaceName);

	if(!NT_SUCCESS(status)){
		IoDeleteDevice(pSelfFilterDevObj);
		return status;
	}

	pSelfFilterDevObj->Flags |= pFilterDevObjExt->pNextLowerDevObj->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
	
	pSelfFilterDevObj->DeviceType = pFilterDevObjExt->pNextLowerDevObj->DeviceType;
   	pSelfFilterDevObj->Characteristics = pFilterDevObjExt->pNextLowerDevObj->Characteristics;

	pFilterDevObjExt->pSelfFilterDevObj = pSelfFilterDevObj;
	pFilterDevObjExt->pUnderlyingPsclDevObj = pUnderlyingPsclDevObj;
	pFilterDevObjExt->pTopDevObj = NULL;

	pFilterDevObjExt->eventNotifyCounter = 0;
	pFilterDevObjExt->commonDevData.devObjType = TYPE_FIDO;
	
	pFilterDevObjExt->isCapturePacket = 0;
	
	IoInitializeRemoveLock(&pFilterDevObjExt->removeLock, PHW_POOL_TAG, (ULONG) 1, (ULONG) 100);

	INITIALIZE_DEVICE_STATE(pFilterDevObjExt);

	pSelfFilterDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
	
	status = IoGetDeviceProperty(
		pUnderlyingPsclDevObj,
		DevicePropertyPhysicalDeviceObjectName,
		sizeof(pDvicePropertyBuffer),
		pDvicePropertyBuffer,
		&realLength);

	if(NT_SUCCESS(status)){
		pFilterDevObjExt->pPdoName = ExAllocatePoolWithTag(NonPagedPool, realLength, PHW_POOL_TAG);
		RtlCopyMemory(pFilterDevObjExt->pPdoName, pDvicePropertyBuffer, realLength);
	}

	realLength = 0;
	RtlZeroMemory(pDvicePropertyBuffer, sizeof(pDvicePropertyBuffer));
		
	status = IoGetDeviceProperty(
		pUnderlyingPsclDevObj,
		DevicePropertyHardwareID,
		sizeof(pDvicePropertyBuffer),
		pDvicePropertyBuffer,
		&realLength);

	if(NT_SUCCESS(status)){
		pFilterDevObjExt->pHardwareID = ExAllocatePoolWithTag(NonPagedPool, realLength, PHW_POOL_TAG);
		RtlCopyMemory(pFilterDevObjExt->pHardwareID, pDvicePropertyBuffer, realLength);
		pFilterDevObjExt->hardwareIDSize = realLength;
	}

	KdPrint(("pdo_name : %ws, hardwareID : %ws", pFilterDevObjExt->pPdoName, pFilterDevObjExt->pHardwareID));
	
	pAutoCapDevNode = PHWIoScopeAutoCapDevListMgr_FindSpecificAutoCapDevNodeFromList(&globals.autoCapDevList, pFilterDevObjExt->pHardwareID);
		
	if(pAutoCapDevNode != NULL){
		pFilterDevObjExt->isCapturePacket = 1;
		PHWIoScopeAutoCapDevListMgr_DeleteAutoCapDevNodeFromList(&globals.autoCapDevList, pAutoCapDevNode);
	}

	pFltDevObjNode = PHWIoScopeFltDevListMgr_CreateFltDevNode(pSelfFilterDevObj);
	
	PHWIoScopeFltDevListMgr_InsertFltDevNodeIntoList(&globals.fltDevObjList, pFltDevObjNode, LIST_INSERT_POS_TAIL);
	
	return STATUS_SUCCESS;

}


NTSTATUS PHWIoScope_DispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp){

	PCOMMON_DEVICE_DATA pCommonDevData;
	PIO_STACK_LOCATION pIoStackLocation;
	NTSTATUS status;

	pCommonDevData = (PCOMMON_DEVICE_DATA)pDevObj->DeviceExtension;
	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	status = STATUS_SUCCESS;
		
	ASSERT(pCommonDevData->devObjType != TYPE_UMKNOWN);
	
	if(pCommonDevData->devObjType != TYPE_CTLDO){
		PDEVICE_EXTENSION pDevExt;
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		
		if(pDevExt->currentDevPnPState != PNP_STATE_REMOVE   && globals.isCaptureMode == 1 && pDevExt->isCapturePacket){
			PHWIoScopeCapturePkt(pDevObj, pIrp, 0);
		}
		
		switch(pIoStackLocation->MajorFunction){
		case IRP_MJ_PNP:
			status = PHWIoScope_DispatchPnP(pDevObj, pIrp);
			return status;
		case IRP_MJ_POWER:
			status = PHWIoScope_DispatchPwr(pDevObj, pIrp);
			return status;
		default:
			status = IoAcquireRemoveLock(&pDevExt->removeLock, pIrp);

			if (!NT_SUCCESS (status)){
				pIrp->IoStatus.Status = status;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				return status;
			}
			
			IoCopyCurrentIrpStackLocationToNext(pIrp);
			
			IoSetCompletionRoutine(pIrp,
				PHWIoScope_IrpCompletionRoutine,
				NULL,
				TRUE,
				TRUE,
				TRUE);
			
			return IoCallDriver(pDevExt->pNextLowerDevObj, pIrp);
		}

	}else{
	
		return PHWIOScopeCtlDevDispatchRoutine(pDevObj, pIrp);
	}

}

NTSTATUS PHWIoScope_IrpCompletionRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp, PVOID pContext){
	PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;

	//KdPrint(("PHWIoScope_IrpCompletionRoutine) Completion Routine Called\n"));
	
	if(pIrp->PendingReturned){
		IoMarkIrpPending(pIrp);
	}
	
	if(pDevExt->currentDevPnPState != PNP_STATE_REMOVE && globals.isCaptureMode == 1 && pDevExt->isCapturePacket){
		PHWIoScopeCapturePkt(pDevObj, pIrp, 1);
	}

	IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);
	return STATUS_SUCCESS;
}

VOID PHWIoScope_DriverUnload(PDRIVER_OBJECT pDrvObj){
	PAGED_CODE();

	UNREFERENCED_PARAMETER(pDrvObj);

	if(globals.RegistryPath.Buffer){
		ExFreePool(globals.RegistryPath.Buffer);
	}

	PHWIoScopeFltDevListMgr_ReleaseList(&globals.fltDevObjList);
	PHWIoScopeAutoCapDevListMgr_ReleaseList(&globals.autoCapDevList);
	return;
}

