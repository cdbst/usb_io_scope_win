
#ifndef __PHWIOSCOPE_H__
#define __PHWIOSCOPE_H__

#include <ntddk.h>
#include <Ntstrsafe.h>
#include <dontuse.h>

#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "PHWIoScopePublic.h"
#include "PHWIoScopeFltDevListMgr.h"
#include "PHWIoScopeAutoCapDevListMgr.h"


#define URB_FUNCTION_OPEN_STATIC_STREAMS                      		    	0x0035
#define URB_FUNCTION_CLOSE_STATIC_STREAMS                         			0x0036
#define URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL			0x0037
#define URB_FUNCTION_ISOCH_TRANSFER_USING_CHAINED_MDL    				 	0x0038

#define PHW_POOL_TAG (ULONG)"twhp"

#define MAX_LEN_DEVICE_PROPERTY_BUFFER 		256



// {DE773A71-CE80-4598-AFB5-3ABC26BF9D93}
DEFINE_GUID(WMI_GUID, 
0xde773a71, 0xce80, 0x4598, 0xaf, 0xb5, 0x3a, 0xbc, 0x26, 0xbf, 0x9d, 0x93);


typedef enum _DEVOBJ_TYPE{
	TYPE_FIDO = 10,
	TYPE_CTLDO,
	TYPE_UMKNOWN

}DEVOBJ_TYPE;

typedef enum _DEVICE_PNP_STATE{
    PNP_STATE_NONSTART = 0,         // Not started yet
    PNP_STATE_START,                // Device has received the START_DEVICE IRP
    PNP_STATE_STOP_PENDING,            // Device has received the QUERY_STOP IRP
    PNP_STATE_STOP,                // Device has received the STOP_DEVICE IRP
    PNP_STATE_REMOVE_PENDING,          // Device has received the QUERY_REMOVE IRP
    PNP_STATE_REMOVE_SURPRISE_PENDING,  // Device has received the SURPRISE_REMOVE IRP
    PNP_STATE_REMOVE,                // Device has received the REMOVE_DEVICE IRP
    PNP_STATE_UNKNOWN                // Unknown state

}DEVICE_PNP_STATE;


#define INITIALIZE_DEVICE_STATE(_pDevObjExt_) \
		(_pDevObjExt_)->previousDevPnPState = PNP_STATE_NONSTART; \
		(_pDevObjExt_)->currentDevPnPState = PNP_STATE_NONSTART;

#define CHANGE_DEVICE_STATE(_pDevObjExt_, _newState_) \
		(_pDevObjExt_)->previousDevPnPState = (_pDevObjExt_)->currentDevPnPState; \
		(_pDevObjExt_)->currentDevPnPState = (_newState_);

#define ROLLBACK_DEVICE_STATE(_pDevObjExt_) \
		(_pDevObjExt_)->currentDevPnPState = (_pDevObjExt_)->previousDevPnPState;


typedef struct _COMMON_DEVICE_DATA{

	DEVOBJ_TYPE devObjType;
	ULONG deleted; // False if the deviceobject is valid, TRUE if it's deleted
	PVOID controlData; // Store your control data here

}COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

typedef struct _DEVICE_EXTENSION{

	COMMON_DEVICE_DATA commonDevData;

	DEVICE_PNP_STATE currentDevPnPState;
	DEVICE_PNP_STATE previousDevPnPState;

	PDEVICE_OBJECT pSelfFilterDevObj;
	PDEVICE_OBJECT pNextLowerDevObj;
	PDEVICE_OBJECT pUnderlyingPsclDevObj;
	PDEVICE_OBJECT pTopDevObj;

	IO_REMOVE_LOCK removeLock;
	ULONG eventNotifyCounter;

	UNICODE_STRING interfaceName;
	PWCHAR pPdoName;
	PWCHAR pHardwareID;
	unsigned int hardwareIDSize;

	UCHAR isCapturePacket;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _CONTROL_DEVICE_EXTENSION {

    COMMON_DEVICE_DATA commonDevData;

    BOOLEAN deleted; // False if the deviceobject is valid, TRUE if it's deleted
    PVOID controlData; // Store your control data here

}CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;

typedef struct _PKT_QUEUE{
	PKT_INFO pktInfo[MAX_SIZE_PKT_QUEUE];

	unsigned int front;
	unsigned int rear;
	unsigned int remained_pkt;
	
	FAST_MUTEX mutex;	
}PKT_QUEUE, *PPKT_QUEUE;


typedef struct _GLOBALS{

    UNICODE_STRING RegistryPath;
	KEVENT ioctlLock;
	ULONG ctlDevObjCount;
	PDEVICE_OBJECT pCtlDevObj;

	USHORT isCaptureMode;
	ULONG inf_filter;
	PKT_QUEUE pktQueue;
	FLT_DEVOBJ_LIST fltDevObjList;
	AUTO_CAP_DEV_LIST autoCapDevList;
}GLOBALS, *PGLOBALS;

DRIVER_INITIALIZE DriverEntry;

DRIVER_ADD_DEVICE PHWIoScope_AddDevice;

DRIVER_UNLOAD PHWIoScope_DriverUnload;

__drv_dispatchType_other
DRIVER_DISPATCH PHWIoScope_DispatchRoutine;

NTSTATUS PHWIoScope_IrpCompletionRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp, PVOID pContext);

extern GLOBALS globals;

#endif


