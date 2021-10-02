#include "PHWIoScope.h"
#include "PHWIoScopeIoctlMgr.h"
#include "PHWIoScopePktMgr.h"
#include "PHWIoScopePublic.h"
#include "PHWIoScopeIoctlCode.h"
#include "PHWIoScopeUtil.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PHWIoScopeCreateCtlDevObj)
#pragma alloc_text (PAGE, PHWIoScopeDeleteCtlDevObj)
#endif

NTSTATUS PHWIoScopeCreateCtlDevObj(PDEVICE_OBJECT           pDevObj){
    UNICODE_STRING ctlDevName;
    UNICODE_STRING symbolicLinkName;
    PCONTROL_DEVICE_EXTENSION pCtlDevExt;
    NTSTATUS status;

    PAGED_CODE();

	status = STATUS_UNSUCCESSFUL;

    //
    // IoCreateDeviceSecure & IoCreateSymbolicLink must be called at
    // PASSIVE_LEVEL.
    // Hence we use an event and not a fast mutex.
    //
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&globals.ioctlLock, Executive, KernelMode, FALSE, NULL);

    //
    // If this is a first instance of the device, then create a controlobject
    // and register dispatch points to handle ioctls.
    //
    if (1 == ++globals.ctlDevObjCount){
        //
        // Initialize the unicode strings
        //
        RtlInitUnicodeString(&ctlDevName, CTL_DEVICE_NAME);
        RtlInitUnicodeString(&symbolicLinkName, DEVICE_SYM_LINK_NAME);

		status = IoCreateDevice(pDevObj->DriverObject,
		        				sizeof(CONTROL_DEVICE_EXTENSION),
		        				&ctlDevName,
		        				FILE_DEVICE_UNKNOWN,
		        				FILE_DEVICE_SECURE_OPEN,
		        				FALSE,
		        				&globals.pCtlDevObj);  

        if(NT_SUCCESS(status)){

            globals.pCtlDevObj->Flags |= DO_BUFFERED_IO;

            status = IoCreateSymbolicLink( &symbolicLinkName, &ctlDevName );

            if (!NT_SUCCESS(status)){
                IoDeleteDevice(globals.pCtlDevObj);
                //KdPrint(("IoCreateSymbolicLink failed %x\n", status));
                goto End;
            }

            pCtlDevExt = (PCONTROL_DEVICE_EXTENSION)globals.pCtlDevObj->DeviceExtension;
            pCtlDevExt->commonDevData.devObjType = TYPE_CTLDO;
            pCtlDevExt->controlData = NULL;
            pCtlDevExt->deleted = FALSE;

            globals.pCtlDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

			//KdPrint(("IoCreate Ctl Device success"));

        }else {
            //KdPrint(("IoCreate Ctl Device failed %x\n", status));
        }
    }

End:

    KeSetEvent(&globals.ioctlLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    return status;

}


VOID PHWIoScopeDeleteCtlDevObj(){
    UNICODE_STRING symbolicLinkName;
    PCONTROL_DEVICE_EXTENSION pCtlDevExt;

    PAGED_CODE();

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&globals.ioctlLock, Executive, KernelMode, FALSE, NULL);

    //
    // If this is the last instance of the device then delete the controlobject
    // and symbolic link to enable the pnp manager to unload the driver.
    //

    if (!(--globals.ctlDevObjCount) && globals.pCtlDevObj)
    {
        RtlInitUnicodeString(&symbolicLinkName, DEVICE_SYM_LINK_NAME);
        pCtlDevExt = (PCONTROL_DEVICE_EXTENSION)globals.pCtlDevObj->DeviceExtension;
        pCtlDevExt->deleted = TRUE;
        IoDeleteSymbolicLink(&symbolicLinkName);
        IoDeleteDevice(globals.pCtlDevObj);
        globals.pCtlDevObj = NULL;
    }

    KeSetEvent(&globals.ioctlLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

	//KdPrint(("IoDelete Ctl Device success"));
}

NTSTATUS PHWIOScopeCtlDevDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp){
	PCONTROL_DEVICE_EXTENSION pCtlDevObj;
	PIO_STACK_LOCATION pIoStackLocation;
	NTSTATUS status;

	pCtlDevObj = (PCONTROL_DEVICE_EXTENSION)pDevObj->DeviceExtension;
	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

	if(!pCtlDevObj->deleted){

		status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = 0;
		pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);

		switch(pIoStackLocation->MajorFunction){
		case IRP_MJ_CREATE:
			break;
		case IRP_MJ_CLOSE:
			break;
		case IRP_MJ_CLEANUP:
			break;
		case IRP_MJ_DEVICE_CONTROL:
			status = PHWIOScopeCtlDevDispatchDeviceControl(pDevObj, pIrp, pIoStackLocation);
			break;
		default:
			break;
        }
    } else {
        ASSERTMSG(FALSE, "Requests being sent to a dead device\n");
        status = STATUS_DEVICE_REMOVED;
    }
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS PHWIOScopeCtlDevDispatchDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation){
	NTSTATUS status;
	PVOID pSystemBuffer;
	ULONG inputBufferLength;
	ULONG outputBufferLength;
	PDEQD_PKT_INFO pPktInfoSystemBuffer;
	
	PCAP_SPECIFIC_DEV_INFO pCapSpecificDevInfo = NULL;
	PWCHAR pTargetPdoName = NULL;
	PFLT_DEVOBJ_NODE targetDevObjNode = NULL;
	ULONG prevIdx;
	ULONG i;

	PFLT_DEVOBJ_NODE traversalNodePtr;
	

	
	PDEVICE_OBJECT pTopDevObj;
	PDEVICE_EXTENSION pFltDevExtension;

	
	UNREFERENCED_PARAMETER(pIrp);
	UNREFERENCED_PARAMETER(pDevObj);

	pSystemBuffer      = pIrp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

	switch(pIoStackLocation->Parameters.DeviceIoControl.IoControlCode){
	case IOCTL_START_APP:
		KdPrint(("PHWIOScope Filter) IOCTL_START_APP\n"));
		status = STATUS_SUCCESS;
		break;
		
	case IOCTL_CLOSE_APP:
		KdPrint(("PHWIOScope Filter) IOCTL_CLOSE_APP\n"));

		globals.inf_filter = 0;
		globals.isCaptureMode = 0;
		
		traversalNodePtr = globals.fltDevObjList.pListHead;

		while(traversalNodePtr != NULL){
			pFltDevExtension = (PDEVICE_EXTENSION)traversalNodePtr->pFltDevObj->DeviceExtension;
			pFltDevExtension->isCapturePacket = 0;
			traversalNodePtr = traversalNodePtr->pNext;
		}
		
		status = STATUS_SUCCESS;
		break;
		
	case IOCTL_CAPTURE_RUN:
		KdPrint(("PHWIOScope Filter) IOCTL_CAPTURE_RUN, info_filtering [0x%x]\n", *(ULONG*)pSystemBuffer));
		globals.isCaptureMode = 1;
		globals.inf_filter = *(ULONG*)pSystemBuffer;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_CAPTURE_PAUSE:
		KdPrint(("PHWIOScope Filter) IOCTL_CAPTURE_PAUSE\n"));
		globals.isCaptureMode = 0;
		globals.inf_filter = 0;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_REQ_PKT:
		
		pPktInfoSystemBuffer = (PDEQD_PKT_INFO)pSystemBuffer;
		
		if(PHWIoScopePktCheckPktQueueUnderflow()){
			ExAcquireFastMutex(&globals.pktQueue.mutex);
			if(outputBufferLength == sizeof(DEQD_PKT_INFO)){
				PHWIoScopeDequeueingPkt(pPktInfoSystemBuffer);
			}else{
				KdPrint(("PHWIOScope Filter) Invailid Parameter"));
			}
			ExReleaseFastMutex(&globals.pktQueue.mutex);

		}else{
			pPktInfoSystemBuffer->numOfPkt=0;
			//KdPrint(("PHWIoScope Filter) PktQueue is empty"));
		}
		
		pIrp->IoStatus.Information = outputBufferLength;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_ON_CAP_SPECIFIC_DEV:
		pCapSpecificDevInfo = (PCAP_SPECIFIC_DEV_INFO) pSystemBuffer;
		prevIdx = 0;
		
		for(i = 0; i < pCapSpecificDevInfo->numOfSpecificCapDev; i++){
			pTargetPdoName = ExAllocatePoolWithTag(PagedPool, (pCapSpecificDevInfo->header[i] + 1) * sizeof(WCHAR), PHW_POOL_TAG);
			
			if(pTargetPdoName){
				
				RtlCopyMemory(pTargetPdoName, &(pCapSpecificDevInfo->specificDevPdoNameList[prevIdx]), pCapSpecificDevInfo->header[i] * sizeof(WCHAR));
				pTargetPdoName[pCapSpecificDevInfo->header[i]] = L'\0';
				prevIdx += pCapSpecificDevInfo->header[i];
				
				targetDevObjNode = PHWIoScopeFltDevListMgr_FindSpecificDevNodeFromList(&globals.fltDevObjList, pTargetPdoName, DEV_NODE_FIND_METHOD_PDO_NAME);
				
				if(targetDevObjNode){
					
					pFltDevExtension = (PDEVICE_EXTENSION)targetDevObjNode->pFltDevObj->DeviceExtension;
					pFltDevExtension->isCapturePacket = 1;
				}

				ExFreePool(pTargetPdoName);
				pTargetPdoName = NULL;
			}
			
		}
		
		status = STATUS_SUCCESS;
		break;
		
	case IOCTL_OFF_CAP_SPECIFIC_DEV:
		traversalNodePtr = globals.fltDevObjList.pListHead;

		while(traversalNodePtr != NULL){
			pFltDevExtension = (PDEVICE_EXTENSION)traversalNodePtr->pFltDevObj->DeviceExtension;
			pFltDevExtension->isCapturePacket = 0;
			traversalNodePtr = traversalNodePtr->pNext;
		}
		
		status = STATUS_SUCCESS;
		break;

	case IOCTL_GET_CAPTURE_MODE_ACTIVATED_DEV_PDONAME:
		
		pCapSpecificDevInfo = (PCAP_SPECIFIC_DEV_INFO)pSystemBuffer;

		PHWIoScopeFltDevListMgr_GetPdoNameListCaptureModeActivatedDev(&globals.fltDevObjList, pCapSpecificDevInfo);
		
		pIrp->IoStatus.Information = outputBufferLength;
		status = STATUS_SUCCESS;
		break;
		
	default:
		KdPrint(("PHWIOScope Filter) Unknown IoCtl Code - Test\n"));
		status = STATUS_INVALID_PARAMETER;
		break;
	}
	
	return status;
}

