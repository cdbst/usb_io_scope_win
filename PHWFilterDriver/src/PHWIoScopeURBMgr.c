#include "PHWIoScopeURBMgr.h"

BOOLEAN PHWIoScopeExpelURBData(PURB pURB, PPKT_INFO pDestination){
	USB_SETUP_PACKET usbSetupPkt;
	BOOLEAN isExpeled;

	isExpeled = 0;
	
	if(pURB == NULL){
		return 0;
	}

	if(!pURB->UrbHeader.Length){
		return 0;
	}

	RtlZeroMemory(&usbSetupPkt, sizeof(USB_SETUP_PACKET));
	
	switch(pURB->UrbHeader.Function){
	case URB_FUNCTION_SELECT_CONFIGURATION:
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		
		if(pDestination->direction != 0){
			return 0;
		}
		
		usbSetupPkt.bRequest = USB_REQUEST_SET_CONFIGURATION;
		usbSetupPkt.wValue.W = pURB->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue;
		usbSetupPkt.wIndex.W = 0x0000;
		usbSetupPkt.wLength = 0x0000;
		usbSetupPkt.bmRequestType.B = 0x00;

		RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
		pDestination->dataSize = sizeof(USB_SETUP_PACKET);
		
		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		isExpeled = 1;
		break;
		
	case URB_FUNCTION_SELECT_INTERFACE:
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		if(pDestination->direction != 0){
			return 0;
		}
		usbSetupPkt.bRequest = USB_REQUEST_SET_INTERFACE;
		usbSetupPkt.wValue.W = pURB->UrbSelectInterface.Interface.AlternateSetting;
		usbSetupPkt.wIndex.W = pURB->UrbSelectInterface.Interface.InterfaceNumber;
		usbSetupPkt.wLength = 0x0000;
		usbSetupPkt.bmRequestType.B = 0x01;

		RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
		pDestination->dataSize = sizeof(USB_SETUP_PACKET);

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		isExpeled = 1;
		break;


	//case URB_FUNCTION_RESET_PIPE: // same with URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL
		//break;
		
	case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
	case URB_FUNCTION_SYNC_CLEAR_STALL:
	
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		if(pDestination->direction != 0){
			return 0;
		}
		
		usbSetupPkt.bmRequestType.B = 0x02;
		usbSetupPkt.bRequest = USB_REQUEST_CLEAR_FEATURE;
		usbSetupPkt.wValue.W = USB_FEATURE_ENDPOINT_STALL;
		usbSetupPkt.wIndex.W = (USHORT)((PUSB_ENDPOINT_DESCRIPTOR)pURB->UrbPipeRequest.PipeHandle)->bEndpointAddress;
		usbSetupPkt.wLength = 0x0000;
	
		break;
		
	case URB_FUNCTION_SYNC_RESET_PIPE:
		break;

	case URB_FUNCTION_ABORT_PIPE:
		break;
	case URB_FUNCTION_CLOSE_STATIC_STREAMS:
		break;


		
	case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL: // DoNot Use if host pc 2000 and later
		//아직 방법 못 찾음.
		break;
	case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:// DoNot Use if host pc 2000 and later
		//아직 방법 못 찾음.
		break;
	case URB_FUNCTION_GET_FRAME_LENGTH: // DoNot Use if host pc 2000 and later
		//아직 방법 못 찾음.
		break;
	case URB_FUNCTION_SET_FRAME_LENGTH: // DoNot Use if host pc 2000 and later
		//아직 방법 못 찾음.
		break;
	case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
		//아직 방법 못 찾음.
		break;


	case URB_FUNCTION_CONTROL_TRANSFER:
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		
		if(pDestination->direction == 0){
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);
			RtlCopyMemory(pDestination->data, pURB->UrbControlTransfer.SetupPacket, sizeof(pURB->UrbControlTransfer.SetupPacket));

			if( (pURB->UrbControlTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_OUT) && pURB->UrbControlTransfer.TransferBufferLength > 0){
				pDestination->isMultiplePkt = 1;
				pDestination->dataDivisionIdx = sizeof(USB_SETUP_PACKET);
				pDestination->dataSize += pURB->UrbControlTransfer.TransferBufferLength;


				if(pURB->UrbControlTransfer.TransferBuffer){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], pURB->UrbControlTransfer.TransferBuffer, pURB->UrbControlTransfer.TransferBufferLength);
				}else if(pURB->UrbControlTransfer.TransferBufferMDL){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], MmGetSystemAddressForMdlSafe(pURB->UrbControlTransfer.TransferBufferMDL, NormalPagePriority), pURB->UrbControlTransfer.TransferBufferLength);
				}

			}
			
			isExpeled = 1;
			
		}else{
			if( (pURB->UrbControlTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) && pURB->UrbControlTransfer.TransferBufferLength > 0){
				pDestination->dataSize = pURB->UrbControlTransfer.TransferBufferLength;
				
				if(pURB->UrbControlTransfer.TransferBuffer){
					RtlCopyMemory(&pDestination->data, pURB->UrbControlTransfer.TransferBuffer, pURB->UrbControlTransfer.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlTransfer.TransferBufferMDL){
					RtlCopyMemory(&pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlTransfer.TransferBufferMDL, NormalPagePriority), pURB->UrbControlTransfer.TransferBufferLength);
					isExpeled = 1;
				}
			}	
		}

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		break;
		
	case URB_FUNCTION_CONTROL_TRANSFER_EX:
		//KdPrint(("[%d] Get URB_FUNCTION_CONTROL_TRANSFER_EX\n", pDestination->direction));
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		
		if(pDestination->direction == 0){
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);
			RtlCopyMemory(pDestination->data, pURB->UrbControlTransferEx.SetupPacket, sizeof(pURB->UrbControlTransferEx.SetupPacket));

			if( (pURB->UrbControlTransferEx.TransferFlags & USBD_TRANSFER_DIRECTION_OUT) && pURB->UrbControlTransferEx.TransferBufferLength > 0){
				pDestination->isMultiplePkt = 1;
				pDestination->dataDivisionIdx = sizeof(USB_SETUP_PACKET);
				pDestination->dataSize += pURB->UrbControlTransferEx.TransferBufferLength;
				
				if(pURB->UrbControlTransferEx.TransferBuffer){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], pURB->UrbControlTransferEx.TransferBuffer, pURB->UrbControlTransferEx.TransferBufferLength);
				}else if(pURB->UrbControlTransferEx.TransferBufferMDL){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], MmGetSystemAddressForMdlSafe(pURB->UrbControlTransferEx.TransferBufferMDL, NormalPagePriority), pURB->UrbControlTransferEx.TransferBufferLength);
				}
			}

			isExpeled = 1;
			
		}else{
			if( (pURB->UrbControlTransferEx.TransferFlags & USBD_TRANSFER_DIRECTION_IN) && pURB->UrbControlTransferEx.TransferBufferLength > 0){
				pDestination->dataSize = pURB->UrbControlTransferEx.TransferBufferLength;
				
				if(pURB->UrbControlTransferEx.TransferBuffer){
					RtlCopyMemory(&pDestination->data, pURB->UrbControlTransferEx.TransferBuffer, pURB->UrbControlTransferEx.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlTransferEx.TransferBufferMDL){
					RtlCopyMemory(&pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlTransferEx.TransferBufferMDL, NormalPagePriority), pURB->UrbControlTransferEx.TransferBufferLength);
					isExpeled = 1;
				}
			}
		}

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		
		break;
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
		
		if(pDestination->direction == 0){
			if(!(globals.inf_filter & INFORMATION_FILTER2_OUT)){
				return 0;
			}
		}else{
			if(!(globals.inf_filter & INFORMATION_FILTER1_IN)){
				return 0;
			}
		}
		
		pDestination->dataSize = pURB->UrbBulkOrInterruptTransfer.TransferBufferLength;
		
		if(pURB->UrbBulkOrInterruptTransfer.TransferBuffer){
			RtlCopyMemory(pDestination->data, pURB->UrbBulkOrInterruptTransfer.TransferBuffer, pURB->UrbBulkOrInterruptTransfer.TransferBufferLength);
			isExpeled = 1;
		}else if(pURB->UrbBulkOrInterruptTransfer.TransferBufferMDL){
			RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbBulkOrInterruptTransfer.TransferBufferMDL, NormalPagePriority), pURB->UrbBulkOrInterruptTransfer.TransferBufferLength);
			isExpeled = 1;
		}
		
		break;
		
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL:
		KdPrint(("URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL\n"));
		break;
		
		
	case URB_FUNCTION_ISOCH_TRANSFER:
		if(pDestination->direction == 0){
			if(!(globals.inf_filter & INFORMATION_FILTER2_OUT)){
				return 0;
			}
		}else{
			if(!(globals.inf_filter & INFORMATION_FILTER1_IN)){
				return 0;
			}
		}
		
		pDestination->dataSize = pURB->UrbIsochronousTransfer.TransferBufferLength;
		
		if(pURB->UrbIsochronousTransfer.TransferBuffer){
			RtlCopyMemory(pDestination->data, pURB->UrbIsochronousTransfer.TransferBuffer, pURB->UrbIsochronousTransfer.TransferBufferLength);
			isExpeled = 1;
		}else if(pURB->UrbIsochronousTransfer.TransferBufferMDL){
			RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbIsochronousTransfer.TransferBufferMDL, NormalPagePriority), pURB->UrbIsochronousTransfer.TransferBufferLength);
			isExpeled = 1;
		}
		
		break;
		
	case URB_FUNCTION_ISOCH_TRANSFER_USING_CHAINED_MDL:
		break;


		
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}

		if(pDestination->direction == 0){
			usbSetupPkt.bmRequestType.B = 0x80;
			usbSetupPkt.bRequest = USB_REQUEST_GET_DESCRIPTOR;
			usbSetupPkt.wValue.LowByte = pURB->UrbControlDescriptorRequest.Index;
			usbSetupPkt.wValue.HiByte = pURB->UrbControlDescriptorRequest.DescriptorType;
			usbSetupPkt.wIndex.W = pURB->UrbControlDescriptorRequest.LanguageId;
			usbSetupPkt.wLength = (USHORT)pURB->UrbControlDescriptorRequest.TransferBufferLength;
			

			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);

			pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
			isExpeled = 1;
			
		}else{
		
			if(pURB->UrbControlDescriptorRequest.TransferBufferLength > 0){
				if(pURB->UrbControlDescriptorRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbControlDescriptorRequest.TransferBuffer, pURB->UrbControlDescriptorRequest.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlDescriptorRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlDescriptorRequest.TransferBufferMDL, NormalPagePriority), pURB->UrbControlDescriptorRequest.TransferBufferLength);
					isExpeled = 1;

				}
				pDestination->dataSize = pURB->UrbControlDescriptorRequest.TransferBufferLength;
				
			}
		}
		
		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		
		break;

	case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}

		if(pDestination->direction != 0){
			return 0;
		}
		
		usbSetupPkt.bmRequestType.B = 0x00;
		usbSetupPkt.bRequest = USB_REQUEST_SET_DESCRIPTOR;
		usbSetupPkt.wValue.LowByte = pURB->UrbControlDescriptorRequest.Index;
		usbSetupPkt.wValue.HiByte = pURB->UrbControlDescriptorRequest.DescriptorType;
		usbSetupPkt.wIndex.W = pURB->UrbControlDescriptorRequest.LanguageId;
		usbSetupPkt.wLength = (USHORT)pURB->UrbControlDescriptorRequest.TransferBufferLength;

		RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
		pDestination->dataSize = sizeof(USB_SETUP_PACKET);

		if(pURB->UrbControlDescriptorRequest.TransferBufferLength > 0){
			pDestination->isMultiplePkt = 1;
			pDestination->dataDivisionIdx = pDestination->dataSize;
			pDestination->dataSize += pURB->UrbControlDescriptorRequest.TransferBufferLength;
			
			if(pURB->UrbControlDescriptorRequest.TransferBuffer){
				RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], pURB->UrbControlDescriptorRequest.TransferBuffer, pURB->UrbControlDescriptorRequest.TransferBufferLength);
			}else if(pURB->UrbControlDescriptorRequest.TransferBufferMDL){
				RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], MmGetSystemAddressForMdlSafe(pURB->UrbControlDescriptorRequest.TransferBufferMDL, NormalPagePriority), pURB->UrbControlDescriptorRequest.TransferBufferLength);

			}
			
			pDestination->dataSize = pURB->UrbControlDescriptorRequest.TransferBufferLength;	
		}
		
		isExpeled = 1;
		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		break;
		
	case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
	case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
	case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
	case URB_FUNCTION_SET_FEATURE_TO_OTHER:
	case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
	case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
	case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
	case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:	
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}

		if(pDestination->direction != 0){
			return 0;
		}
		
		switch(pURB->UrbHeader.Function){
		case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
			usbSetupPkt.bmRequestType.B = 0x80;
			usbSetupPkt.bRequest = USB_REQUEST_SET_FEATURE;
			break;
		case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
			usbSetupPkt.bmRequestType.B = 0x81;
			usbSetupPkt.bRequest = USB_REQUEST_SET_FEATURE;
			break;
		case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
			usbSetupPkt.bmRequestType.B = 0x82;
			usbSetupPkt.bRequest = USB_REQUEST_SET_FEATURE;
			break;
		case URB_FUNCTION_SET_FEATURE_TO_OTHER:
			usbSetupPkt.bmRequestType.B = 0x83;
			usbSetupPkt.bRequest = USB_REQUEST_SET_FEATURE;
			break;
		case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
			usbSetupPkt.bmRequestType.B = 0x00;
			usbSetupPkt.bRequest = USB_REQUEST_CLEAR_FEATURE;
			break;
		case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
			usbSetupPkt.bmRequestType.B = 0x01;
			usbSetupPkt.bRequest = USB_REQUEST_CLEAR_FEATURE;
			break;
		case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
			usbSetupPkt.bmRequestType.B = 0x02;
			usbSetupPkt.bRequest = USB_REQUEST_CLEAR_FEATURE;
			break;
		case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
			usbSetupPkt.bmRequestType.B = 0x03;
			usbSetupPkt.bRequest = USB_REQUEST_CLEAR_FEATURE;
			break;
		}

		usbSetupPkt.wValue.W = pURB->UrbControlFeatureRequest.FeatureSelector; // URB의 해당맴버가 존재하지 않는다고 나옴. MSDN에는 존재하는데 말이다..
		usbSetupPkt.wIndex.W = pURB->UrbControlFeatureRequest.Index;
		
		RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
		pDestination->dataSize = sizeof(USB_SETUP_PACKET);

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		isExpeled = 1;
		break;

	case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
	case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
	case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
	case URB_FUNCTION_GET_STATUS_FROM_OTHER:
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		if(pDestination->direction == 0){
			switch(pURB->UrbHeader.Function){
			case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
				usbSetupPkt.bmRequestType.B = 0x80;
				break;
			case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
				usbSetupPkt.bmRequestType.B = 0x81;
				break;
			case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
				usbSetupPkt.bmRequestType.B = 0x82;
				break;
			case URB_FUNCTION_GET_STATUS_FROM_OTHER:
				usbSetupPkt.bmRequestType.B = 0x83;
				break;
			}
			
			usbSetupPkt.bRequest = USB_REQUEST_GET_STATUS;
			usbSetupPkt.wIndex.W = pURB->UrbControlGetStatusRequest.Index;
			ASSERT(pURB->UrbControlGetStatusRequest.TransferBufferLength == 2);
			usbSetupPkt.wLength = (USHORT)pURB->UrbControlGetStatusRequest.TransferBufferLength;

			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);

			isExpeled = 1;
			
		}else{
			if(pURB->UrbControlGetStatusRequest.TransferBufferLength == 2){
				if(pURB->UrbControlGetStatusRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbControlGetStatusRequest.TransferBuffer, 2);
					isExpeled = 1;
				}else if(pURB->UrbControlGetStatusRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlGetStatusRequest.TransferBufferMDL, NormalPagePriority), 2);
					isExpeled = 1;
				}
				pDestination->dataSize = 2;
			}
		}

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		break;
		
	case URB_FUNCTION_VENDOR_DEVICE:
	case URB_FUNCTION_VENDOR_INTERFACE:
	case URB_FUNCTION_VENDOR_ENDPOINT:
	case URB_FUNCTION_VENDOR_OTHER:
	case URB_FUNCTION_CLASS_DEVICE:
	case URB_FUNCTION_CLASS_INTERFACE:
	case URB_FUNCTION_CLASS_ENDPOINT:
	case URB_FUNCTION_CLASS_OTHER:
		KdPrint(("Vender of Class Specific Ctrl Transfer appeared! [0x%2x]\n", pURB->UrbHeader.Function));
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		
		if(pDestination->direction == 0){
			
			switch(pURB->UrbHeader.Function){
			case URB_FUNCTION_VENDOR_DEVICE:
				usbSetupPkt.bmRequestType.Type = 2;
				usbSetupPkt.bmRequestType.Recipient = 0;
				break;
			case URB_FUNCTION_VENDOR_INTERFACE:
				usbSetupPkt.bmRequestType.Type = 2;
				usbSetupPkt.bmRequestType.Recipient = 1;
				break;
			case URB_FUNCTION_VENDOR_ENDPOINT:
				usbSetupPkt.bmRequestType.Type = 2;
				usbSetupPkt.bmRequestType.Recipient = 2;				
				break;
			case URB_FUNCTION_VENDOR_OTHER:
				usbSetupPkt.bmRequestType.Type = 2;
				usbSetupPkt.bmRequestType.Recipient = 3;
				break;
			case URB_FUNCTION_CLASS_DEVICE:
				usbSetupPkt.bmRequestType.Type = 1;
				usbSetupPkt.bmRequestType.Recipient = 0;
				break;
			case URB_FUNCTION_CLASS_INTERFACE:
				usbSetupPkt.bmRequestType.Type = 1;
				usbSetupPkt.bmRequestType.Recipient = 1;
				break;
			case URB_FUNCTION_CLASS_ENDPOINT:
				usbSetupPkt.bmRequestType.Type = 1;
				usbSetupPkt.bmRequestType.Recipient = 2;
				break;
			case URB_FUNCTION_CLASS_OTHER:
				usbSetupPkt.bmRequestType.Type = 1;
				usbSetupPkt.bmRequestType.Recipient = 3;
				break;
			}

			
			usbSetupPkt.bmRequestType.Dir = pURB->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN ? 1 : 0;
			usbSetupPkt.bRequest = pURB->UrbControlVendorClassRequest.Request;
			usbSetupPkt.wValue.W = pURB->UrbControlVendorClassRequest.Value;
			usbSetupPkt.wLength = (USHORT)pURB->UrbControlVendorClassRequest.TransferBufferLength;

			pDestination->dataSize = sizeof(USB_SETUP_PACKET);
			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			isExpeled = 1;

			if( !(pURB->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN) && pURB->UrbControlVendorClassRequest.TransferBufferLength > 0){
				pDestination->isMultiplePkt = 1;
				pDestination->dataDivisionIdx = sizeof(USB_SETUP_PACKET);
				pDestination->dataSize += pURB->UrbControlVendorClassRequest.TransferBufferLength;

				if(pURB->UrbControlVendorClassRequest.TransferBuffer){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], pURB->UrbControlVendorClassRequest.TransferBuffer, pURB->UrbControlVendorClassRequest.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlVendorClassRequest.TransferBufferMDL){
					RtlCopyMemory(&pDestination->data[pDestination->dataDivisionIdx], MmGetSystemAddressForMdlSafe(pURB->UrbControlVendorClassRequest.TransferBufferMDL, NormalPagePriority), pURB->UrbControlVendorClassRequest.TransferBufferLength);
					isExpeled = 1;
				}	
			}
			
		}else{
			if( (pURB->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN) && pURB->UrbControlVendorClassRequest.TransferBufferLength > 0){
				pDestination->dataSize = pURB->UrbControlVendorClassRequest.TransferBufferLength;

				if(pURB->UrbControlVendorClassRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbControlVendorClassRequest.TransferBuffer, pURB->UrbControlVendorClassRequest.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlVendorClassRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlVendorClassRequest.TransferBufferMDL, NormalPagePriority), pURB->UrbControlVendorClassRequest.TransferBufferLength);
					isExpeled = 1;
				}
			}
			
		}

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		
		break;
		
	case URB_FUNCTION_GET_CONFIGURATION:
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}

		if(pDestination->direction == 0){
			usbSetupPkt.bRequest = USB_REQUEST_GET_CONFIGURATION;
			usbSetupPkt.bmRequestType.B = 0x80;
			usbSetupPkt.wLength = 1;

			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);
			
			isExpeled = 1;
			
		}else{
			if(pURB->UrbControlGetConfigurationRequest.TransferBufferLength == 1){
				if(pURB->UrbControlGetConfigurationRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbControlGetConfigurationRequest.TransferBuffer, 1);
					isExpeled = 1;
				}else if(pURB->UrbControlGetConfigurationRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlGetConfigurationRequest.TransferBufferMDL, NormalPagePriority), 1);
					isExpeled = 1;
				}
				pDestination->dataSize = 1;
			}
		}

		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;	
		break;
		
	case URB_FUNCTION_GET_INTERFACE:
		//KdPrint(("[%d] Get URB_FUNCTION_GET_INTERFACE\n", pDestination->direction));
		
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}
		if(pDestination->direction == 0){
			usbSetupPkt.bRequest = USB_REQUEST_GET_CONFIGURATION;
			usbSetupPkt.wIndex.W = pURB->UrbControlGetStatusRequest.Index;
			usbSetupPkt.bmRequestType.B = 0x80;
			usbSetupPkt.wLength = 1;

			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);
			
			isExpeled = 1;
			
		}else{
			if(pURB->UrbControlGetStatusRequest.TransferBufferLength == 1){
				if(pURB->UrbControlGetStatusRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbControlGetStatusRequest.TransferBuffer, 1);
					isExpeled = 1;
				}else if(pURB->UrbControlGetConfigurationRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbControlGetStatusRequest.TransferBufferMDL, NormalPagePriority), 1);
					isExpeled = 1;
				}
				pDestination->dataSize = 1;
			}
		}
		
		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		break;
		
	case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
		if(!(globals.inf_filter & INFORMATION_FILTER4_USBCTRL)){
			return 0;
		}

		if(pDestination->direction == 0){
			
			usbSetupPkt.bmRequestType.Dir = 1;
			usbSetupPkt.bmRequestType.Type = 2;
			usbSetupPkt.bmRequestType.Recipient = pURB->UrbOSFeatureDescriptorRequest.Recipient;
			
			usbSetupPkt.bRequest = USB_REQUEST_GET_DESCRIPTOR;

			usbSetupPkt.wValue.LowByte = pURB->UrbOSFeatureDescriptorRequest.InterfaceNumber;
			usbSetupPkt.wValue.HiByte = pURB->UrbOSFeatureDescriptorRequest.MS_PageIndex;

			usbSetupPkt.wIndex.W = pURB->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex;
			
			usbSetupPkt.wLength = (USHORT)pURB->UrbOSFeatureDescriptorRequest.TransferBufferLength;
			
			RtlCopyMemory(pDestination->data, &usbSetupPkt, sizeof(USB_SETUP_PACKET));
			pDestination->dataSize = sizeof(USB_SETUP_PACKET);

			pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
			isExpeled = 1;
			
		}else{
		
			if(pURB->UrbOSFeatureDescriptorRequest.TransferBufferLength > 0){
				if(pURB->UrbOSFeatureDescriptorRequest.TransferBuffer){
					RtlCopyMemory(pDestination->data, pURB->UrbOSFeatureDescriptorRequest.TransferBuffer, pURB->UrbOSFeatureDescriptorRequest.TransferBufferLength);
					isExpeled = 1;
				}else if(pURB->UrbControlDescriptorRequest.TransferBufferMDL){
					RtlCopyMemory(pDestination->data, MmGetSystemAddressForMdlSafe(pURB->UrbOSFeatureDescriptorRequest.TransferBufferMDL, NormalPagePriority), pURB->UrbOSFeatureDescriptorRequest.TransferBufferLength);
					isExpeled = 1;

				}
				pDestination->dataSize = pURB->UrbOSFeatureDescriptorRequest.TransferBufferLength;
			}
		}
		
		pDestination->dataMajorType = DATA_MAJOR_TYPE_USBCTRL;
		break;
	case URB_FUNCTION_OPEN_STATIC_STREAMS:
		break;
	default:
		//return STATUS_NOT_SUPPORTED;
		return 0;
	}

	if(isExpeled){
		pDestination->dataMinorType = (UCHAR)pURB->UrbHeader.Function;
	}

	return isExpeled;
	
}


