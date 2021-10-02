#include "stdafx.h"
#include "PHWIoScopePktQueue.h"

PktQueue::PktQueue(){
	this->front = 0;
	this->rear = 0;
	this->remainedPkt = 0;
}

PktQueue::~PktQueue(){

}

BOOLEAN PktQueue::QueueingPkt(PPKT_INFO pkt){
	if ((this->rear + 1) % MAX_SIZE_PKT_QUEUE == this->front){
		TRACE("queue is full %d", this->remainedPkt);
		return FALSE;
	}
	RtlCopyMemory(&this->pktQueue[this->rear], pkt, sizeof(PKT_INFO));
	this->rear = (this->rear + 1) % MAX_SIZE_PKT_QUEUE;
	this->remainedPkt++;
	return TRUE;
}
BOOLEAN PktQueue::DequeueingPkt(PPKT_INFO pkt){
	if (this->front == this->rear){
		return FALSE;
	}
	RtlCopyMemory(pkt, &this->pktQueue[this->front], sizeof(PKT_INFO));
	RtlZeroMemory(&this->pktQueue[this->front], sizeof(PKT_INFO));
	this->front = (this->front + 1) % MAX_SIZE_PKT_QUEUE;
	this->remainedPkt--;
	return TRUE;
}

unsigned int PktQueue::GetRemainedPkt(){
	return this->remainedPkt;
}









