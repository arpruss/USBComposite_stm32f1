/******************************************************************************
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
#include "usb_x360.h"


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

u16 GetEPTxAddr(u8 bEpNum);

static uint32 ProtocolValue;

static void hidDataTxCb(void);
//static void hidDataRxCb(void);

static void usbInit(void);
static void usbReset(void);
static RESULT usbDataSetup(uint8 request);
static RESULT usbNoDataSetup(uint8 request);
static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting);
static uint8* usbGetDeviceDescriptor(uint16 length);
static uint8* usbGetConfigDescriptor(uint16 length);
static uint8* usbGetStringDescriptor(uint16 length);
static void usbSetConfiguration(void);
static void usbSetDeviceAddress(void);
//static RESULT HID_SetProtocol(void);
static uint8 *HID_GetProtocolValue(uint16 Length);
//static uint8 *HID_GetReportDescriptor(uint16 Length);
static uint8 *HID_GetHIDDescriptor(uint16 Length);

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

static const usb_descriptor_device usbHIDDescriptor_Device =
USB_X360_DECLARE_DEV_DESC(0x045e, 0x028e);

typedef struct {
    usb_descriptor_config_header Config_Header;
    usb_descriptor_interface     HID_Interface;
    uint8                        unknown_descriptor1[17];
    usb_descriptor_endpoint      DataInEndpoint;
    usb_descriptor_endpoint      DataOutEndpoint;
} __packed usb_descriptor_config;


#define MAX_POWER (100 >> 1)
static const usb_descriptor_config usbHIDDescriptor_Config =
{
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
	
	.HID_Interface = {
		.bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0x00,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = 0xFF, 
        .bInterfaceSubClass = 0x5D,
        .bInterfaceProtocol = 0x01, 
        .iInterface         = 0x00,
	},
    
    .unknown_descriptor1 = {
        17,33,0,1,1,37,129,20,0,0,0,0,19,2,8,0,0,
    }, 
	
	.DataInEndpoint = {
		.bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_X360_TX_ENDP),//0x81,//USB_X360_TX_ADDR,
        .bmAttributes     = 3, 
        .wMaxPacketSize   = 0x20, 
        .bInterval        = 4, 
	},

    .DataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT | USB_X360_RX_ENDP),
        .bmAttributes     = 3, 
        .wMaxPacketSize   = 0x20, 
        .bInterval        = 8, 
    },
};

/*
  String Descriptors:

  we may choose to specify any or none of the following string
  identifiers:

  iManufacturer:    LeafLabs
  iProduct:         Maple
  iSerialNumber:    NONE
  iConfiguration:   NONE
  iInterface(CCI):  NONE
  iInterface(DCI):  NONE

*/

/* Unicode language identifier: 0x0409 is US English */
static const usb_descriptor_string usbHIDDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

static const usb_descriptor_string usbHIDDescriptor_iManufacturer = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(8),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {'L', 0, 'e', 0, 'a', 0, 'f', 0,
                'L', 0, 'a', 0, 'b', 0, 's', 0},
};

static const usb_descriptor_string usbHIDDescriptor_iProduct = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(5),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'a', 0, 'p', 0, 'l', 0, 'e', 0},
};

static const usb_descriptor_string usbHIDDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(3),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'H', 0, 'I', 0, 'D', 0},
};

static ONE_DESCRIPTOR usbHIDDevice_Descriptor = {
    (uint8*)&usbHIDDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR usbHIDConfig_Descriptor = {
    (uint8*)&usbHIDDescriptor_Config,
    sizeof(usbHIDDescriptor_Config)
};

#if 0 
static ONE_DESCRIPTOR HID_Report_Descriptor = {
    (uint8*)&hid_report_descriptor,
    sizeof(hid_report_descriptor)
};
#endif

#define N_STRING_DESCRIPTORS 4
static ONE_DESCRIPTOR usbHIDString_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbHIDDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbHIDDescriptor_iManufacturer,USB_DESCRIPTOR_STRING_LEN(8)},
    {(uint8*)&usbHIDDescriptor_iProduct,     USB_DESCRIPTOR_STRING_LEN(5)},
    {(uint8*)&usbHIDDescriptor_iInterface,     USB_DESCRIPTOR_STRING_LEN(3)}
};

/*
 * Etc.
 */

/* I/O state */

#define HID_BUFFER_SIZE	512

/* Received data */
static volatile uint8 hidBufferRx[HID_BUFFER_SIZE];
/* Read index into hidBufferRx */
static volatile uint32 rx_offset = 0;
/* Number of bytes left to transmit */
static volatile uint32 n_unsent_bytes = 0;
/* Are we currently sending an IN packet? */
static volatile uint8 transmitting = 0;
/* Number of unread bytes */
static volatile uint32 n_unread_bytes = 0;

/*
 * Endpoint callbacks
 */

static void (*ep_int_in[7])(void) =
    {hidDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
    {NOP_Process,
     NOP_Process, // hidDataRxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

/*
 * Globals required by usb_lib/
 *
 * Override weak definitions in the core.
 */

#define NUM_ENDPTS                0x03
static DEVICE my_Device_Table = {
    .Total_Endpoint      = NUM_ENDPTS,
    .Total_Configuration = 1
};

#define MAX_PACKET_SIZE            0x40  /* 64B, maximum for USB FS Devices */
static DEVICE_PROP my_Device_Property = {
    .Init                        = usbInit,
    .Reset                       = usbReset,
    .Process_Status_IN           = NOP_Process,
    .Process_Status_OUT          = NOP_Process,
    .Class_Data_Setup            = usbDataSetup,
    .Class_NoData_Setup          = usbNoDataSetup,
    .Class_Get_Interface_Setting = usbGetInterfaceSetting,
    .GetDeviceDescriptor         = usbGetDeviceDescriptor,
    .GetConfigDescriptor         = usbGetConfigDescriptor,
    .GetStringDescriptor         = usbGetStringDescriptor,
    .RxEP_buffer                 = NULL,
    .MaxPacketSize               = MAX_PACKET_SIZE
};

static USER_STANDARD_REQUESTS my_User_Standard_Requests = {
    .User_GetConfiguration   = NOP_Process,
    .User_SetConfiguration   = usbSetConfiguration,
    .User_GetInterface       = NOP_Process,
    .User_SetInterface       = NOP_Process,
    .User_GetStatus          = NOP_Process,
    .User_ClearFeature       = NOP_Process,
    .User_SetEndPointFeature = NOP_Process,
    .User_SetDeviceFeature   = NOP_Process,
    .User_SetDeviceAddress   = usbSetDeviceAddress
};

/*
 * HID interface
 */

void x360_enable(void) {
    usb_generic_enable(&my_Device_Table, &my_Device_Property, &my_User_Standard_Requests, ep_int_in, ep_int_out);
}

void x360_disable(void) {
    usb_generic_disable();
}

void x360_putc(char ch) {
    while (!x360_tx((uint8*)&ch, 1))
        ;
}

static void usb_copy_to_pma(const uint8 *buf, uint16 len, uint16 pma_offset) {
    uint16 *dst = (uint16*)usb_pma_ptr(pma_offset);
    uint16 n = len >> 1;
    uint16 i;
    for (i = 0; i < n; i++) {
        *dst = (uint16)(*buf) | *(buf + 1) << 8;
        buf += 2;
        dst += 2;
    }
    if (len & 1) {
        *dst = *buf;
    }
}

#if 0
static void usb_copy_from_pma(uint8 *buf, uint16 len, uint16 pma_offset) {
    uint32 *src = (uint32*)usb_pma_ptr(pma_offset);
    uint16 *dst = (uint16*)buf;
    uint16 n = len >> 1;
    uint16 i;
    for (i = 0; i < n; i++) {
        *dst++ = *src++;
    }
    if (len & 1) {
        *dst = *src & 0xFF;
    }
}
#endif

/* This function is non-blocking.
 *
 * It copies data from a usercode buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 x360_tx(const uint8* buf, uint32 len) {
    /* Last transmission hasn't finished, so abort. */
    if (x360_is_transmitting()) {
        return 0;
    }

    /* We can only put USB_X360_TX_EPSIZE bytes in the buffer. */
    if (len > USB_X360_TX_EPSIZE) {
        len = USB_X360_TX_EPSIZE;
    }

    /* Queue bytes for sending. */
    if (len) {
        usb_copy_to_pma(buf, len, GetEPTxAddr(USB_X360_TX_ENDP));//USB_X360_TX_ADDR);
    }
    // We still need to wait for the interrupt, even if we're sending
    // zero bytes. (Sending zero-size packets is useful for flushing
    // host-side buffers.)
    usb_set_ep_tx_count(USB_X360_TX_ENDP, len);
    n_unsent_bytes = len;
    transmitting = 1;
    usb_set_ep_tx_stat(USB_X360_TX_ENDP, USB_EP_STAT_TX_VALID);

    return len;
}

uint32 x360_data_available(void) {
    return n_unread_bytes;
}

uint8 x360_is_transmitting(void) {
    return transmitting;
}

uint16 x360_get_pending(void) {
    return n_unsent_bytes;
}

#if 0
/* Nonblocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 x360_rx(uint8* buf, uint32 len) {
    /* Copy bytes to buffer. */
    uint32 n_copied = x360_peek(buf, len);

    /* Mark bytes as read. */
    n_unread_bytes -= n_copied;
    rx_offset = (rx_offset + n_copied) % HID_BUFFER_SIZE;

    /* If all bytes have been read, re-enable the RX endpoint, which
     * was set to NAK when the current batch of bytes was received. */
    if (n_unread_bytes <= (HID_BUFFER_SIZE - USB_X360_RX_EPSIZE)) {
        usb_set_ep_rx_count(USB_X360_RX_ENDP, USB_X360_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_X360_RX_ENDP, USB_EP_STAT_RX_VALID);
		rx_offset = 0;
    }

    return n_copied;
}
#endif

/*
 * Callbacks
 */

static void hidDataTxCb(void) {
    n_unsent_bytes = 0;
    transmitting = 0;
}

#if 0
static void hidDataRxCb(void) {
	uint32 ep_rx_size;
	uint32 tail = (rx_offset + n_unread_bytes) % HID_BUFFER_SIZE;
	uint8 ep_rx_data[USB_X360_RX_EPSIZE];
	uint32 i;

    usb_set_ep_rx_stat(USB_X360_RX_ENDP, USB_EP_STAT_RX_NAK);
    ep_rx_size = usb_get_ep_rx_count(USB_X360_RX_ENDP);
    /* This copy won't overwrite unread bytes, since we've set the RX
     * endpoint to NAK, and will only set it to VALID when all bytes
     * have been read. */
    usb_copy_from_pma((uint8*)ep_rx_data, ep_rx_size,
                      USB_X360_RX_ADDR);

	for (i = 0; i < ep_rx_size; i++) {
		hidBufferRx[tail] = ep_rx_data[i];
		tail = (tail + 1) % HID_BUFFER_SIZE;
	}

	n_unread_bytes += ep_rx_size;

    if (n_unread_bytes <= (HID_BUFFER_SIZE - USB_X360_RX_EPSIZE)) {
        usb_set_ep_rx_count(USB_X360_RX_ENDP, USB_X360_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_X360_RX_ENDP, USB_EP_STAT_RX_VALID);
    }
}
#endif

static void usbInit(void) {
    pInformation->Current_Configuration = 0;

    USB_BASE->CNTR = USB_CNTR_FRES;

    USBLIB->irq_mask = 0;
    USB_BASE->CNTR = USBLIB->irq_mask;
    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
    USB_BASE->CNTR = USBLIB->irq_mask;

    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_ISR_MSK;
    USB_BASE->CNTR = USBLIB->irq_mask;

    nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
    USBLIB->state = USB_UNCONNECTED;
}

#define BTABLE_ADDRESS        0x00
static void usbReset(void) {
    pInformation->Current_Configuration = 0;

    /* current feature is current bmAttributes */
    pInformation->Current_Feature = (USB_CONFIG_ATTR_BUSPOWERED |
                                     USB_CONFIG_ATTR_SELF_POWERED);

    USB_BASE->BTABLE = BTABLE_ADDRESS;

    /* setup control endpoint 0 */
    usb_set_ep_type(USB_EP0, USB_EP_EP_TYPE_CONTROL);
    usb_set_ep_tx_stat(USB_EP0, USB_EP_STAT_TX_STALL);
    usb_set_ep_rx_addr(USB_EP0, USB_X360_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_X360_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* TODO figure out differences in style between RX/TX EP setup */
#if 0
    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_X360_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_X360_RX_ENDP, USB_X360_RX_ADDR);
    usb_set_ep_rx_count(USB_X360_RX_ENDP, USB_X360_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_X360_RX_ENDP, USB_EP_STAT_RX_VALID);
#endif
    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_X360_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_X360_TX_ENDP, USB_X360_TX_ADDR);
    usb_set_ep_tx_stat(USB_X360_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_X360_TX_ENDP, USB_EP_STAT_RX_DISABLED);

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

    /* Reset the RX/TX state */
    n_unread_bytes = 0;
    n_unsent_bytes = 0;
    rx_offset = 0;
    transmitting = 0;
}

static RESULT usbDataSetup(uint8 request) {
    uint8* (*CopyRoutine)(uint16) = 0;
	
	if ((request == GET_DESCRIPTOR)
		&& (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
		&& (pInformation->USBwIndex0 == 0)){
#if 0			
		if (pInformation->USBwValue1 == REPORT_DESCRIPTOR){
			CopyRoutine = HID_GetReportDescriptor;
		} else 
#endif            
        if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE){
			CopyRoutine = HID_GetHIDDescriptor;
		}
		
	} /* End of GET_DESCRIPTOR */
	  /*** GET_PROTOCOL ***/
	else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))//){
			 && request == GET_PROTOCOL){
		CopyRoutine = HID_GetProtocolValue;
	}
	
	if (CopyRoutine == NULL){
		return USB_UNSUPPORT;
	}

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

static RESULT usbNoDataSetup(uint8 request) {
	if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
		&& (request == SET_PROTOCOL)){
		uint8 wValue0 = pInformation->USBwValue0;
		ProtocolValue = wValue0;
		return USB_SUCCESS;
		//return HID_SetProtocol();
	}else{
		return USB_UNSUPPORT;
	}
}

static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting) {
    if (alt_setting > 0) {
        return USB_UNSUPPORT;
    } else if (interface > 1) {
        return USB_UNSUPPORT;
    }

    return USB_SUCCESS;
}

static uint8* usbGetDeviceDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &usbHIDDevice_Descriptor);
}

static uint8* usbGetConfigDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &usbHIDConfig_Descriptor);
}

static uint8* usbGetStringDescriptor(uint16 length) {
    uint8 wValue0 = pInformation->USBwValue0;

    if (wValue0 >= N_STRING_DESCRIPTORS) {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &usbHIDString_Descriptor[wValue0]);
}

/*
static RESULT HID_SetProtocol(void){
	uint8 wValue0 = pInformation->USBwValue0;
	ProtocolValue = wValue0;
	return USB_SUCCESS;
}
*/
static uint8* HID_GetProtocolValue(uint16 Length){
	if (Length == 0){
		pInformation->Ctrl_Info.Usb_wLength = 1;
		return NULL;
	} else {
		return (uint8 *)(&ProtocolValue);
	}
}

#if 0
static uint8* HID_GetReportDescriptor(uint16 Length){
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}
#endif

static uint8* HID_GetHIDDescriptor(uint16 Length)
{
  return Standard_GetDescriptorData(Length, &usbHIDConfig_Descriptor);
}

static void usbSetConfiguration(void) {
    if (pInformation->Current_Configuration != 0) {
        USBLIB->state = USB_CONFIGURED;
    }
}

static void usbSetDeviceAddress(void) {
    USBLIB->state = USB_ADDRESSED;
}
