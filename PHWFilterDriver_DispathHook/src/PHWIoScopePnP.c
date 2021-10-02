#include "PHWIoScope.h"
#include "PHWIoScopePnP.h"
#include "PHWIoScopeIoctlMgr.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PHWIoScope_DispatchPnP)
#pragma alloc_text (PAGE, PHWIoScope_PnPStartDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPQueryStopDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPStopDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPCancelStopDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPQueryRemoveDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPRemoveDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPSurpriseRemoveDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPCancelRemoveDevice)
#pragma alloc_text (PAGE, PHWIoScope_PnPDeviceUsageNotification)
#endif

NTSTATUS PHWIoScope_DispatchPnP(PDEVICE_OBJECT pDevObj, PIRP pIrp){

	PDEVICE_EXTENSION pDevExt;
	PIO_STACK_LOCATION pIoStackLocation;
	NTSTATUS status;

	PAGED_CODE();

	pDevExt = (PDEVICE_EXTENSION) pDevObj->DeviceExtension;
	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	status = IoAcquireRemoveLock (&pDevExt->removeLock, pIrp);

    if(!NT_SUCCESS(status)){
        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return status;
    }

	switch(pIoStackLocation->MinorFunction){
	case IRP_MN_START_DEVICE:

		status = PHWIoScope_PnPStartDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_QUERY_STOP_DEVICE:

		status = PHWIoScope_PnPQueryStopDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_STOP_DEVICE:

		status = PHWIoScope_PnPQueryStopDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_CANCEL_STOP_DEVICE:

		status = PHWIoScope_PnPCancelStopDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_QUERY_REMOVE_DEVICE:

		status = PHWIoScope_PnPQueryRemoveDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_REMOVE_DEVICE:

		IoReleaseRemoveLockAndWait(&pDevExt->removeLock, pIrp);
		status = PHWIoScope_PnPRemoveDevice(pDevObj, pIrp, pIoStackLocation);

		return status;
	case IRP_MN_CANCEL_REMOVE_DEVICE:

		status = PHWIoScope_PnPCancelRemoveDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_SURPRISE_REMOVAL:

		status = PHWIoScope_PnPSurpriseRemoveDevice(pDevObj, pIrp, pIoStackLocation);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:

		status = PHWIoScope_PnPDeviceUsageNotification(pDevObj, pIrp, pIoStackLocation);
		//RemoveLock will be released by completionRoutine

		return status;
	//case IRP_MN_QUERY_CAPABILITIES:
	default:
		IoSkipCurrentIrpStackLocation (pIrp);
	    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);
		IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

		return status;
	}

}

NTSTATUS PHWIoScope_PnPStartDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){
	KEVENT eventObj;
	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pIoStackLocation);

	KeInitializeEvent(&eventObj, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp,
		PHWIoScope_PnPCompleteDeviceStart,
		&eventObj,
		TRUE,
		TRUE,
		TRUE);

	status = IoCallDriver(pDevExt->pNextLowerDevObj, pIrp);

	if(status == STATUS_PENDING){

       KeWaitForSingleObject(&eventObj, Executive, KernelMode, FALSE, NULL);
       status = pIrp->IoStatus.Status;
	}

	if(NT_SUCCESS(status)){

	    status = IoSetDeviceInterfaceState(&pDevExt->interfaceName, TRUE);

	    if (!NT_SUCCESS (status)){
			KdPrint(("PnP Start - Device Interface State Enable Fail 0x%x", status));
			return status;
		}

		CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_START);

        if(pDevExt->pNextLowerDevObj->Characteristics & FILE_REMOVABLE_MEDIA) {
            pDevObj->Characteristics |= FILE_REMOVABLE_MEDIA;
        }

		if(pDevExt->previousDevPnPState != PNP_STATE_STOP){
			PHWIoScopeCreateCtlDevObj(pDevObj);
	    }
	}

    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS PHWIoScope_PnPQueryStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_STOP_PENDING);
	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;
}

NTSTATUS PHWIoScope_PnPStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_STOP);
	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;
}

NTSTATUS PHWIoScope_PnPCancelStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	if(pDevExt->currentDevPnPState == PNP_STATE_STOP_PENDING){
		ROLLBACK_DEVICE_STATE(pDevExt);
	}

	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;
}

NTSTATUS PHWIoScope_PnPQueryRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_REMOVE_PENDING);
	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;

}

NTSTATUS PHWIoScope_PnPRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	status = IoSetDeviceInterfaceState(&pDevExt->interfaceName, FALSE);

	if(!NT_SUCCESS(status)){
		KdPrint(("PnP RemoveDevice - Device Interface State Disable Fail 0x%x", status));
	}	

    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pDevExt->pNextLowerDevObj, pIrp);
    CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_REMOVE);

	PHWIoScopeDeleteCtlDevObj();
	IoDetachDevice(pDevExt->pNextLowerDevObj);
	RtlFreeUnicodeString(&pDevExt->interfaceName);
	IoDeleteDevice(pDevObj);
	
	return status;
}

NTSTATUS PHWIoScope_PnPSurpriseRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	status = IoSetDeviceInterfaceState(&pDevExt->interfaceName, FALSE);

	if(!NT_SUCCESS(status)){
		KdPrint(("PnP SurpriseRemoveDevice - Device Interface State Disable Fail 0x%x", status));
	}	

	CHANGE_DEVICE_STATE(pDevExt, PNP_STATE_REMOVE_SURPRISE_PENDING);
	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;
}

NTSTATUS PHWIoScope_PnPCancelRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	if(pDevExt->currentDevPnPState == PNP_STATE_REMOVE_PENDING){
		ROLLBACK_DEVICE_STATE(pDevExt);
	}

	pIrp->IoStatus.Status = status;
	IoSkipCurrentIrpStackLocation (pIrp);
    status = IoCallDriver (pDevExt->pNextLowerDevObj, pIrp);

	return status;
}


NTSTATUS PHWIoScope_PnPDeviceUsageNotification(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){

	PDEVICE_EXTENSION pDevExt;
	NTSTATUS status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	UNREFERENCED_PARAMETER(pIoStackLocation);

	#pragma prefast(suppress:__WARNING_INACCESSIBLE_MEMBER)
    if ((pDevObj->AttachedDevice == NULL) ||
        (pDevObj->AttachedDevice->Flags & DO_POWER_PAGABLE)) {

        pDevObj->Flags |= DO_POWER_PAGABLE;
    }

    IoCopyCurrentIrpStackLocationToNext(pIrp);

    IoSetCompletionRoutine(
        pIrp,
        PHWIoScope_PnPCompleteDeviceUsageNotification,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    return IoCallDriver(pDevExt->pNextLowerDevObj, pIrp);

}

NTSTATUS PHWIoScope_PnPCompleteDeviceStart(PDEVICE_OBJECT pDevObj, PIRP pIrp, PVOID pContext){
	PKEVENT pEventObj;

	pEventObj = (PKEVENT)pContext;

	UNREFERENCED_PARAMETER(pDevObj);

	if(pIrp->PendingReturned){
		KeSetEvent(pEventObj, IO_NO_INCREMENT, FALSE);
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS PHWIoScope_PnPCompleteDeviceUsageNotification(PDEVICE_OBJECT pDevObj, PIRP pIrp, PVOID pContext){

	PDEVICE_EXTENSION pDevExt;

    UNREFERENCED_PARAMETER(pContext);
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

    if(pIrp->PendingReturned){
        IoMarkIrpPending(pIrp);
    }

    //
    // On the way up, pagable might become clear. Mimic the driver below us.
    //
    if (!(pDevExt->pNextLowerDevObj->Flags & DO_POWER_PAGABLE)){

        pDevObj->Flags &= ~DO_POWER_PAGABLE;
    }

    IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

    return STATUS_SUCCESS;

}

