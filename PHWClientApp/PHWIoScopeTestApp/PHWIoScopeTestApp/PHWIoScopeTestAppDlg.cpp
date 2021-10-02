
// PHWIoScopeTestAppDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PHWIoScopeTestApp.h"
#include "PHWIoScopeTestAppDlg.h"
#include "afxdialogex.h"

#include "PHWIoScopePublic.h"
#include "PHWIoScopeIoctlCode.h"
#include <Dbt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinThread* pPktQueingThread = NULL;
CWinThread* pPktDequeingThread = NULL;
CEvent* pQueingEvent;
CEvent* pDequeingEvent;

CEvent* pCapureEvnet;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// CPHWIoScopeTestAppDlg dialog



CPHWIoScopeTestAppDlg::CPHWIoScopeTestAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPHWIoScopeTestAppDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPHWIoScopeTestAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MSG, msgList);
	DDX_Control(pDX, ID_SWITCH, btnSwitch);
	DDX_Control(pDX, IDC_CKBOX_IFLTR2, inf_filter2);
	DDX_Control(pDX, IDC_CKBOX_IFLTR3, inf_filter3);
	DDX_Control(pDX, IDC_CKBOX_IFLTR1, inf_filter1);
	DDX_Control(pDX, IDC_CKBOX_IFLTR1, inf_filter1);
	DDX_Control(pDX, IDC_CHECK1, inf_filter4);
	DDX_Control(pDX, IDC_TREE1, devTreeView);
}

BEGIN_MESSAGE_MAP(CPHWIoScopeTestAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
	ON_BN_CLICKED(ID_SWITCH, &CPHWIoScopeTestAppDlg::OnBnClickedSwitch)
	ON_WM_CLOSE()
	ON_BN_CLICKED(ID_BTN_EXIT, &CPHWIoScopeTestAppDlg::OnBnClickedBtnExit)
	ON_BN_CLICKED(ID_BTN_CLEAR, &CPHWIoScopeTestAppDlg::OnBnClickedBtnClear)
	ON_BN_CLICKED(IDC_BTN_DFLTR, &CPHWIoScopeTestAppDlg::OnBnClickedBtnDfltr)
	ON_MESSAGE(WM_DEVICECHANGE, OnDeviceChange)
	ON_BN_CLICKED(ID_BTN_DEVTREE_RENEW, &CPHWIoScopeTestAppDlg::OnBnClickedBtnDevtreeRenew)
END_MESSAGE_MAP()


// CPHWIoScopeTestAppDlg message handlers

BOOL CPHWIoScopeTestAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	
	//장치 노티 이벤트 등록 작업
	numOfDevNotiHanleList = 0;

	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVCLASS_USB;

	this->devNotiHanleList[this->numOfDevNotiHanleList++] = RegisterDeviceNotification(
		AfxGetMainWnd()->m_hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVCLASS_KEYBOARD;

	this->devNotiHanleList[this->numOfDevNotiHanleList++] = RegisterDeviceNotification(
		AfxGetMainWnd()->m_hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVCLASS_MOUSE;

	this->devNotiHanleList[this->numOfDevNotiHanleList++] = RegisterDeviceNotification(
		AfxGetMainWnd()->m_hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);
	

	//장치 리스트 열거
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_USB); // Discover DevTree;
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_KEYBOARD); // Discover DevTree;
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_MOUSE); // Discover DevTree;
	this->devTreeInfo.TraversalDevTree(this->devTreeInfo.GetRootDevNode());
	this->devTreeInfo.DrawDevTree(&this->devTreeView, this->devTreeInfo.GetRootDevNode());
	this->devTreeInfo.ExpandDevTree(&this->devTreeView, this->devTreeInfo.GetRootDevNode());
	
	
	//컨트롤 디바이스 오브젝트 핸들을 얻는다.
	this->devHandle = NULL;
	this->msgReceiver = new MsgReceiver();
	if (!(this->msgReceiver->GetDeviceHandle(&this->devHandle))){
		AfxMessageBox(_T("Invalid Device Handle"));
		AfxGetMainWnd()->PostMessage(WM_CLOSE);
		return TRUE;
	}
	
	//장치와 통신하기 위한 컨트롤러 객체를 생성하고 앱이 시작됬다는 것을 컨트롤 디바이스 오브젝트에게 알린다.
	this->devCtlr = new DeviceCtlr();
	if (!(this->devCtlr->DoIoctl(*devHandle, IOCTL_START_APP))){
		AfxMessageBox(_T("Device IOCTL Fail"));
		AfxGetMainWnd()->PostMessage(WM_CLOSE);
		return TRUE;
	}
	
	// 페킷 큐를 생성하고 큐에 페킷을 가져오는 쓰레드와 받아온 페킷을 출력하는 쓰레드를 생성한다.
	this->pktQueue = new PktQueue();

	pCapureEvnet = new CEvent(FALSE, TRUE);
	this->msgList.SetHorizontalExtent(MAX_PKT_DATA_BUFFER_SIZE * 6);

	pQueingEvent = new CEvent(FALSE, FALSE);
	this->CreatePktQueingThread();

	pDequeingEvent = new CEvent(FALSE, FALSE);
	this->CreatePktDequeingThread();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPHWIoScopeTestAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPHWIoScopeTestAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPHWIoScopeTestAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CPHWIoScopeTestAppDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	if (this->msgReceiver){
		delete this->msgReceiver;
	}

	if (this->devCtlr){
		delete this->devCtlr;
	}

	if (this->pktQueue){
		delete this->pktQueue;
	}

	if (pPktQueingThread){
		this->DestroyPktQueingThread();
	}

	if (pPktDequeingThread){
		this->DestroyPktDequeingThread();
	}

	
}


void CPHWIoScopeTestAppDlg::OnBnClickedSwitch()
{
	CString btnTxt;
	this->btnSwitch.GetWindowText(btnTxt);

	if (btnTxt == "OFF"){
		if (!(this->devCtlr->DoIoctl(*this->devHandle, IOCTL_CAPTURE_PAUSE))){
			AfxMessageBox(_T("Device IOCTL Fail"));
		}
		//pQueingEvent->ResetEvent();
		//pDequeingEvent->ResetEvent();

		pCapureEvnet->ResetEvent();

		inf_filter1.EnableWindow(1);
		inf_filter2.EnableWindow(1);
		inf_filter3.EnableWindow(1);
		inf_filter4.EnableWindow(1);

		this->btnSwitch.SetWindowTextW(_T("ON"));
	}
	else if (btnTxt == "ON"){

		ULONG inf_filter = 0;

		if (inf_filter1.GetCheck()){
			inf_filter |= INFORMATION_FILTER1_IN;
		}
		if (inf_filter2.GetCheck()){
			inf_filter |= INFORMATION_FILTER2_OUT;
		}
		if (inf_filter3.GetCheck()){
			inf_filter |= INFORMATION_FILTER3_IRP;
		}
		if (inf_filter4.GetCheck()){
			inf_filter |= INFORMATION_FILTER4_USBCTRL;
		}

		inf_filter1.EnableWindow(0);
		inf_filter2.EnableWindow(0);
		inf_filter3.EnableWindow(0);
		inf_filter4.EnableWindow(0);

		if (!(this->devCtlr->DoIoctlTx(*this->devHandle, IOCTL_CAPTURE_RUN, &inf_filter, sizeof(inf_filter)))){
			AfxMessageBox(_T("Device IOCTL Fail"));
		}
		pQueingEvent->SetEvent();
		//pDequeingEvent->SetEvent();
		pCapureEvnet->SetEvent();
		this->btnSwitch.SetWindowTextW(_T("OFF"));
	}

}


void CPHWIoScopeTestAppDlg::OnClose()
{
	
	// TODO: Add your message handler code here and/or call default
	if (!(this->devCtlr->DoIoctl(*devHandle, IOCTL_CLOSE_APP))){
		AfxMessageBox(_T("Device IOCTL Fail"));
	}

	this->DestroyPktQueingThread();
	this->DestroyPktDequeingThread();

	if (pQueingEvent)
		delete pQueingEvent;

	if (pDequeingEvent)
		delete pDequeingEvent;

	if (pCapureEvnet)
		delete pCapureEvnet;
	

	for (unsigned int i = 0; i < this->numOfDevNotiHanleList; i++){
		if (this->devNotiHanleList[i] != NULL){
			if (!UnregisterDeviceNotification(this->devNotiHanleList[i])){
				TRACE(_T("UnregisterDeviceNotification"));
			}
		}
	}
	this->numOfDevNotiHanleList = 0;

	//this->devTreeInfo.DestroyDevNode(this->devTreeInfo.GetRootDevNode());

	CDialogEx::OnClose();
}

void CPHWIoScopeTestAppDlg::CreatePktQueingThread()
{
	if (pPktQueingThread != NULL)
	{
		AfxMessageBox(_T("thread가 이미 실행중입니다!"));
		return;
	}

	pPktQueingThread = AfxBeginThread(PktQueingThread, this,
		THREAD_PRIORITY_HIGHEST, 0, CREATE_SUSPENDED);

	if (pPktQueingThread == NULL){
		AfxMessageBox(_T("thread 생성 실패!"));
	}

	pPktQueingThread->m_bAutoDelete = FALSE;
	pPktQueingThread->ResumeThread();
}

void CPHWIoScopeTestAppDlg::DestroyPktQueingThread()
{
	if (NULL != pPktQueingThread)
	{
		/*
		DWORD dwResult = ::WaitForSingleObject(pPktQueingThread->m_hThread, INFINITE);

		if (dwResult == WAIT_TIMEOUT){
			TRACE("thread 제거 실패!");
		}
		else if (dwResult == WAIT_OBJECT_0){
			TRACE("thread 제거 성공!");
		}
		*/
		delete pPktQueingThread;
		pPktQueingThread = NULL;
	}

}

void CPHWIoScopeTestAppDlg::CreatePktDequeingThread()
{
	if (pPktDequeingThread != NULL)
	{
		AfxMessageBox(_T("thread가 이미 실행중입니다!"));
		return;
	}

	pPktDequeingThread = AfxBeginThread(PktDequeingThread, this,
		THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

	if (pPktDequeingThread == NULL){
		AfxMessageBox(_T("thread 생성 실패!"));
	}

	pPktDequeingThread->m_bAutoDelete = FALSE;
	pPktDequeingThread->ResumeThread();
}

void CPHWIoScopeTestAppDlg::DestroyPktDequeingThread()
{
	if (NULL != pPktDequeingThread)
	{
		/*
		DWORD dwResult = ::WaitForSingleObject(pPktQueingThread->m_hThread, INFINITE);

		if (dwResult == WAIT_TIMEOUT){
		TRACE("thread 제거 실패!");
		}
		else if (dwResult == WAIT_OBJECT_0){
		TRACE("thread 제거 성공!");
		}
		*/
		delete pPktDequeingThread;
		pPktDequeingThread = NULL;
	}

}


UINT PktQueingThread(LPVOID lpParam)
{
	CPHWIoScopeTestAppDlg* dlgClass = (CPHWIoScopeTestAppDlg*)lpParam;
	DEQD_PKT_INFO deqdPktInfo;
	CString listviewMsg;
	unsigned int i;

	while (1){
		WaitForSingleObject(pCapureEvnet->m_hObject, INFINITE);

		if (!(dlgClass->devCtlr->DoIoctlRx(*dlgClass->devHandle, IOCTL_REQ_PKT, &deqdPktInfo, sizeof(DEQD_PKT_INFO)))){
			AfxMessageBox(_T("Device IOCTL Fail"));
		}
		
		i = 0;

		while (i < deqdPktInfo.numOfPkt){
			WaitForSingleObject(pQueingEvent->m_hObject, INFINITE);
			if (!dlgClass->pktQueue->QueueingPkt(&deqdPktInfo.pktInfo[i])){
				pDequeingEvent->SetEvent();
				continue;
			}
			else{
				pDequeingEvent->SetEvent();
				i++;
				//TRACE("QUEUE\n");
			}
		}

		Sleep(1);
	}
	return 0L;
}

UINT PktDequeingThread(LPVOID lpParam)
{
	CPHWIoScopeTestAppDlg* dlgClass = (CPHWIoScopeTestAppDlg*)lpParam;
	PKT_INFO pktInfo;
	CString listviewMsg;

	while (1){
		WaitForSingleObject(pDequeingEvent->m_hObject, INFINITE);

		if (!dlgClass->pktQueue->DequeueingPkt(&pktInfo)){
			continue;
		}
		else{

			if (pktInfo.isMultiplePkt){

				setListMsgPrefix(&pktInfo, &listviewMsg); // SET prefix

				for (unsigned int j = 0; j < pktInfo.dataDivisionIdx; j++){
					listviewMsg.AppendFormat(_T("%02X "), pktInfo.data[j]);
				}

				if (listviewMsg != _T("")){
					dlgClass->msgList.AddString(listviewMsg);
				}

				listviewMsg = _T("");
			}

			setListMsgPrefix(&pktInfo, &listviewMsg); // SET prefix

			for (unsigned int j = pktInfo.dataDivisionIdx; j < pktInfo.dataSize; j++){
				listviewMsg.AppendFormat(_T("%02X "), pktInfo.data[j]);
			}
			
			if (listviewMsg != _T("")){
				dlgClass->msgList.AddString(listviewMsg);
			}
			listviewMsg = _T("");
			
		}
		
		//TRACE("DEQUEUE\n");
		pQueingEvent->SetEvent();
	}
	return 0L;
}


void CPHWIoScopeTestAppDlg::OnBnClickedBtnExit()
{
	// TODO: Add your control notification handler code here
	AfxGetMainWnd()->PostMessage(WM_CLOSE);
}


void CPHWIoScopeTestAppDlg::OnBnClickedBtnClear()
{
	// TODO: Add your control notification handler code here
	this->msgList.ResetContent();
}


void setListMsgPrefix(PPKT_INFO pPktInfo, CString* pListMsg){
	if (pPktInfo->direction == 0){
		if (pPktInfo->dataMajorType != DATA_MAJOR_TYPE_USBCTRL){
			pListMsg->AppendFormat(_T("[OUT] "));
		}
		else{
			pListMsg->AppendFormat(_T("[CTRL OUT] "));
		}
	}
	else{
		if (pPktInfo->dataMajorType != DATA_MAJOR_TYPE_USBCTRL){
			pListMsg->AppendFormat(_T("[I   N] "));
		}
		else{
			pListMsg->AppendFormat(_T("[CTRL I   N] "));
		}
	}
}

void CPHWIoScopeTestAppDlg::OnBnClickedBtnDfltr()
{
	// TODO: Add your control notification handler code here
	CAP_SPECIFIC_DEV_INFO capSpecificDevInfo;

	capSpecificDevInfo.numOfSpecificCapDev = 0;
	this->devTreeInfo.FindCheckedNodes(this->devTreeInfo.GetRootDevNode(), &this->devTreeView, &capSpecificDevInfo);

	if (!(this->devCtlr->DoIoctl(*devHandle, IOCTL_OFF_CAP_SPECIFIC_DEV))){
		AfxMessageBox(_T("Device IOCTL Fail"));
	}

	if (!(this->devCtlr->DoIoctlTx(*devHandle, IOCTL_ON_CAP_SPECIFIC_DEV, &capSpecificDevInfo, sizeof(capSpecificDevInfo)))){
		AfxMessageBox(_T("Device IOCTL Fail"));
	}
	
}

/*
LRESULT CPHWIoScopeTestAppDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HDR pHdr = NULL;
	PDEV_BROADCAST_DEVICEINTERFACE pDevInf = NULL;
	PDEV_BROADCAST_HANDLE pDevHnd = NULL;
	PDEV_BROADCAST_OEM pDevOem = NULL;
	PDEV_BROADCAST_PORT pDevPort = NULL;
	PDEV_BROADCAST_VOLUME pDevVolume = NULL;
	
	if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam || DBT_DEVNODES_CHANGED == wParam) {
		TRACE(_T("Device Changed!@\n"));
		if (DBT_DEVICEARRIVAL == wParam){
			TRACE(_T("Device is Attached!@\n"));
		}
		else if (DBT_DEVICEREMOVECOMPLETE == wParam){
			TRACE(_T("Device is Removed!@\n"));
		}
		pHdr = (PDEV_BROADCAST_HDR)lParam;
		switch (pHdr->dbch_devicetype) {
		case DBT_DEVTYP_DEVICEINTERFACE:
			TRACE(_T("Evt type is DBT_DEVTYP_DEVICEINTERFACE!@\n"));
			pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
			// do something...
			break;

		case DBT_DEVTYP_HANDLE:
			TRACE(_T("Evt type is DBT_DEVTYP_HANDLE!@\n"));
			pDevHnd = (PDEV_BROADCAST_HANDLE)pHdr;
			// do something...
			break;

		case DBT_DEVTYP_OEM:
			TRACE(_T("Evt type is DBT_DEVTYP_OEM!@\n"));
			pDevOem = (PDEV_BROADCAST_OEM)pHdr;
			// do something...
			break;

		case DBT_DEVTYP_PORT:
			TRACE(_T("Evt type is DBT_DEVTYP_PORT!@\n"));
			pDevPort = (PDEV_BROADCAST_PORT)pHdr;
			// do something...
			break;

		case DBT_DEVTYP_VOLUME:
			TRACE(_T("Evt type is DBT_DEVTYP_VOLUME!@\n"));
			pDevVolume = (PDEV_BROADCAST_VOLUME)pHdr;
		
			// do something...
			break;
		}
	}
	return 0;
}
*/

LRESULT CPHWIoScopeTestAppDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

	switch (wParam){
	case DBT_DEVICEARRIVAL:
		TRACE(_T("Evt DBT_DEVICEARRIVAL\n"));
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		TRACE(_T("Evt DBT_DEVICEREMOVECOMPLETE\n"));
		break;
	case DBT_DEVNODES_CHANGED:
		TRACE(_T("Evt DBT_DEVNODES_CHANGED\n"));
		break;
	default:
		TRACE(_T("Evt UNKNOWN\n"));
		break;
	}
	return 0;
}

void CPHWIoScopeTestAppDlg::OnBnClickedBtnDevtreeRenew()
{
	CAP_SPECIFIC_DEV_INFO captureModeActivatedDevList;

	this->devTreeView.DeleteAllItems();
	this->devTreeInfo.ResetDevNode();
	
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_USB); // Discover DevTree;
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_KEYBOARD); // Discover DevTree;
	this->devTreeInfo.DiscoverDevNodes(&GUID_DEVCLASS_MOUSE); // Discover DevTree;

	this->devTreeInfo.DrawDevTree(&this->devTreeView, this->devTreeInfo.GetRootDevNode());
	this->devTreeInfo.ExpandDevTree(&this->devTreeView, this->devTreeInfo.GetRootDevNode());

	if (!(this->devCtlr->DoIoctlRx(*this->devHandle, IOCTL_GET_CAPTURE_MODE_ACTIVATED_DEV_PDONAME, &captureModeActivatedDevList, sizeof(CAP_SPECIFIC_DEV_INFO)))){
		AfxMessageBox(_T("Device IOCTL Fail"));
		return;
	}

	this->devTreeInfo.SetCheckBoxOnWithSpecificDevInfo(&this->devTreeView, &captureModeActivatedDevList, 1);
}
