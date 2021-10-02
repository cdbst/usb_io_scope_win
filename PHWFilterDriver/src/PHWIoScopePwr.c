#include "PHWIoScope.h"
#include "PHWIoScopePwr.h"

NTSTATUS PHWIoScope_DispatchPwr(PDEVICE_OBJECT pDevObj, PIRP pIrp){

    PDEVICE_EXTENSION pDevExt;
    NTSTATUS status;

    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    status = IoAcquireRemoveLock (&pDevExt->removeLock, pIrp);

    if(!NT_SUCCESS (status)){
        pIrp->IoStatus.Status = status;
        PoStartNextPowerIrp(pIrp);
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        return status;
    }

    PoStartNextPowerIrp(pIrp);
    IoSkipCurrentIrpStackLocation(pIrp);
    status = PoCallDriver(pDevExt->pNextLowerDevObj, pIrp);
    IoReleaseRemoveLock(&pDevExt->removeLock, pIrp);

    return status;
}


