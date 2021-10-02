#ifndef __PHWIOSCOPEIOCTLMGR_H__
#define __PHWIOSCOPEIOCTLMGR_H__


NTSTATUS PHWIoScopeCreateCtlDevObj(PDEVICE_OBJECT pDevObj);
NTSTATUS PHWIOScopeCtlDevDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VOID PHWIoScopeDeleteCtlDevObj();
NTSTATUS PHWIOScopeCtlDevDispatchDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIoStackLocation);

#endif


