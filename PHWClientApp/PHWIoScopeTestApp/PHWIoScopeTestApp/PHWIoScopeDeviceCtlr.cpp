#include "stdafx.h"
#include "PHWIoScopeDeviceCtlr.h"
#include "PHWIoScopePublic.h"
#include "PHWIoScopeIoctlCode.h"
#include <Dbt.h>
#include <afxwin.h>
#include <winioctl.h>


DeviceCtlr::DeviceCtlr(){
	this->devHandle = NULL;
}

DeviceCtlr::~DeviceCtlr(){
	if (this->devHandle != NULL){
		CloseHandle(this->devHandle);
	}
}

BOOL DeviceCtlr::GetDeviceHandle(HANDLE** devHandle){

	this->devHandle = CreateFile(CTL_DEVICE_GLOBAL_PATH,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (this->devHandle == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	else{
		*devHandle = &(this->devHandle);
		return TRUE;
	}
}

BOOL DeviceCtlr::DoIoctl(HANDLE devHandle, DWORD ioctlCode){

	DWORD byteRet = NULL;
	
	if (!DeviceIoControl(devHandle,
		ioctlCode,
		NULL,	// Ptr to InBuffer
		0,		// Length of InBuffer
		0,      // Ptr to OutBuffer
		0,      // Length of OutBuffer
		&byteRet,      // BytesReturned
		0))		// Ptr to Overlapped structure
	{   
		CString errMsg;
		errMsg.Format(_T("DeviceIoControl failed with error 0x%x\n"), GetLastError());
		AfxMessageBox(errMsg);
		return FALSE;
	}

	return TRUE;
}


BOOL DeviceCtlr::DoIoctlRx(HANDLE devHandle, DWORD ioctlCode, PVOID rxBuffer, UINT rxBufferSize){

	DWORD byteRet = NULL;

	if (!DeviceIoControl(devHandle,
		ioctlCode,
		NULL,	// Ptr to InBuffer
		0,		// Length of InBuffer
		rxBuffer,      // Ptr to OutBuffer
		rxBufferSize,      // Length of OutBuffer
		&byteRet,      // BytesReturned
		0))		// Ptr to Overlapped structure
	{
		CString errMsg;
		errMsg.Format(_T("DeviceIoControl failed with error 0x%x\n"), GetLastError());
		AfxMessageBox(errMsg);
		return FALSE;
	}

	return TRUE;
}

BOOL DeviceCtlr::DoIoctlTx(HANDLE devHandle, DWORD ioctlCode, PVOID txBuffer, UINT txBufferSize){

	DWORD byteRet = NULL;

	if (!DeviceIoControl(devHandle,
		ioctlCode,
		txBuffer,	// Ptr to InBuffer
		txBufferSize,		// Length of InBuffer
		0,      // Ptr to OutBuffer
		0,      // Length of OutBuffer
		&byteRet,      // BytesReturned
		0))		// Ptr to Overlapped structure
	{
		CString errMsg;
		errMsg.Format(_T("DeviceIoControl failed with error 0x%x\n"), GetLastError());
		AfxMessageBox(errMsg);
		return FALSE;
	}

	return TRUE;
}

