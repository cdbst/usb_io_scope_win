
// PHWIoScopeTestAppDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "PHWIoScopeReceiver.h"
#include "PHWIoScopeDeviceCtlr.h"
#include "PHWIoScopePktQueue.h"
#include "PHWIoScopeDevTreeInfo.h"
#include <winioctl.h>
#include "afxcmn.h"

#define MAX_DEV_NOTI_HANDLE_LIST 16

// CPHWIoScopeTestAppDlg dialog
class CPHWIoScopeTestAppDlg : public CDialogEx
{
private:

// Construction
public:
	MsgReceiver* msgReceiver;
	DeviceCtlr* devCtlr;
	HANDLE* devHandle;
	PktQueue* pktQueue;
	DevTreeInfo devTreeInfo;

	HDEVNOTIFY devNotiHanleList[MAX_DEV_NOTI_HANDLE_LIST];
	UINT numOfDevNotiHanleList;


	CPHWIoScopeTestAppDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PHWIOSCOPETESTAPP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CListBox msgList;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedSwitch();
	CButton btnSwitch;
	afx_msg void OnClose();

	void CreatePktQueingThread();
	void DestroyPktQueingThread();

	void CreatePktDequeingThread();
	void DestroyPktDequeingThread();

	afx_msg void OnBnClickedBtnExit();
	afx_msg void OnBnClickedBtnClear();
	CButton inf_filter2;
	CButton inf_filter3;
	CButton inf_filer1;
	CButton inf_filter1;
	CButton inf_filter4;
	CTreeCtrl devTreeView;
	afx_msg void OnBnClickedBtnDfltr();
	LRESULT OnDeviceChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnDevtreeRenew();
};

UINT PktQueingThread(LPVOID lpParam);
UINT PktDequeingThread(LPVOID lpParam);

void setListMsgPrefix(PPKT_INFO pPktInfo, CString* pListMsg);

