#pragma once
#include "afxwin.h"
#include "PHWIoScopePublic.h"

class PktQueue{
private:
	PKT_INFO pktQueue[MAX_SIZE_PKT_QUEUE];
	unsigned int front;
	unsigned int rear;
	unsigned int remainedPkt;
public:
	PktQueue();
	~PktQueue();
	BOOLEAN QueueingPkt(PPKT_INFO pkt);
	BOOLEAN DequeueingPkt(PPKT_INFO pkt);
	unsigned int GetRemainedPkt();
};


