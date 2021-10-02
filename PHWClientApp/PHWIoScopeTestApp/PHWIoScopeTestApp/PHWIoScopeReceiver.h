#ifndef __PHWIOSCOPERECEIVER_H__
#define __PHWIOSCOPERECEIVER_H__

class MsgReceiver{
private:
	HDEVNOTIFY hDevNotify;
	HANDLE devHandle;
public:
	MsgReceiver();
	~MsgReceiver();
	BOOL GetDeviceHandle(HANDLE** devHandle);
};

#endif