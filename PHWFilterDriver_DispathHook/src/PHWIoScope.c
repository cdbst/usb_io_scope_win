#include "PHWIoScope.h"
#include "PHWIoScopePnP.h"
#include "PHWIoScopePwr.h"
#include "PHWIoScopeIoctlMgr.h"
#include "PHWIoScopePktMgr.h"


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

	return STATUS_SUCCESS;
}


NTSTATUS PHWIoScope_AddDevice(PDRIVER_OBJECT drvObj, PDEVICE_OBJECT pUnderlyingPsclDevObj){

	NTSTATUS status;
	PDEVICE_OBJECT pSelfFilterDevObj;
	PDEVICE_EXTENSION pFilterDevObjExt;
	ULONG deviceType;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pSelfFilterDevObj = NULL;
	pFilterDevObjExt = NULL;
	deviceType = FILE_DEVICE_UNKNOWN;

	if (!NT_SUCCESS(status)){
        IoDeleteDevice(drvObj->DeviceObject);
        return status;
    }

	if(RtlIsNtDdiVersionAvailable(NTDDI_WINXP)){
        pSelfFilterDevObj = IoGetAttachedDeviceReference(pUnderlyingPsclDevObj);
        deviceType = pSelfFilterDevObj->DeviceType;
        ObDereferenceObject(pSelfFilterDevObj);
	}

    status = IoCreateDevice (
                drvObj,
                sizeof (DEVICE_EXTENSION),
                NULL,
                deviceType,
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

	pFilterDevObjExt->eventNotifyCounter = 0;
	pFilterDevObjExt->commonDevData.devObjType = TYPE_FIDO;

	IoInitializeRemoveLock(&pFilterDevObjExt->removeLock, PHW_POOL_TAG, (ULONG) 1, (ULONG) 100);

	INITIALIZE_DEVICE_STATE(pFilterDevObjExt);

	pSelfFilterDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

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

        IoSetCompletionRoutine(pIrp, 
                       PHWIoScope_IrpCompletionRoutine, 
                       NULL, 
                       TRUE, 
                       TRUE, 
                       TRUE);
		
		if(globals.isCaptureMode == 1){
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

			IoSkipCurrentIrpStackLocation(pIrp);
			status = IoCallDriver(pDevExt->pNextLowerDevObj, pIrp);
			IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);
			return status;
		}

	}else{
		return PHWIOScopeCtlDevDispatchRoutine(pDevObj, pIrp);
	}

}

NTSTATUS PHWIoScope_IrpCompletionRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp, PVOID pContext){
	if(pIrp->PendingReturned){
		IoMarkIrpPending(pIrp);
	}
	
	if(globals.isCaptureMode == 1){
		PHWIoScopeCapturePkt(pDevObj, pIrp, 1);
	}

	return STATUS_SUCCESS;
}



VOID PHWIoScope_DriverUnload(PDRIVER_OBJECT pDrvObj){
	PAGED_CODE();

	UNREFERENCED_PARAMETER(pDrvObj);

	if(globals.RegistryPath.Buffer){
		ExFreePool(globals.RegistryPath.Buffer);
	}

	return;
}

