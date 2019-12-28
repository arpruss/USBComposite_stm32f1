/*****************************f*************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * FIXME: this works on the STM32F1 USB peripherals, and probably no
 * place else. Nonportable bits really need to be factored out, and
 * the result made cleaner.
 */

#include "usb_generic.h"
#include "usb_multi_x360.h"

#include <string.h>

#include <libmaple/usb.h>
#include <libmaple/nvic.h>
#include <libmaple/delay.h>

/* Private headers */
#include "usb_lib_globals.h"
#include "usb_reg_map.h"

/* usb_lib headers */
#include "usb_type.h"
#include "usb_core.h"
#include "usb_def.h"

typedef enum _HID_REQUESTS
{
 
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,
 
  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
 
} HID_REQUESTS;

#define NUM_INTERFACES                  1
#define NUM_ENDPOINTS                   2

#define USB_ENDPOINT_IN(addr)           ((addr) | 0x80)
#define HID_ENDPOINT_INT 				1
#define USB_ENDPOINT_TYPE_INTERRUPT     0x03
 
#define HID_DESCRIPTOR_TYPE             0x21
 
#define REPORT_DESCRIPTOR               0x22


typedef struct
{
	uint8_t len;			// 9
	uint8_t dtype;			// 0x21
	uint8_t	versionL;		// 0x101
	uint8_t	versionH;		// 0x101
	uint8_t	country;
	uint8_t	numDesc;
	uint8_t	desctype;		// 0x22 report
	uint8_t	descLenL;
	uint8_t	descLenH;
} HIDDescriptor;

#define X360_INTERFACE_OFFSET 0
#define X360_INTERFACE_NUMBER (X360_INTERFACE_OFFSET+usbMultiX360Part.startInterface)
#define X360_ENDPOINT_TX 0
#define X360_ENDPOINT_RX 1

#define USB_X360_RX_ADDR(i) x360Endpoints[(i)*NUM_ENDPOINTS+X360_ENDPOINT_RX].pmaAddress
#define USB_X360_TX_ADDR(i) x360Endpoints[(i)*NUM_ENDPOINTS+X360_ENDPOINT_TX].pmaAddress
#define USB_X360_RX_ENDP(i) x360Endpoints[(i)*NUM_ENDPOINTS+X360_ENDPOINT_RX].address
#define USB_X360_TX_ENDP(i) x360Endpoints[(i)*NUM_ENDPOINTS+X360_ENDPOINT_TX].address

static void usb_multi_x360_clear(void);

static uint32 numControllers = USB_X360_MAX_CONTROLLERS;

static void x360DataRxCb(uint32 controller);
static void x360DataTxCb(uint32 controller);
static void x360DataTxCb0(void) { x360DataTxCb(0); }
static void x360DataRxCb0(void) { x360DataRxCb(0); }
static void x360DataTxCb1(void) { x360DataTxCb(1); }
static void x360DataRxCb1(void) { x360DataRxCb(1); }
static void x360DataTxCb2(void) { x360DataTxCb(2); }
static void x360DataRxCb2(void) { x360DataRxCb(2); }
static void x360DataTxCb3(void) { x360DataTxCb(3); }
static void x360DataRxCb3(void) { x360DataRxCb(3); }

static void x360Reset(void);
static RESULT x360DataSetup(uint8 request, uint8 interface);
static RESULT x360NoDataSetup(uint8 request, uint8 interface);

/*
 * Descriptors
 */
 
#if 0 
const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x3a,                    //   USAGE (Counted Buffer)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x3f,                    //     USAGE (Reserved)
    0x09, 0x3b,                    //     USAGE (Byte Count)
    0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x0c,                    //     USAGE_MINIMUM (Button 12)
    0x29, 0x0f,                    //     USAGE_MAXIMUM (Button 15)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x09,                    //     USAGE (Button 9)
    0x09, 0x0a,                    //     USAGE (Button 10)
    0x09, 0x07,                    //     USAGE (Button 7)
    0x09, 0x08,                    //     USAGE (Button 8)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x05,                    //     USAGE (Button 5)
    0x09, 0x06,                    //     USAGE (Button 6)
    0x09, 0x0b,                    //     USAGE (Button 11)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x04,                    //     USAGE_MAXIMUM (Button 4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x46, 0xff, 0x00,              //     PHYSICAL_MAXIMUM (255)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x32,                    //     USAGE (Z)
    0x09, 0x35,                    //     USAGE (Rz)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x16, 0x00, 0x80,              //     LOGICAL_MINIMUM (-32768)
    0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
    0x36, 0x00, 0x80,              //     PHYSICAL_MINIMUM (-32768)
    0x46, 0xff, 0x7f,              //     PHYSICAL_MAXIMUM (32767)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x05, 0x01,                    //       USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //       USAGE (X)
    0x09, 0x31,                    //       USAGE (Y)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x05, 0x01,                    //       USAGE_PAGE (Generic Desktop)
    0x09, 0x33,                    //       USAGE (Rx)
    0x09, 0x34,                    //       USAGE (Ry)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
#endif

typedef struct {
//    usb_descriptor_config_header Config_Header;
    usb_descriptor_interface     HID_Interface;
    uint8                        unknown_descriptor1[17];
    usb_descriptor_endpoint      DataInEndpoint;
    usb_descriptor_endpoint      DataOutEndpoint;
} __packed usb_descriptor_config;


#define MAX_POWER (100 >> 1)
static const usb_descriptor_config X360Descriptor_Config =
{
#if 0    
	.Config_Header = {
		.bLength 			  = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_descriptor_config),//0,
        .bNumInterfaces       = 0x01,
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
	},
#endif    
	
	.HID_Interface = {
		.bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = X360_INTERFACE_OFFSET,  // PATCH
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,  
        .bInterfaceClass    = 0xFF, 
        .bInterfaceSubClass = 0x5D,
        .bInterfaceProtocol = 0x01, 
        .iInterface         = 0x00,
	},
    
    .unknown_descriptor1 = { 
        17,33,0,1,1,37,129/* PATCH 0x80 | USB_X360_TX_ENDP */,20,0,0,0,0,19,2/* was 2, now PATCH: USB_X360_RX_ENDP */,8,0,0,
    }, 
	
	.DataInEndpoint = {
		.bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | 0),//PATCH: USB_X360_TX_ENDP
        .bmAttributes     = USB_EP_TYPE_INTERRUPT, 
        .wMaxPacketSize   = 0x20, 
        .bInterval        = 4, 
	},

    .DataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT | 0),//PATCH: USB_X360_RX_ENDP
        .bmAttributes     = USB_EP_TYPE_INTERRUPT, 
        .wMaxPacketSize   = 0x20, 
        .bInterval        = 8, 
    },
};

static USBEndpointInfo x360Endpoints[NUM_ENDPOINTS*USB_X360_MAX_CONTROLLERS] = {
    {
        .callback = x360DataTxCb0,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 1
    },
    {
        .callback = x360DataRxCb0,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 0,
    },
    {
        .callback = x360DataTxCb1,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 1
    },
    {
        .callback = x360DataRxCb1,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 0,
    },
    {
        .callback = x360DataTxCb2,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 1
    },
    {
        .callback = x360DataRxCb2,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 0,
    },
    {
        .callback = x360DataTxCb3,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 1
    },
    {
        .callback = x360DataRxCb3,
        .bufferSize = 0x20,
        .type = USB_EP_EP_TYPE_INTERRUPT, 
        .tx = 0,
    },
};

static void getX360PartDescriptor(uint8* _out);

USBCompositePart usbMultiX360Part = {
    .numInterfaces = USB_X360_MAX_CONTROLLERS * NUM_INTERFACES,
    .numEndpoints = sizeof(x360Endpoints)/sizeof(*x360Endpoints),
    .descriptorSize = USB_X360_MAX_CONTROLLERS * sizeof(X360Descriptor_Config),
    .getPartDescriptor = getX360PartDescriptor,
    .usbInit = NULL,
    .usbReset = x360Reset,
    .usbDataSetup = x360DataSetup,
    .usbNoDataSetup = x360NoDataSetup,
    .endpoints = x360Endpoints,
    .clear = usb_multi_x360_clear
};

static volatile struct controller_data {
    uint32 ProtocolValue;
    uint8* hidBufferRx;
    uint32 n_unsent_bytes;
    uint8 transmitting;
    void (*rumble_callback)(uint8 left, uint8 right);
    void (*led_callback)(uint8 pattern);
} controllers[USB_X360_MAX_CONTROLLERS] = {{0}};

#define OUT_BYTE(s,v) out[(uint8*)&(s.v)-(uint8*)&s]

static void getX360PartDescriptor(uint8* out) {
    for(uint32 i=0; i<numControllers; i++) {
        memcpy(out, &X360Descriptor_Config, sizeof(X360Descriptor_Config));
        // patch to reflect where the part goes in the descriptor
        OUT_BYTE(X360Descriptor_Config, HID_Interface.bInterfaceNumber) += usbMultiX360Part.startInterface+NUM_INTERFACES*i;
        uint8 rx_endp = USB_X360_RX_ENDP(i);
        uint8 tx_endp = USB_X360_TX_ENDP(i);
        OUT_BYTE(X360Descriptor_Config, DataOutEndpoint.bEndpointAddress) += rx_endp;
        OUT_BYTE(X360Descriptor_Config, DataInEndpoint.bEndpointAddress) += tx_endp;
        OUT_BYTE(X360Descriptor_Config, unknown_descriptor1[6]) = 0x80 | tx_endp;
        OUT_BYTE(X360Descriptor_Config, unknown_descriptor1[13]) = rx_endp;
        out += sizeof(X360Descriptor_Config);
    }
}


/*
 * Etc.
 */

static void usb_multi_x360_clear(void) {
    memset((void*)controllers, 0, sizeof controllers);
    numControllers = USB_X360_MAX_CONTROLLERS;
}

void usb_multi_x360_initialize_controller_data(uint32 _numControllers, uint8* buffers) {
    numControllers = _numControllers;
    
    for (uint32 i=0; i<numControllers; i++) {
        volatile struct controller_data* c = &controllers[i];
        c->hidBufferRx = buffers;
        buffers += USB_X360_RX_EPSIZE;
        c->rumble_callback = NULL;
        c->led_callback = NULL;
    }
    
    usbMultiX360Part.numInterfaces = NUM_INTERFACES * numControllers;
    usbMultiX360Part.descriptorSize = sizeof(X360Descriptor_Config) * numControllers;
    usbMultiX360Part.numEndpoints = NUM_ENDPOINTS * numControllers;
}


/*
 * HID interface
 */

void usb_multi_x360_set_rumble_callback(uint32 controller, void (*callback)(uint8 left, uint8 right)) {
    controllers[controller].rumble_callback = callback;
}

void usb_multi_x360_set_led_callback(uint32 controller, void (*callback)(uint8 pattern)) {
    controllers[controller].led_callback = callback;
}

uint8 usb_multi_x360_is_transmitting(uint32 controller) {
    return controllers[controller].transmitting;
}

/* This function is non-blocking.
 *
 * It copies data from a usercode buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 usb_multi_x360_tx(uint32 controller, const uint8* buf, uint32 len) {
    volatile struct controller_data* c = &controllers[controller];
    
    /* Last transmission hasn't finished, so abort. */
    if (usb_multi_x360_is_transmitting(controller)) {
        return 0;
    }

    /* We can only put USB_X360_TX_EPSIZE bytes in the buffer. */
    if (len > USB_X360_TX_EPSIZE) {
        len = USB_X360_TX_EPSIZE;
    }

    /* Queue bytes for sending. */
    if (len) {
        usb_copy_to_pma(buf, len, USB_X360_TX_ADDR(controller));
    }
    // We still need to wait for the interrupt, even if we're sending
    // zero bytes. (Sending zero-size packets is useful for flushing
    // host-side buffers.)
    usb_set_ep_tx_count(USB_X360_TX_ENDP(controller), len);
    c->n_unsent_bytes = len;
    c->transmitting = 1;
    usb_set_ep_tx_stat(USB_X360_TX_ENDP(controller), USB_EP_STAT_TX_VALID);

    return len;
}

static void x360DataRxCb(uint32 controller)
{
    volatile struct controller_data* c = &controllers[controller];
        
    uint32 rx_endp = USB_X360_RX_ENDP(controller);
	uint32 ep_rx_size = usb_get_ep_rx_count(rx_endp);
	// This copy won't overwrite unread bytes as long as there is 
	// enough room in the USB Rx buffer for next packet
	uint32 *src = usb_pma_ptr(USB_X360_RX_ADDR(controller));
    uint16 tmp = 0;
	uint8 val;
	uint32 i;
    
    volatile uint8* hidBufferRx = c->hidBufferRx;
    
	for (i = 0; i < ep_rx_size; i++) {
		if (i&1) {
			val = tmp>>8;
		} else {
			tmp = *src++;
			val = tmp&0xFF;
		}
		hidBufferRx[i] = val;
	}
    
    if (ep_rx_size == 3) {
        if (c->led_callback != NULL && hidBufferRx[0] == 1 && hidBufferRx[1] == 3)
            c->led_callback(hidBufferRx[2]);
    }
    else if (ep_rx_size == 8) {
        if (c->rumble_callback != NULL && hidBufferRx[0] == 0 && hidBufferRx[1] == 8)
            c->rumble_callback(hidBufferRx[3],hidBufferRx[4]);
    }
    usb_set_ep_rx_stat(rx_endp, USB_EP_STAT_RX_VALID);
}

/*
 * Callbacks
 */

static void x360DataTxCb(uint32 controller) {
    volatile struct controller_data* c = &controllers[controller];
    
    c->n_unsent_bytes = 0;
    c->transmitting = 0;
}


static uint8* HID_GetProtocolValue(uint32 controller, uint16 Length){
	if (Length == 0){
		pInformation->Ctrl_Info.Usb_wLength = 1;
		return NULL;
	} else {
		return (uint8 *)(&controllers[controller].ProtocolValue);
	}
}

static uint8* HID_GetProtocolValue0(uint16 n) { return HID_GetProtocolValue(0, n); }
static uint8* HID_GetProtocolValue1(uint16 n) { return HID_GetProtocolValue(1, n); }
static uint8* HID_GetProtocolValue2(uint16 n) { return HID_GetProtocolValue(2, n); }
static uint8* HID_GetProtocolValue3(uint16 n) { return HID_GetProtocolValue(3, n); }

static uint8* (*HID_GetProtocolValues[USB_X360_MAX_CONTROLLERS])(uint16 Length) = { HID_GetProtocolValue0, HID_GetProtocolValue1, HID_GetProtocolValue2, HID_GetProtocolValue3 };

static RESULT x360DataSetup(uint8 request, uint8 interface) {
    uint8* (*CopyRoutine)(uint16) = 0;
	
#if 0			
	if (request == GET_DESCRIPTOR
        && pInformation->USBwIndex0 == X360_INTERFACE_NUMBER?? && 
		&& (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))){
		if (pInformation->USBwValue1 == REPORT_DESCRIPTOR){
			CopyRoutine = HID_GetReportDescriptor;
		} else 
        if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE){
			CopyRoutine = HID_GetHIDDescriptor;
		}
		
	} /* End of GET_DESCRIPTOR */
	  /*** GET_PROTOCOL ***/
	else 
#endif            
    
    if((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) && request == GET_PROTOCOL) {
        CopyRoutine = HID_GetProtocolValues[interface / NUM_INTERFACES];
	}
    else {
		return USB_UNSUPPORT;
	}

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

static RESULT x360NoDataSetup(uint8 request, uint8 interface) {
	if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
		&& (request == SET_PROTOCOL)){
		controllers[interface / NUM_INTERFACES].ProtocolValue = pInformation->USBwValue0;
		return USB_SUCCESS;
	}else{
		return USB_UNSUPPORT;
	}
}

static void x360Reset(void) {
      /* Reset the RX/TX state */
    for (uint8 i = 0 ; i < numControllers ; i++) {
        volatile struct controller_data* c = &controllers[i];
        c->n_unsent_bytes = 0;
        c->transmitting = 0;
    }
}
