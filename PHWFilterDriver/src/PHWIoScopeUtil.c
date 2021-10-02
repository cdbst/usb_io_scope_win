#include <PHWIoScopeUtil.h>


PDEVICE_OBJECT PHWIoScopeGetTopDevObj(PDEVICE_OBJECT pDevObj){
	PDEVICE_OBJECT finder = pDevObj;

	while(finder->AttachedDevice != NULL){
		finder = finder->AttachedDevice;
	}

	return finder;
}


