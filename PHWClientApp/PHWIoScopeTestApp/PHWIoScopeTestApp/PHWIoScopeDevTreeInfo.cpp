#include "stdafx.h"
#include "PHWIoScopeDevTreeInfo.h"

DevTreeInfo::DevTreeInfo(){
	this->rootNode = this->CreateDevNode(_T(""), _T(""), _T(""), NULL);
	this->rootNode->sDevDesc.Format(_T("%S"), "USB_CLASS");
}

DevTreeInfo::~DevTreeInfo(){
	// ROOT 노드부터 각각의 노드들을 순회하면서 각각의 노드들을 메모리 해재하는 코드를 구현한다.
	this->DestroyDevNode(this->rootNode);
}


BOOL DevTreeInfo::DiscoverDevNodes(const GUID *ClassGuid){
	
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs(
		ClassGuid,
		0, // Enumerator
		0,
		DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE){
		// Insert error handling here.
		return 1;
	}

	

	// Enumerate through all devices in Set.

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++){
		DWORD DataT;
		LPTSTR buffer = NULL;
		DWORD buffersize = 0;
		
		CString devDesc;
		CString devPath;
		CString devPDO;

		//
		// Call function with null to begin with, 
		// then use the returned buffer size (doubled)
		// to Alloc the buffer. Keep calling until
		// success or an unknown failure.
		//
		//  Double the returned buffersize to correct
		//  for underlying legacy CM functions that 
		//  return an incorrect buffersize value on 
		//  DBCS/MBCS systems.
		//

		while (!SetupDiGetDeviceRegistryProperty(
			hDevInfo,
			&DeviceInfoData,
			SPDRP_DEVICEDESC,
			&DataT,
			(PBYTE)buffer,
			buffersize,
			&buffersize)){

			if (GetLastError() ==ERROR_INSUFFICIENT_BUFFER){
				// Change the buffer size.
				if (buffer) LocalFree(buffer);
				// Double the size to avoid problems on 
				// W2k MBCS systems per KB 888609. 
				buffer = (LPTSTR)LocalAlloc(LPTR, buffersize * 2);
			}else{
				// Insert error handling here.
				break;
			}
		}
		
		//TRACE(_T("DEV - NAME:[%s]\n"), buffer);
		devDesc.Format(_T("%s"), buffer);
		TRACE(devDesc);

		buffer = NULL;
		buffersize = 0;

		while (!SetupDiGetDeviceRegistryProperty(
			hDevInfo,
			&DeviceInfoData,
			SPDRP_LOCATION_PATHS,
			&DataT,
			(PBYTE)buffer,
			buffersize,
			&buffersize)){

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
				// Change the buffer size.
				if (buffer) LocalFree(buffer);
				// Double the size to avoid problems on 
				// W2k MBCS systems per KB 888609. 
				buffer = (LPTSTR)LocalAlloc(LPTR, buffersize * 2);
			}else{
				// Insert error handling here.
				break;
			}
		}
		
		//TRACE(_T("DEV - PATH:[%s]\n"), buffer);
		devPath.Format(_T("%s"), buffer);
		//TRACE(devPath);

		while (!SetupDiGetDeviceRegistryProperty(
			hDevInfo,
			&DeviceInfoData,
			SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
			&DataT,
			(PBYTE)buffer,
			buffersize,
			&buffersize)){

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
				// Change the buffer size.
				if (buffer) LocalFree(buffer);
				// Double the size to avoid problems on 
				// W2k MBCS systems per KB 888609. 
				buffer = (LPTSTR)LocalAlloc(LPTR, buffersize * 2);
			}
			else{
				// Insert error handling here.
				break;
			}
		}
		devPDO.Format(_T("%s"), buffer);
		TRACE(_T("%s\n----------------------------------"), devPDO);

		this->CreateNodeWithDevPath(devDesc, devPath, devPDO);

		if (buffer) LocalFree(buffer);	
	}

	if (GetLastError() != NO_ERROR &&
		GetLastError() != ERROR_NO_MORE_ITEMS){
		// Insert error handling here.
		return 0;
	}

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return 1;
}

PDEV_NODE DevTreeInfo::CreateDevNode(CString _devDesc, CString _devPath, CString _devPDO, PDEV_NODE _parentDevNode){
	PDEV_NODE newDevNode;
	//newDevNode = (PDEV_NODE)malloc(sizeof(DEV_NODE));
	newDevNode = new DEV_NODE;
	newDevNode->dChildNodeNum = 0;
	
	newDevNode->sDevDesc = _devDesc;
	newDevNode->sPDO = _devPDO;
	newDevNode->sPath = _devPath;
	newDevNode->pParentNode = _parentDevNode;
	if (newDevNode->pParentNode == NULL){
		return newDevNode;
	}
	else{
		if (newDevNode->pParentNode->dChildNodeNum == MAX_DEV_NODE_CHILDNUM - 1){
			TRACE(_T("OVER FLOW\n"));
			return NULL;
		}
		newDevNode->pParentNode->rChildNodes[_parentDevNode->dChildNodeNum] = newDevNode;
		newDevNode->pParentNode->dChildNodeNum++;

		return newDevNode;
	}
	
}

void DevTreeInfo::DestroyDevNode(PDEV_NODE _pDevNode){

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->DestroyDevNode(_pDevNode->rChildNodes[i]);
	}

	TRACE(_T("D : %s\n"), _pDevNode->sDevDesc);
	delete _pDevNode;
}

void DevTreeInfo::CreateNodeWithDevPath(CString _devDesc, CString _devPath, CString _devPDO){
	PDEV_NODE curTreePos = this->rootNode;
	BOOL isAreadyExist = FALSE;

	_devPath.AppendFormat(_T("%s"), "#");

	for (int i = 0; i < _devPath.GetLength(); i++){
		if (_devPath[i] == (wchar_t)'#'){

			//Check
			for (int j = 0; j < curTreePos->dChildNodeNum; j++){
				if (curTreePos->rChildNodes[j]->sPath.Compare(_devPath.Left(i)) == 0){
					curTreePos = curTreePos->rChildNodes[j];

					if (i == _devPath.GetLength() - 1){
						curTreePos->sDevDesc = _devDesc;
						curTreePos->sPDO = _devPDO;
					}

					isAreadyExist = TRUE;
				}
			}

			if (isAreadyExist == FALSE){
				if (i != _devPath.GetLength() - 1){
					curTreePos = this->CreateDevNode(_T(""), _devPath.Left(i), _devPDO, curTreePos);
				}
				else{
					curTreePos = this->CreateDevNode(_devDesc, _devPath.Left(i), _devPDO, curTreePos);
				}
			}
			else{
				isAreadyExist = FALSE;
			}
		}
	}
}

void DevTreeInfo::TraversalDevTree(PDEV_NODE _pDevNode){

	//TRACE(_pDevNode->sPath);

	if (_pDevNode->dChildNodeNum == 0){
		return;
	}

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->TraversalDevTree(_pDevNode->rChildNodes[i]);
	}
}

void DevTreeInfo::FindCheckedNodes(PDEV_NODE _pDevNode, CTreeCtrl* _pTreeView, PCAP_SPECIFIC_DEV_INFO _pCapDevListInfo){

	//TRACE(_pDevNode->sPath);

	if (_pTreeView->GetCheck(_pDevNode->hTreeItem)){
		if (_pCapDevListInfo->numOfSpecificCapDev == 0){
			wcscpy_s(_pCapDevListInfo->specificDevPdoNameList, (const WCHAR *)_pDevNode->sPDO);
		}
		else{
			wcscat_s(_pCapDevListInfo->specificDevPdoNameList, (const WCHAR *)_pDevNode->sPDO);
		}
		_pCapDevListInfo->header[_pCapDevListInfo->numOfSpecificCapDev] = _pDevNode->sPDO.GetLength();
		_pCapDevListInfo->numOfSpecificCapDev++;
	}

	if (_pDevNode->dChildNodeNum == 0){
		return;
	}

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->FindCheckedNodes(_pDevNode->rChildNodes[i], _pTreeView, _pCapDevListInfo);
	}

}

void DevTreeInfo::DrawDevTree(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode){
	TVINSERTSTRUCT tvi;

	if (_pDevNode->pParentNode == NULL){
		tvi.hParent = TVI_ROOT;
	}else{
		tvi.hParent = _pDevNode->pParentNode->hTreeItem;
	}

	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT;

	if (_pDevNode->sDevDesc.Compare(_T("")) == 0){
		tvi.item.pszText = (LPWSTR)(LPCWSTR)_pDevNode->sPath;
	}else{
		tvi.item.pszText = (LPWSTR)(LPCWSTR)_pDevNode->sDevDesc;
	}
	_pDevNode->hTreeItem = _pTreeView->InsertItem(&tvi);

	if (_pDevNode->dChildNodeNum == 0){
		return;
	}

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->DrawDevTree(_pTreeView, _pDevNode->rChildNodes[i]);
	}
}

void DevTreeInfo::SetCheckBoxOnWithSpecificDevInfo(CTreeCtrl* _pTreeView, PCAP_SPECIFIC_DEV_INFO pSpecificDevInfo, bool toggle){
	unsigned int prevIdx = 0;
	WCHAR targetPdoName[MAX_SIZE_CAP_DEV_LIST] = { 0, };

	TRACE(pSpecificDevInfo->specificDevPdoNameList);

	for (unsigned int i = 0; i < pSpecificDevInfo->numOfSpecificCapDev; i++){
		
		memcpy(targetPdoName, &(pSpecificDevInfo->specificDevPdoNameList[prevIdx]), sizeof(WCHAR) * (pSpecificDevInfo->header[i] + 1));
		targetPdoName[pSpecificDevInfo->header[i]] = L'\0';
		prevIdx += pSpecificDevInfo->header[i];

		this->ToggleSpecificDevNode(_pTreeView, this->rootNode, targetPdoName, toggle);

		memset(targetPdoName, 0, sizeof(WCHAR) * (pSpecificDevInfo->header[i] + 1));
	}
	
}

void DevTreeInfo::ToggleSpecificDevNode(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode, PWCHAR pTargetPdoName, bool toggle){
	CString tragetPdoName;
	tragetPdoName.Format(_T("%s"), pTargetPdoName);

	if (_pDevNode->sPDO.Compare(tragetPdoName) == 0 && _pDevNode->sDevDesc.Compare(_T(""))){
		TRACE(_pDevNode->sDevDesc);
		_pTreeView->SetCheck(_pDevNode->hTreeItem, toggle);
	}

	if (_pDevNode->dChildNodeNum == 0){
		return;
	}

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->ToggleSpecificDevNode(_pTreeView, _pDevNode->rChildNodes[i], pTargetPdoName, toggle);
	}
}


void DevTreeInfo::ExpandDevTree(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode){

	_pTreeView->Expand(_pDevNode->hTreeItem, TVE_EXPAND);

	if (_pDevNode->dChildNodeNum == 0){
		return;
	}

	for (int i = 0; i < _pDevNode->dChildNodeNum; i++){
		this->ExpandDevTree(_pTreeView, _pDevNode->rChildNodes[i]);
	}
}


void DevTreeInfo::ResetDevNode(){
	this->DestroyDevNode(this->rootNode);
	this->rootNode = this->CreateDevNode(_T(""), _T(""), _T(""), NULL);
	this->rootNode->sDevDesc.Format(_T("%S"), "USB_CLASS");
}

PDEV_NODE DevTreeInfo::GetRootDevNode(){
	return this->rootNode;
}