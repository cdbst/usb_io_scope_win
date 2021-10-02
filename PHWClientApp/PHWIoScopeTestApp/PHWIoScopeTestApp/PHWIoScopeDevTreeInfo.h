#ifndef __PHWIOSCOPEDEVTREEINFO_H__
#define __PHWIOSCOPEDEVTREEINFO_H__

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include "PHWIoScopePublic.h"

#define MAX_DEV_NODE 256
#define MAX_FLT_DEV_NUM 256
#define MAX_DEV_NODE_CHILDNUM 128

typedef struct _DEV_NODE{
	CString sDevDesc;// 표시명칭
	CString sPath;// 절대경로
	CString sPDO;
	unsigned short dChildNodeNum;
	struct _DEV_NODE* rChildNodes[MAX_DEV_NODE_CHILDNUM];
	struct _DEV_NODE* pParentNode;
	HTREEITEM hTreeItem;
}DEV_NODE, *PDEV_NODE;

class DevTreeInfo{
private:
	PDEV_NODE rootNode;
	UINT numOfNodes;
public:
	BOOL DiscoverDevNodes(const GUID *ClassGuid);
	PDEV_NODE CreateDevNode(CString _devDesc, CString _devPath, CString _devPDO, PDEV_NODE _parentDevNode);
	void DestroyDevNode(PDEV_NODE _pDevNode);
	void CreateNodeWithDevPath(CString _devDesc, CString _devPath, CString _devPDO);
	void TraversalDevTree(PDEV_NODE _pDevNode);
	void ExpandDevTree(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode);
	void DrawDevTree(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode);
	PDEV_NODE GetRootDevNode();
	void FindCheckedNodes(PDEV_NODE _pDevNode, CTreeCtrl* _pTreeView, PCAP_SPECIFIC_DEV_INFO _pCapDevListInfo);
	void SetCheckBoxOnWithSpecificDevInfo(CTreeCtrl* _pTreeView, PCAP_SPECIFIC_DEV_INFO pSpecificDevInfo, bool toggle);
	void ToggleSpecificDevNode(CTreeCtrl* _pTreeView, PDEV_NODE _pDevNode, PWCHAR pTargetPdoName, bool toggle);
	void ResetDevNode();
	DevTreeInfo();
	~DevTreeInfo();
};


#endif