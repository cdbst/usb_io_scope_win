#ifndef __PHWIOSCOPEPKTMGR_H__
#define __PHWIOSCOPEPKTMGR_H__

#include "PHWIoScope.h"

typedef struct _QUEUEING_PKT_CONTEXT{
	PPKT_INFO pPktInfo;
	PIO_WORKITEM pIoWorkItem;
}QUEUEING_PKT_CONTEXT, *PQUEUEING_PKT_CONTEXT;


void PHWIoScopeCapturePkt(PDEVICE_OBJECT pDevObj, PIRP pIrp, unsigned char pktDirection);
void PHWIoScopeQueueingPkt(PDEVICE_OBJECT pDevObj, PVOID pContext); // This Function is WorkItem CallBack Routime.
void PHWIoScopeDequeueingPkt(PDEQD_PKT_INFO pDeqdPktInfo);

int PHWIoScopePktCheckPktQueueOverflow();
int PHWIoScopePktCheckPktQueueUnderflow();

void PHWIoScopeExpelMeaningfulData(PDEVICE_OBJECT pDevObj, PIRP pIrp, PPKT_INFO pDestination, unsigned char pktDirection);
void PHWIoScopeExpelBufferData(PIRP pIrp, PPKT_INFO pDestination, PVOID pBuffer);
void PHWIoScopeExpelURBData(PURB pURB, PPKT_INFO pDestination);

#endif


