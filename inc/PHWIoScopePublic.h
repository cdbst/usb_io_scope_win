#ifndef __PHWIOSCOPEPUBLIC_H__
#define __PHWIOSCOPEPUBLIC_H__

#include <initguid.h>

#define MAX_PKT_DATA_BUFFER_SIZE	1040
#define MAX_SIZE_PKT_QUEUE			4096
#define MAX_TRANSFER_AT_ONCE		256

#define DRIVER_NAME "PHWIoScope.sys"
#define DEVICE_SYM_LINK_NAME L"\\DosDevices\\PHWIoScope"
#define CTL_DEVICE_NAME L"\\Device\\PHWIoScope"
#define CTL_DEVICE_GLOBAL_PATH L"\\\\.\\PHWIoscope"

typedef struct _PKT_INFO{
	ULONG_PTR devObjInfo;
	ULONG_PTR iprInfo;
	unsigned char direction;
	unsigned int irpMajorFunction;
	unsigned int irpMinorFunction;
	unsigned char dataMajorType;
	unsigned char dataMinorType;
	ULONG dataSize;

	UCHAR isMultiplePkt;
	unsigned int dataDivisionIdx;
	unsigned char data[MAX_PKT_DATA_BUFFER_SIZE];
}PKT_INFO, *PPKT_INFO;

typedef struct _DEQD_PKT_INFO{
	unsigned int numOfPkt;
	PKT_INFO pktInfo[MAX_TRANSFER_AT_ONCE];
}DEQD_PKT_INFO, *PDEQD_PKT_INFO;

#define MAX_SIZE_CAP_DEV_LIST 1024

typedef struct _CAP_SPECIFIC_DEV_INFO{
	unsigned int header[MAX_SIZE_CAP_DEV_LIST];
	unsigned int numOfSpecificCapDev;
	WCHAR specificDevPdoNameList[MAX_SIZE_CAP_DEV_LIST];
}CAP_SPECIFIC_DEV_INFO, *PCAP_SPECIFIC_DEV_INFO;

// {58BFC9CF-D70D-4B25-99EA-5B06BC4BA21B}
DEFINE_GUID(GUID_PHW_IO_EVNET,
0x58bfc9cf, 0xd70d, 0x4b25, 0x99, 0xea, 0x5b, 0x6, 0xbc, 0x4b, 0xa2, 0x1b);

// {DC299C66-053C-4955-961B-8FDCC83DC57D}
DEFINE_GUID(GUID_PHW_CTL_DEVOBJ,
0xdc299c66, 0x53c, 0x4955, 0x96, 0x1b, 0x8f, 0xdc, 0xc8, 0x3d, 0xc5, 0x7d);

// {4d36e97d-e325-11ce-bfc1-08002be10318}
DEFINE_GUID(GUID_PHWFLT_DEVICE_CLASS,
	0x4d36e97d, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

// {EC9F3C47-85CC-4EF7-8B97-7D3B09D3051A}
DEFINE_GUID(GUID_PHWFLT_INTERFACE,
	0xec9f3c47, 0x85cc, 0x4ef7, 0x8b, 0x97, 0x7d, 0x3b, 0x9, 0xd3, 0x5, 0x1a);



#define INFORMATION_FILTER1_IN			0x01
#define INFORMATION_FILTER2_OUT			0x02
#define INFORMATION_FILTER3_IRP			0x04
#define INFORMATION_FILTER4_USBCTRL		0x08
#define INFORMATION_FILTER_MAX_FUNC		4


#define DATA_MAJOR_TYPE_USBCTRL			0x01


#endif
