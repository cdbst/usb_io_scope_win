#ifndef __PHWIOSCOPEFLTDEVLISTMGR_H__
#define __PHWIOSCOPEFLTDEVLISTMGR_H__

#include <wdm.h>
#include "PHWIoScopePublic.h"

typedef struct _FLT_DEVOBJ_NODE{
	struct _FLT_DEVOBJ_NODE* pNext;
	struct _FLT_DEVOBJ_NODE* pPrev;
	PDEVICE_OBJECT pFltDevObj;
}FLT_DEVOBJ_NODE, *PFLT_DEVOBJ_NODE;

typedef struct _FLT_DEVOBJ_LIST{
	PFLT_DEVOBJ_NODE pListHead;
	PFLT_DEVOBJ_NODE pListTail;
	ULONG numOfFltDevObj;
	KSPIN_LOCK fltDevObjListSpinLock;
}FLT_DEVOBJ_LIST, *PFLT_DEVOBJ_LIST;

typedef enum _LIST_INSERT_POS{
	LIST_INSERT_POS_HEAD,
	LIST_INSERT_POS_TAIL
}LIST_INSERT_POS;

typedef enum _DEV_NODE_FIND_METHOD{
	DEV_NODE_FIND_METHOD_PDO_NAME,
	DEV_NODE_FIND_METHOD_FIDO_ADDR
}DEV_NODE_FIND_METHOD;

VOID PHWIoScopeFltDevListMgr_InitList(PFLT_DEVOBJ_LIST pFltDevObjList);
VOID PHWIoScopeFltDevListMgr_ReleaseList(PFLT_DEVOBJ_LIST pFltDevObjList);

VOID PHWIoScopeFltDevListMgr_InsertFltDevNodeIntoList(
	PFLT_DEVOBJ_LIST pFltDevObjList,
	PFLT_DEVOBJ_NODE pFltDevObjNode,
	LIST_INSERT_POS insertPos);

PFLT_DEVOBJ_NODE PHWIoScopeFltDevListMgr_CreateFltDevNode(PDEVICE_OBJECT pFltDevObj);
VOID PHWIoScopeFltDevListMgr_DeleteFltDevNodeFromList(PFLT_DEVOBJ_LIST pFltDevObjList, PFLT_DEVOBJ_NODE pFltDevObjNode);

PFLT_DEVOBJ_NODE PHWIoScopeFltDevListMgr_FindSpecificDevNodeFromList(
	PFLT_DEVOBJ_LIST pFltDevObjList,
	PVOID pFindInfo,
	DEV_NODE_FIND_METHOD findMethod);


VOID PHWIoScopeFltDevListMgr_GetPdoNameListCaptureModeActivatedDev(PFLT_DEVOBJ_LIST pFltDevObjList, PCAP_SPECIFIC_DEV_INFO pCapSpecificDevInfo);




#endif


