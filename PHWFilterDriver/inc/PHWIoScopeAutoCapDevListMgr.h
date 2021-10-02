#ifndef __PHWIOSCOPEAUTOCAPDEVLISTMGR_H__
#define __PHWIOSCOPEAUTOCAPDEVLISTMGR_H__

//새로 연결되면 자동으로 패킷을 캡쳐해야 하는 장치를 관리하기 위함이다. 

#include <wdm.h>

typedef struct _AUTO_CAP_DEV_NODE{
	PWCHAR pHardwareID;
	struct _AUTO_CAP_DEV_NODE* pNext;
	struct _AUTO_CAP_DEV_NODE* pPrev;
}AUTO_CAP_DEV_NODE, *PAUTO_CAP_DEV_NODE;

typedef struct _AUTO_CAP_DEV_LIST{
	PAUTO_CAP_DEV_NODE pListHead;
	PAUTO_CAP_DEV_NODE pListTail;
	unsigned int numOfAutoCapDevNodes;
	KSPIN_LOCK autoCapDevListSpinLock;
}AUTO_CAP_DEV_LIST, *PAUTO_CAP_DEV_LIST;


VOID PHWIoScopeAutoCapDevListMgr_InitList(PAUTO_CAP_DEV_LIST pAutoCapDevList);

VOID PHWIoScopeAutoCapDevListMgr_ReleaseList(PAUTO_CAP_DEV_LIST pAutoCapDevList);

VOID PHWIoScopeAutoCapDevListMgr_InsertAutoCapDevNodeIntoList(
	PAUTO_CAP_DEV_LIST pAutoCapDevList,
	PAUTO_CAP_DEV_NODE pAutoCapDevNode);

PAUTO_CAP_DEV_NODE PHWIoScopeAutoCapDevListMgr_CreateAutoCapDevNode(PWCHAR pHardwareID, const unsigned int hardwareIDSize);
VOID PHWIoScopeAutoCapDevListMgr_DeleteAutoCapDevNodeFromList(PAUTO_CAP_DEV_LIST pAutoCapDevList, PAUTO_CAP_DEV_NODE pAutoCapDevNode);
PAUTO_CAP_DEV_NODE PHWIoScopeAutoCapDevListMgr_FindSpecificAutoCapDevNodeFromList(PAUTO_CAP_DEV_LIST pAutoCapDevList, PWCHAR pHardwareID);
#endif


