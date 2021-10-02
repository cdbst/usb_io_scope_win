#ifndef __PHWIOSCOPEUSBPKT_H_
#define __PHWIOSCOPEUSBPKT_H_

//
// USB 1.1: 9.4 Standard Device Requests, Table 9-4. Standard Request Codes
// USB 2.0: 9.4 Standard Device Requests, Table 9-4. Standard Request Codes
//
#define USB_REQUEST_GET_STATUS          0x00
#define USB_REQUEST_CLEAR_FEATURE       0x01
#define USB_REQUEST_SET_FEATURE         0x03
#define USB_REQUEST_SET_ADDRESS         0x05
#define USB_REQUEST_GET_DESCRIPTOR      0x06
#define USB_REQUEST_SET_DESCRIPTOR      0x07
#define USB_REQUEST_GET_CONFIGURATION   0x08
#define USB_REQUEST_SET_CONFIGURATION   0x09
#define USB_REQUEST_GET_INTERFACE       0x0A
#define USB_REQUEST_SET_INTERFACE       0x0B
#define USB_REQUEST_SYNC_FRAME          0x0C
//
// USB 3.0: 9.4 Standard Device Requests, Table 9-4. Standard Request Codes
//
#define USB_REQUEST_SET_SEL             0x30
#define USB_REQUEST_ISOCH_DELAY         0x31


// USB 1.1: 9.4 Standard Device Requests, Table 9-6. Standard Feature Selectors
//
//#define USB_FEATURE_ENDPOINT_STALL              0x00
//#define USB_FEATURE_REMOTE_WAKEUP               0x01
//
// USB 2.0: 9.4 Standard Device Requests, Table 9-6. Standard Feature Selectors
//
#define USB_FEATURE_TEST_MODE                   0x02
//
// USB 3.0: 9.4 Standard Device Requests, Table 9-6. Standard Feature Selectors
//
#define USB_FEATURE_FUNCTION_SUSPEND            0x00
#define USB_FEATURE_U1_ENABLE                   0x30
#define USB_FEATURE_U2_ENABLE                   0x31
#define USB_FEATURE_LTM_ENABLE                  0x32


typedef union _BM_REQ_TYPE {
    struct {
        UCHAR   Recipient:2;
        UCHAR   Reserved:3;
        UCHAR   Type:2;
        UCHAR   Dir:1;
    };
    UCHAR B;
} BM_REQ_TYPE, *PBM_REQ_TYPE;

typedef struct _USB_SETUP_PACKET {
    BM_REQ_TYPE bmRequestType;
    UCHAR bRequest;

    union _WVALUE {
        struct {
            UCHAR LowByte;
            UCHAR HiByte;
        };
        USHORT W;
    } wValue;

    union _WINDEX {
        struct {
            UCHAR LowByte;
            UCHAR HiByte;
        };
        USHORT W;
    } wIndex;
    USHORT wLength;
} USB_SETUP_PACKET, *PUSB_SETUP_PACKET;


#endif
