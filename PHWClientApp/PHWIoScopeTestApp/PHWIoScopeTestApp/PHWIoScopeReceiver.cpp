#include "stdafx.h"
#include "PHWIoScopeReceiver.h"
#include "PHWIoScopePublic.h"
#include <Dbt.h>
#include <afxwin.h>


MsgReceiver::MsgReceiver(){
    this->devHandle = NULL;
    this->hDevNotify = NULL;
}

MsgReceiver::~MsgReceiver(){
    
    if (this->hDevNotify!=NULL && !UnregisterDeviceNotification(this->hDevNotify)){
        AfxMessageBox(_T("Notify Obj Delete Error"));
    }

    if (this->devHandle != NULL){
        CloseHandle(this->devHandle);
    }
}
BOOL MsgReceiver::GetDeviceHandle(HANDLE** devHandle){

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
        *devHandle = &this->devHandle;
        return TRUE;
    }

}