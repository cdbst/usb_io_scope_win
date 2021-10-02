
#ifndef __PHWIOSCOPEPNP_H__
#define __PHWIOSCOPEPNP_H__

__drv_dispatchType(IRP_MJ_PNP)
DRIVER_DISPATCH PHWIoScope_DispatchPnP;

IO_COMPLETION_ROUTINE PHWIoScope_PnPCompleteDeviceStart;

IO_COMPLETION_ROUTINE PHWIoScope_PnPCompleteDeviceUsageNotification;

NTSTATUS PHWIoScope_PnPStartDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPQueryStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPCancelStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPStopDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPQueryRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPSurpriseRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPCancelRemoveDevice(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

NTSTATUS PHWIoScope_PnPDeviceUsageNotification(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

#endif


