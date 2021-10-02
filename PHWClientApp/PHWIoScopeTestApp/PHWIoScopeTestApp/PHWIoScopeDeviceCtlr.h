#ifndef __PHWIOSCOPEDEVICECTLR_H__
#define __PHWIOSCOPEDEVICECTLR_H__

class DeviceCtlr{
private:
	HANDLE devHandle;
public:
	DeviceCtlr();
	~DeviceCtlr();
	BOOL GetDeviceHandle(HANDLE** devHandle);
	BOOL DoIoctl(HANDLE devHandle, DWORD ioctlCode);
	BOOL DoIoctlRx(HANDLE devHandle, DWORD ioctlCode, PVOID rxBuffer, UINT rxBufferSize);
	BOOL DoIoctlTx(HANDLE devHandle, DWORD ioctlCode, PVOID txBuffer, UINT txBufferSize);
};

#endif

