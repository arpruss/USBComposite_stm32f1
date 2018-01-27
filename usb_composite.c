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
 * @file libmaple/usb/stm32f1/usb_hid.c
 * @brief USB HID (human interface device) support
 *
 * FIXME: this works on the STM32F1 USB peripherals, and probably no
 * place else. Nonportable bits really need to be factored out, and
 * the result made cleaner.
 */

#include "usb_composite.h"
#include "usb_generic.h"
#include <string.h>
#include <libmaple/usb.h>
#include <libmaple/nvic.h>
#include <libmaple/delay.h>

/* Private headers */
#include "usb_lib_globals.h"
#include "usb_reg_map.h"

uint16 GetEPTxAddr(uint8 /*bEpNum*/);

/* usb_lib headers */
#include "usb_type.h"
#include "usb_core.h"
#include "usb_def.h"

static uint32 ProtocolValue;
// Are we currently sending an IN packet?
static volatile int8 transmitting;

static void hidDataTxCb(void);
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
static uint8* HID_GetReportDescriptor(uint16 Length);
static uint8* HID_GetProtocolValue(uint16 Length);
static void vcomDataTxCb(void);
static void vcomDataRxCb(void);

static volatile HIDBuffer_t hidBuffers[MAX_HID_BUFFERS] = {{ 0 }};
static uint8 haveSerial = 0;
static volatile HIDBuffer_t* currentHIDBuffer = NULL;

//#define DUMMY_BUFFER_SIZE 0x40 // at least as big as a buffer size

#define NUM_SERIAL_ENDPOINTS       3
#define CCI_INTERFACE_NUMBER 	0x01
#define DCI_INTERFACE_NUMBER 	0x02

#define HID_INTERFACE_NUMBER 	0x00
#define NUM_HID_ENDPOINTS          1


/*
 * Descriptors
 */
 

const uint8_t hid_report_descriptor[] = {
};

#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0004 // was 0x0024
static usb_descriptor_device usbCompositeDescriptor_Device =
    USB_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);
	

typedef struct {
    usb_descriptor_config_header Config_Header;

    //HID
    usb_descriptor_interface     	HID_Interface;
	HIDDescriptor			 	 	HID_Descriptor;
    usb_descriptor_endpoint      	HIDDataInEndpoint;
    //CDCACM
	IADescriptor 					IAD;
    usb_descriptor_interface     	CCI_Interface;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_IntHeader;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_CallManagement;
    CDC_FUNCTIONAL_DESCRIPTOR(1) 	CDC_Functional_ACM;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_Union;
    usb_descriptor_endpoint      	ManagementEndpoint;
    usb_descriptor_interface     	DCI_Interface;
    usb_descriptor_endpoint      	DataOutEndpoint;
    usb_descriptor_endpoint      	DataInEndpoint;
} __packed usb_descriptor_config;


#define MAX_POWER (100 >> 1)
static usb_descriptor_config usbCompositeDescriptor_Config = {
    .Config_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_descriptor_config),
        .bNumInterfaces       = 3, // 1 if no serial
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
    },
    //HID
    
	.HID_Interface = {
		.bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = HID_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = NUM_HID_ENDPOINTS,
        .bInterfaceClass    = USB_INTERFACE_CLASS_HID,
        .bInterfaceSubClass = USB_INTERFACE_SUBCLASS_HID,
        .bInterfaceProtocol = 0x00, /* Common AT Commands */
        .iInterface         = 0x00,
	},
	.HID_Descriptor = {
		.len				= 9,//sizeof(HIDDescDescriptor),
		.dtype				= HID_DESCRIPTOR_TYPE,
		.versionL			= 0x10,
		.versionH			= 0x01,
		.country			= 0x00,
		.numDesc			= 0x01,
		.desctype			= REPORT_DESCRIPTOR,//0x22,
		.descLenL			= sizeof(hid_report_descriptor),
		.descLenH			= 0x00,
	},
	.HIDDataInEndpoint = {
		.bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_HID_TX_ENDP),//0x81,//USB_HID_TX_ADDR,
        .bmAttributes     = USB_ENDPOINT_TYPE_INTERRUPT,
        .wMaxPacketSize   = USB_HID_TX_EPSIZE,//0x40,//big enough for a keyboard 9 byte packet and for a mouse 5 byte packet
        .bInterval        = 0x0A,
	},
    //CDCACM
	.IAD = {
		.bLength			= 0x08,
		.bDescriptorType	= 0x0B,
		.bFirstInterface	= CCI_INTERFACE_NUMBER,
		.bInterfaceCount	= 0x02,
		.bFunctionClass		= 0x02,
		.bFunctionSubClass	= 0x02,
		.bFunctionProtocol	= 0x01,
		.iFunction			= 0x02,
	},

    .CCI_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = CCI_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x01,
        .bInterfaceClass    = USB_INTERFACE_CLASS_CDC,
        .bInterfaceSubClass = USB_INTERFACE_SUBCLASS_CDC_ACM,
        .bInterfaceProtocol = 0x01, /* Common AT Commands */
        .iInterface         = 0x00,
    },

    .CDC_Functional_IntHeader = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x00,
        .Data            = {0x01, 0x10},
    },

    .CDC_Functional_CallManagement = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x01,
        .Data            = {0x03, DCI_INTERFACE_NUMBER},
    },

    .CDC_Functional_ACM = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(1),
        .bDescriptorType = 0x24,
        .SubType         = 0x02,
        .Data            = {0x06},
    },

    .CDC_Functional_Union = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x06,
        .Data            = {CCI_INTERFACE_NUMBER, DCI_INTERFACE_NUMBER},
    },

    .ManagementEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN |
                             USBHID_CDCACM_MANAGEMENT_ENDP),
        .bmAttributes     = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize   = USBHID_CDCACM_MANAGEMENT_EPSIZE,
        .bInterval        = 0xFF,
    },

    .DCI_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = DCI_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = USB_INTERFACE_CLASS_DIC,
        .bInterfaceSubClass = 0x00, /* None */
        .bInterfaceProtocol = 0x00, /* None */
        .iInterface         = 0x00,
    },

    .DataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT |
                             USBHID_CDCACM_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USBHID_CDCACM_RX_EPSIZE,
        .bInterval        = 0x00,
    },

    .DataInEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USBHID_CDCACM_TX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USBHID_CDCACM_TX_EPSIZE,
        .bInterval        = 0x00,
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
/* FIXME move to Wirish */
static const usb_descriptor_string usbHIDDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

#define default_iManufacturer_length 8
static const usb_descriptor_string usbHIDDescriptor_iManufacturer = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(default_iManufacturer_length),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {'L', 0, 'e', 0, 'a', 0, 'f', 0,
                'L', 0, 'a', 0, 'b', 0, 's', 0},
};

#define default_iProduct_length 5
static const usb_descriptor_string usbHIDDescriptor_iProduct = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(default_iProduct_length),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {'M', 0, 'a', 0, 'p', 0, 'l', 0, 'e', 0},
};

static ONE_DESCRIPTOR Device_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Device,
    sizeof(usb_descriptor_device)
};

ONE_DESCRIPTOR Config_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Config,
    sizeof(usb_descriptor_config)
};

static ONE_DESCRIPTOR HID_Report_Descriptor = {
    (uint8*)&hid_report_descriptor,
    sizeof(hid_report_descriptor)
};

static uint8 numStringDescriptors = 3;

#define MAX_STRING_DESCRIPTORS 4
static ONE_DESCRIPTOR String_Descriptor[MAX_STRING_DESCRIPTORS] = {
    {(uint8*)&usbHIDDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbHIDDescriptor_iManufacturer,         USB_DESCRIPTOR_STRING_LEN(default_iManufacturer_length)},
    {(uint8*)&usbHIDDescriptor_iProduct,              USB_DESCRIPTOR_STRING_LEN(default_iProduct_length)},
    {NULL,                                            0},
#if 0    
    {(uint8*)&usbVcomDescriptor_iInterface,              USB_DESCRIPTOR_STRING_LEN(4)},
    {(uint8*)&usbHIDDescriptor_iInterface,              USB_DESCRIPTOR_STRING_LEN(3)}
#endif    
};


/*
 * Etc.
 */

/* I/O state */

#define HID_TX_BUFFER_SIZE	256 // must be power of 2
#define HID_TX_BUFFER_SIZE_MASK (HID_TX_BUFFER_SIZE-1)
// Tx data
static volatile uint8 hidBufferTx[HID_TX_BUFFER_SIZE];
// Write index to hidBufferTx
static volatile uint32 hid_tx_head;
// Read index from hidBufferTx
static volatile uint32 hid_tx_tail;

#define CDC_SERIAL_RX_BUFFER_SIZE	256 // must be power of 2
#define CDC_SERIAL_RX_BUFFER_SIZE_MASK (CDC_SERIAL_RX_BUFFER_SIZE-1)

/* Received data */
static volatile uint8 vcomBufferRx[CDC_SERIAL_RX_BUFFER_SIZE];
/* Write index to vcomBufferRx */
static volatile uint32 vcom_rx_head;
/* Read index from vcomBufferRx */
static volatile uint32 vcom_rx_tail;

#define CDC_SERIAL_TX_BUFFER_SIZE	256 // must be power of 2
#define CDC_SERIAL_TX_BUFFER_SIZE_MASK (CDC_SERIAL_TX_BUFFER_SIZE-1)
// Tx data
static volatile uint8 vcomBufferTx[CDC_SERIAL_TX_BUFFER_SIZE];
// Write index to vcomBufferTx
static volatile uint32 vcom_tx_head;
// Read index from vcomBufferTx
static volatile uint32 vcom_tx_tail;



/* Other state (line coding, DTR/RTS) */

static volatile composite_cdcacm_line_coding line_coding = {
    /* This default is 115200 baud, 8N1. */
    .dwDTERate   = 115200,
    .bCharFormat = USBHID_CDCACM_STOP_BITS_1,
    .bParityType = USBHID_CDCACM_PARITY_NONE,
    .bDataBits   = 8,
};

/* DTR in bit 0, RTS in bit 1. */
static volatile uint8 line_dtr_rts = 0;

/*
 * Endpoint callbacks
 */

static void (*ep_int_in[7])(void) =
    {hidDataTxCb,
     vcomDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) = {
     NOP_Process,
     NOP_Process, /*hidDataRxCb*/
     NOP_Process, 
     vcomDataRxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process};

/*
 * Globals required by usb_lib/
 */
 
#define NUM_ENDPTS                (1+NUM_SERIAL_ENDPOINTS+NUM_HID_ENDPOINTS)
    
static DEVICE my_Device_Table = {
    .Total_Endpoint      = NUM_ENDPTS,
    .Total_Configuration = 1
};

#define MAX_PACKET_SIZE            0x40  /* 64B, maximum for USB FS Devices */
static DEVICE_PROP my_Device_Property = {
    .Init                        = usbInit,
    .Reset                       = usbReset,
    .Process_Status_IN           = NOP_Process, //hidStatusIn,
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

uint8 usb_is_transmitting(void) {
    return transmitting;
}

static void (*rx_hook)(unsigned, void*) = 0;
static void (*iface_setup_hook)(unsigned, void*) = 0;

void composite_cdcacm_set_hooks(unsigned hook_flags, void (*hook)(unsigned, void*)) {
    if (hook_flags & USBHID_CDCACM_HOOK_RX) {
        rx_hook = hook;
    }
    if (hook_flags & USBHID_CDCACM_HOOK_IFACE_SETUP) {
        iface_setup_hook = hook;
    }
}

void composite_cdcacm_putc(char ch) {
    while (!composite_cdcacm_tx((uint8*)&ch, 1))
        ;
}

/* This function is non-blocking.
 *
 * It copies data from a user buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 composite_cdcacm_tx(const uint8* buf, uint32 len)
{
	if (len==0) return 0; // no data to send

	uint32 head = vcom_tx_head; // load volatile variable
	uint32 tx_unsent = (head - vcom_tx_tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;

    // We can only put bytes in the buffer if there is place
    if (len > (CDC_SERIAL_TX_BUFFER_SIZE-tx_unsent-1) ) {
        len = (CDC_SERIAL_TX_BUFFER_SIZE-tx_unsent-1);
    }
	if (len==0) return 0; // buffer full

	uint16 i;
	// copy data from user buffer to USB Tx buffer
	for (i=0; i<len; i++) {
		vcomBufferTx[head] = buf[i];
		head = (head+1) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
	}
	vcom_tx_head = head; // store volatile variable
	
	while(transmitting >= 0);
	
	if (transmitting<0) {
		vcomDataTxCb(); // initiate data transmission
	}

    return len;
}



uint32 composite_cdcacm_data_available(void) {
    return (vcom_rx_head - vcom_rx_tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
}

uint16 composite_cdcacm_get_pending(void) {
    return (vcom_tx_head - vcom_tx_tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
}

/* Non-blocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 composite_cdcacm_rx(uint8* buf, uint32 len)
{
    /* Copy bytes to buffer. */
    uint32 n_copied = composite_cdcacm_peek(buf, len);

    /* Mark bytes as read. */
	uint16 tail = vcom_rx_tail; // load volatile variable
	tail = (tail + n_copied) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	vcom_rx_tail = tail; // store volatile variable

	uint32 rx_unread = (vcom_rx_head - tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    // If buffer was emptied to a pre-set value, re-enable the RX endpoint
    if ( rx_unread <= 64 ) { // experimental value, gives the best performance
        usb_set_ep_rx_stat(USBHID_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
	}
    return n_copied;
}

/* Non-blocking byte lookahead.
 *
 * Looks at unread bytes without marking them as read. */
uint32 composite_cdcacm_peek(uint8* buf, uint32 len)
{
    unsigned i;
    uint32 tail = vcom_rx_tail;
	uint32 rx_unread = (vcom_rx_head-tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;

    if (len > rx_unread) {
        len = rx_unread;
    }

    for (i = 0; i < len; i++) {
        buf[i] = vcomBufferRx[tail];
        tail = (tail + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

uint32 composite_cdcacm_peek_ex(uint8* buf, uint32 offset, uint32 len)
{
    unsigned i;
    uint32 tail = (vcom_rx_tail + offset) & CDC_SERIAL_RX_BUFFER_SIZE_MASK ;
	uint32 rx_unread = (vcom_rx_head-tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;

    if (len + offset > rx_unread) {
        len = rx_unread - offset;
    }

    for (i = 0; i < len; i++) {
        buf[i] = vcomBufferRx[tail];
        tail = (tail + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

/* Roger Clark. Added. for Arduino 1.0 API support of Serial.peek() */
int composite_cdcacm_peek_char() 
{
    if (composite_cdcacm_data_available() == 0) 
	{
		return -1;
    }

    return vcomBufferRx[vcom_rx_tail];
}

uint8 composite_cdcacm_get_dtr() {
    return ((line_dtr_rts & USBHID_CDCACM_CONTROL_LINE_DTR) != 0);
}

uint8 composite_cdcacm_get_rts() {
    return ((line_dtr_rts & USBHID_CDCACM_CONTROL_LINE_RTS) != 0);
}

void composite_cdcacm_get_line_coding(composite_cdcacm_line_coding *ret) {
    ret->dwDTERate = line_coding.dwDTERate;
    ret->bCharFormat = line_coding.bCharFormat;
    ret->bParityType = line_coding.bParityType;
    ret->bDataBits = line_coding.bDataBits;
}

int composite_cdcacm_get_baud(void) {
    return line_coding.dwDTERate;
}

int composite_cdcacm_get_stop_bits(void) {
    return line_coding.bCharFormat;
}

int composite_cdcacm_get_parity(void) {
    return line_coding.bParityType;
}

int composite_cdcacm_get_n_data_bits(void) {
    return line_coding.bDataBits;
}

/*
 * Callbacks
 */
static void vcomDataTxCb(void)
{
	uint32 tail = vcom_tx_tail; // load volatile variable
	uint32 tx_unsent = (vcom_tx_head - tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
	if (tx_unsent==0) {
		if ( (--transmitting)==0) goto flush_vcom; // no more data to send
		return; // it was already flushed, keep Tx endpoint disabled
	}
	transmitting = 1;
    // We can only send up to USBHID_CDCACM_TX_EPSIZE bytes in the endpoint.
    if (tx_unsent > USBHID_CDCACM_TX_EPSIZE) {
        tx_unsent = USBHID_CDCACM_TX_EPSIZE;
    }
	// copy the bytes from USB Tx buffer to PMA buffer
	uint32 *dst = usb_pma_ptr(USBHID_CDCACM_TX_ADDR);
    uint16 tmp = 0;
	uint16 val;
	unsigned i;
	for (i = 0; i < tx_unsent; i++) {
		val = vcomBufferTx[tail];
		tail = (tail + 1) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
		if (i&1) {
			*dst++ = tmp | (val<<8);
		} else {
			tmp = val;
		}
	}
    if ( tx_unsent&1 ) {
        *dst = tmp;
    }
	vcom_tx_tail = tail; // store volatile variable
flush_vcom:
	// enable Tx endpoint
    usb_set_ep_tx_count(USBHID_CDCACM_TX_ENDP, tx_unsent);
    usb_set_ep_tx_stat(USBHID_CDCACM_TX_ENDP, USB_EP_STAT_TX_VALID);
}


static void vcomDataRxCb(void)
{
	uint32 head = vcom_rx_head; // load volatile variable

	uint32 ep_rx_size = usb_get_ep_rx_count(USBHID_CDCACM_RX_ENDP);
	// This copy won't overwrite unread bytes as long as there is 
	// enough room in the USB Rx buffer for next packet
	uint32 *src = usb_pma_ptr(USBHID_CDCACM_RX_ADDR);
    uint16 tmp = 0;
	uint8 val;
	uint32 i;
	for (i = 0; i < ep_rx_size; i++) {
		if (i&1) {
			val = tmp>>8;
		} else {
			tmp = *src++;
			val = tmp&0xFF;
		}
		vcomBufferRx[head] = val;
		head = (head + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	}
	vcom_rx_head = head; // store volatile variable

	uint32 rx_unread = (head - vcom_rx_tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	// only enable further Rx if there is enough room to receive one more packet
	if ( rx_unread < (CDC_SERIAL_RX_BUFFER_SIZE-USBHID_CDCACM_RX_EPSIZE) ) {
		usb_set_ep_rx_stat(USBHID_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
	}

    if (rx_hook) {
        rx_hook(USBHID_CDCACM_HOOK_RX, 0);
    }
}

static uint8* vcomGetSetLineCoding(uint16 length) {
    if (length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(struct composite_cdcacm_line_coding);
    }
    return (uint8*)&line_coding;
}

/*
 * HID interface
 */

void usb_composite_enable(const uint8* report_descriptor, uint16 report_descriptor_length, const uint8 serial,
    uint16 idVendor, uint16 idProduct, const uint8* iManufacturer, const uint8* iProduct, const uint8* iSerialNumber
    ) {
    HID_Report_Descriptor.Descriptor = (uint8*)report_descriptor;
    HID_Report_Descriptor.Descriptor_Size = report_descriptor_length;        
    usbCompositeDescriptor_Config.HID_Descriptor.descLenL = (uint8_t)report_descriptor_length;
    usbCompositeDescriptor_Config.HID_Descriptor.descLenH = (uint8_t)(report_descriptor_length>>8);
        
    if (idVendor != 0)
        usbCompositeDescriptor_Device.idVendor = idVendor;
    else
        usbCompositeDescriptor_Device.idVendor = LEAFLABS_ID_VENDOR;
     
    if (idProduct != 0)
        usbCompositeDescriptor_Device.idProduct = idProduct;
    else
        usbCompositeDescriptor_Device.idProduct = MAPLE_ID_PRODUCT;
    
    if (iManufacturer == NULL) {
        iManufacturer = (uint8*)&usbHIDDescriptor_iManufacturer;
    }
           
    String_Descriptor[1].Descriptor = (uint8*)iManufacturer;
    String_Descriptor[1].Descriptor_Size = iManufacturer[0];
     
    if (iProduct == NULL) {
        iProduct = (uint8*)&usbHIDDescriptor_iProduct;
    }
           
    String_Descriptor[2].Descriptor = (uint8*)iProduct;
    String_Descriptor[2].Descriptor_Size = iProduct[0];
    
    if (iSerialNumber == NULL) {
        numStringDescriptors = 3;
        usbCompositeDescriptor_Device.iSerialNumber = 0;
    }
    else {
        String_Descriptor[3].Descriptor = (uint8*)iSerialNumber;
        String_Descriptor[3].Descriptor_Size = iSerialNumber[0];
        numStringDescriptors = 4;
        usbCompositeDescriptor_Device.iSerialNumber = 3;
    }
    
    haveSerial = serial;

    if (serial) {
        ep_int_in[1] = vcomDataTxCb;
        ep_int_out[3] = vcomDataRxCb;
        usbCompositeDescriptor_Config.Config_Header.bNumInterfaces = 3;
        Config_Descriptor.Descriptor_Size = sizeof(usb_descriptor_config);
        usbCompositeDescriptor_Config.Config_Header.wTotalLength = sizeof(usb_descriptor_config);
        my_Device_Table.Total_Endpoint = 1+NUM_SERIAL_ENDPOINTS+NUM_HID_ENDPOINTS;
    }
    else {
        ep_int_in[1] = NOP_Process;
        ep_int_out[3] = NOP_Process;
        usbCompositeDescriptor_Config.Config_Header.bNumInterfaces = 1;
        Config_Descriptor.Descriptor_Size = (uint8*)&(usbCompositeDescriptor_Config.IAD)-(uint8*)&usbCompositeDescriptor_Config;
        usbCompositeDescriptor_Config.Config_Header.wTotalLength = Config_Descriptor.Descriptor_Size;
        my_Device_Table.Total_Endpoint = 1+NUM_HID_ENDPOINTS;
    }

    usb_generic_enable(&my_Device_Table, &my_Device_Property, &my_User_Standard_Requests, ep_int_in, ep_int_out);
}

void usb_composite_disable(void) {
    usb_generic_disable();

    usb_hid_clear_buffers(HID_REPORT_TYPE_FEATURE);
    usb_hid_clear_buffers(HID_REPORT_TYPE_OUTPUT);
}

void usb_hid_putc(char ch) {
    while (!usb_hid_tx((uint8*)&ch, 1))
        ;
}

    /*
static void hidStatusIn() {
    if (pInformation->ControlState == WAIT_STATUS_IN) {
        if (currentInFeature >= 0) {
            if (featureBuffers[currentInFeature].bufferSize == featureBuffers[currentInFeature].currentDataSize) 
                featureBuffers[currentInFeature].state = HID_BUFFER_UNREAD;
            currentInFeature = -1;
        }
        if (currentOutput >= 0) {
            if (outputBuffers[currentOutput].bufferSize == outputBuffers[currentOutput].currentDataSize) 
                outputBuffers[currentOutput].state = HID_BUFFER_UNREAD;
            currentOutput = -1;
        }
    }
}
    */

static volatile HIDBuffer_t* usb_hid_find_buffer(uint8_t type, uint8_t reportID) {
    uint8_t typeTest = type == HID_REPORT_TYPE_OUTPUT ? HID_BUFFER_MODE_OUTPUT : 0;
    for (int i=0; i<MAX_HID_BUFFERS; i++) {
        if ( hidBuffers[i].buffer != NULL &&
             ( hidBuffers[i].mode & HID_BUFFER_MODE_OUTPUT ) == typeTest && 
             hidBuffers[i].reportID == reportID ) {
            return hidBuffers+i;
        }
    }
    return NULL;
}

void usb_hid_set_feature(uint8_t reportID, uint8_t* data) {
    volatile HIDBuffer_t* buffer = usb_hid_find_buffer(HID_REPORT_TYPE_FEATURE, reportID);
    if (buffer != NULL) {
        usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_NAK);
        unsigned delta = reportID != 0;
        memcpy((uint8_t*)buffer->buffer+delta, data, buffer->bufferSize-delta);
        if (reportID)
            buffer->buffer[0] = reportID;
        buffer->currentDataSize = buffer->bufferSize;
        buffer->state = HID_BUFFER_READ;
        usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);
        return;
    }
}

static uint8_t have_unread_data_in_hid_buffer() {
    for (int i=0;i<MAX_HID_BUFFERS; i++) {
        if (hidBuffers[i].buffer != NULL && hidBuffers[i].state == HID_BUFFER_UNREAD)
            return 1;
    }
    return 0;
}

uint16_t usb_hid_get_data(uint8_t type, uint8_t reportID, uint8_t* out, uint8_t poll) {
    volatile HIDBuffer_t* buffer;
    unsigned ret = 0;
    
    buffer = usb_hid_find_buffer(type, reportID);
    
    if (buffer == NULL)
        return 0;

    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);

    if (buffer->reportID == reportID && buffer->state != HID_BUFFER_EMPTY && !(poll && buffer->state == HID_BUFFER_READ)) {
        if (buffer->bufferSize != buffer->currentDataSize) {
           buffer->state = HID_BUFFER_EMPTY;
           ret = 0;
        }
        else {
            unsigned delta = reportID != 0;
            if (out != NULL)
                memcpy(out, (uint8*)buffer->buffer+delta, buffer->bufferSize-delta);
            
            if (poll) {
                buffer->state = HID_BUFFER_READ;
            }

            ret = buffer->bufferSize-delta;
        }
    }
    
    if (! have_unread_data_in_hid_buffer() ) {
        usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);
    }

    nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
            
    return ret;
}

void usb_hid_clear_buffers(uint8_t type) {
    uint8_t typeTest = type == HID_REPORT_TYPE_OUTPUT ? HID_BUFFER_MODE_OUTPUT : 0;
    for (int i=0; i<MAX_HID_BUFFERS; i++) {
        if (( hidBuffers[i].mode & HID_BUFFER_MODE_OUTPUT ) == typeTest) {
            hidBuffers[i].buffer = NULL;
        }
    }
}

uint8_t usb_hid_add_buffer(uint8_t type, volatile HIDBuffer_t* buf) {
    if (type == HID_BUFFER_MODE_OUTPUT) 
        buf->mode |= HID_BUFFER_MODE_OUTPUT;
    else
        buf->mode &= ~HID_BUFFER_MODE_OUTPUT;
    memset((void*)buf->buffer, 0, buf->bufferSize);
    buf->buffer[0] = buf->reportID;

    volatile HIDBuffer_t* buffer = usb_hid_find_buffer(type, buf->reportID);

    if (buffer != NULL) {
        *buffer = *buf;
        return 1;
    }
    else {
        for (int i=0; i<MAX_HID_BUFFERS; i++) {
            if (hidBuffers[i].buffer == NULL) {
                hidBuffers[i] = *buf;
                return 1;
            }
        }
        return 0;
    }
}

void usb_hid_set_buffers(uint8_t type, volatile HIDBuffer_t* bufs, int n) {
    uint8_t typeMask = type == HID_REPORT_TYPE_OUTPUT ? HID_BUFFER_MODE_OUTPUT : 0;
    usb_hid_clear_buffers(type);
    for (int i=0; i<n; i++) {
        bufs[i].mode &= ~HID_REPORT_TYPE_OUTPUT;
        bufs[i].mode |= typeMask;
        usb_hid_add_buffer(type, bufs+i);
    }
    currentHIDBuffer = NULL;
}

/* This function is non-blocking.
 *
 * It copies data from a user buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 usb_hid_tx(const uint8* buf, uint32 len)
{
	if (len==0) return 0; // no data to send

	uint32 head = hid_tx_head; // load volatile variable
	uint32 tx_unsent = (head - hid_tx_tail) & HID_TX_BUFFER_SIZE_MASK;

    // We can only put bytes in the buffer if there is place
    if (len > (HID_TX_BUFFER_SIZE-tx_unsent-1) ) {
        len = (HID_TX_BUFFER_SIZE-tx_unsent-1);
    }
	if (len==0) return 0; // buffer full

	uint16 i;
	// copy data from user buffer to USB Tx buffer
	for (i=0; i<len; i++) {
		hidBufferTx[head] = buf[i];
		head = (head+1) & HID_TX_BUFFER_SIZE_MASK;
	}
	hid_tx_head = head; // store volatile variable

	while(transmitting >= 0);
	
	if (transmitting<0) {
		hidDataTxCb(); // initiate data transmission
	}

    return len;
}



uint16 usb_hid_get_pending(void) {
    return (hid_tx_head - hid_tx_tail) & HID_TX_BUFFER_SIZE_MASK;
}

static void hidDataTxCb(void)
{
	uint32 tail = hid_tx_tail; // load volatile variable
	uint32 tx_unsent = (hid_tx_head - tail) & HID_TX_BUFFER_SIZE_MASK;
	if (tx_unsent==0) {
		if ( (--transmitting)==0) goto flush_hid; // no more data to send
		return; // it was already flushed, keep Tx endpoint disabled
	}
	transmitting = 1;
    // We can only send up to USBHID_CDCACM_TX_EPSIZE bytes in the endpoint.
    if (tx_unsent > USB_HID_TX_EPSIZE) {
        tx_unsent = USB_HID_TX_EPSIZE;
    }
	// copy the bytes from USB Tx buffer to PMA buffer
	uint32 *dst = usb_pma_ptr(USB_HID_TX_ADDR);
    uint16 tmp = 0;
	uint16 val;
	unsigned i;
	for (i = 0; i < tx_unsent; i++) {
		val = hidBufferTx[tail];
		tail = (tail + 1) & HID_TX_BUFFER_SIZE_MASK;
		if (i&1) {
			*dst++ = tmp | (val<<8);
		} else {
			tmp = val;
		}
	}
    if ( tx_unsent&1 ) {
        *dst = tmp;
    }
	hid_tx_tail = tail; // store volatile variable
flush_hid:
	// enable Tx endpoint
    usb_set_ep_tx_count(USB_HID_TX_ENDP, tx_unsent);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_VALID);
}



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
    
    currentHIDBuffer = NULL;
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
    usb_set_ep_rx_addr(USB_EP0, USBHID_CDCACM_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USBHID_CDCACM_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* set up hid endpoint IN (TX)  */
    usb_set_ep_type(USB_HID_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_HID_TX_ENDP, USB_HID_TX_ADDR);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_HID_TX_ENDP, USB_EP_STAT_RX_DISABLED);
    
    if (haveSerial) {
        /* setup management endpoint 1  */
        usb_set_ep_type(USBHID_CDCACM_MANAGEMENT_ENDP, USB_EP_EP_TYPE_INTERRUPT);
        usb_set_ep_tx_addr(USBHID_CDCACM_MANAGEMENT_ENDP,
                           USBHID_CDCACM_MANAGEMENT_ADDR);
        usb_set_ep_tx_stat(USBHID_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_TX_NAK);
        usb_set_ep_rx_stat(USBHID_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_RX_DISABLED);

        /* TODO figure out differences in style between RX/TX EP setup */

        /* set up data endpoint OUT (RX) */
        usb_set_ep_type(USBHID_CDCACM_RX_ENDP, USB_EP_EP_TYPE_BULK);
        usb_set_ep_rx_addr(USBHID_CDCACM_RX_ENDP, USBHID_CDCACM_RX_ADDR);
        usb_set_ep_rx_count(USBHID_CDCACM_RX_ENDP, USBHID_CDCACM_RX_EPSIZE);
        usb_set_ep_rx_stat(USBHID_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);

        /* set up data endpoint IN (TX)  */
        usb_set_ep_type(USBHID_CDCACM_TX_ENDP, USB_EP_EP_TYPE_BULK);
        usb_set_ep_tx_addr(USBHID_CDCACM_TX_ENDP, USBHID_CDCACM_TX_ADDR);
        usb_set_ep_tx_stat(USBHID_CDCACM_TX_ENDP, USB_EP_STAT_TX_NAK);
        usb_set_ep_rx_stat(USBHID_CDCACM_TX_ENDP, USB_EP_STAT_RX_DISABLED);
        
        //VCOM
        vcom_rx_head = 0;
        vcom_rx_tail = 0;
        vcom_tx_head = 0;
        vcom_tx_tail = 0;
    }

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

    /* Reset the RX/TX state */

	transmitting = -1;
   
    /* Reset the RX/TX state */
	hid_tx_head = 0;
	hid_tx_tail = 0;
}

static uint8* HID_Set(uint16 length) {
    if (currentHIDBuffer == NULL)
        return NULL;
    
    if (length ==0) {
        if ( (0 == (currentHIDBuffer->mode & HID_BUFFER_MODE_NO_WAIT)) && 
                currentHIDBuffer->state == HID_BUFFER_UNREAD && 
                pInformation->Ctrl_Info.Usb_wOffset < pInformation->USBwLengths.w) {
            pInformation->Ctrl_Info.Usb_wLength = 0xFFFF;
            return NULL;
        }

        uint16 len = pInformation->USBwLengths.w;
        if (len > currentHIDBuffer->bufferSize)
            len = currentHIDBuffer->bufferSize;
        
        currentHIDBuffer->currentDataSize = len;
        
        currentHIDBuffer->state = HID_BUFFER_EMPTY;
        
        if (pInformation->Ctrl_Info.Usb_wOffset < len) { 
            pInformation->Ctrl_Info.Usb_wLength = len - pInformation->Ctrl_Info.Usb_wOffset;
        }
        else {
            pInformation->Ctrl_Info.Usb_wLength = 0; 
        }

        return NULL;
    }
    
    if (pInformation->USBwLengths.w <= pInformation->Ctrl_Info.Usb_wOffset + pInformation->Ctrl_Info.PacketSize) {
        currentHIDBuffer->state = HID_BUFFER_UNREAD;
    }
    
    return (uint8*)currentHIDBuffer->buffer + pInformation->Ctrl_Info.Usb_wOffset;
}

static uint8* HID_GetFeature(uint16 length) {
    if (currentHIDBuffer == NULL)
        return NULL;
    
    unsigned wOffset = pInformation->Ctrl_Info.Usb_wOffset;
    
    if (length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = currentHIDBuffer->bufferSize - wOffset;
        return NULL;
    }

    return (uint8*)currentHIDBuffer->buffer + wOffset;
}

static RESULT usbDataSetup(uint8 request) {
    uint8* (*CopyRoutine)(uint16) = 0;
    
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {        
        switch (request) {
        case USBHID_CDCACM_GET_LINE_CODING:
            if (haveSerial)
                CopyRoutine = vcomGetSetLineCoding;
            break;
        case USBHID_CDCACM_SET_LINE_CODING:
            if (haveSerial)
                CopyRoutine = vcomGetSetLineCoding;
            break;
        case SET_REPORT:
            if (pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){                
                if (pInformation->USBwValue1 == HID_REPORT_TYPE_FEATURE) {
                    volatile HIDBuffer_t* buffer = usb_hid_find_buffer(HID_REPORT_TYPE_FEATURE, pInformation->USBwValue0);
                    
                    if (buffer == NULL) {
                        return USB_UNSUPPORT;
                    }
                    
                    if (0 == (buffer->mode & HID_BUFFER_MODE_NO_WAIT) && buffer->state == HID_BUFFER_UNREAD) {
                        return USB_NOT_READY;
                    } 
                    else 
                    {
                        currentHIDBuffer = buffer;
                        CopyRoutine = HID_Set;        
                    }
                }
                else if (pInformation->USBwValue1 == HID_REPORT_TYPE_OUTPUT) {
                    volatile HIDBuffer_t* buffer = usb_hid_find_buffer(HID_REPORT_TYPE_OUTPUT, pInformation->USBwValue0);
                        
                    if (buffer == NULL) {
                        return USB_UNSUPPORT;
                    }
                    
                    if (0 == (buffer->mode & HID_BUFFER_MODE_NO_WAIT) && buffer->state == HID_BUFFER_UNREAD) {
                        return USB_NOT_READY;
                    } 
                    else 
                    {
                        currentHIDBuffer = buffer;
                        CopyRoutine = HID_Set;        
                    }
                }
            }
            break;
        case GET_REPORT:
            if (pInformation->USBwValue1 == HID_REPORT_TYPE_FEATURE &&
                pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){						
            volatile HIDBuffer_t* buffer = usb_hid_find_buffer(HID_REPORT_TYPE_FEATURE, pInformation->USBwValue0);
            
            if (buffer == NULL || buffer->state == HID_BUFFER_EMPTY) {
                return USB_UNSUPPORT;
            }

            currentHIDBuffer = buffer;
            CopyRoutine = HID_GetFeature;        
            break;
        }
        default:
            break;
        }
        /* Call the user hook. */
        if (iface_setup_hook) {
            uint8 req_copy = request;
            iface_setup_hook(USBHID_CDCACM_HOOK_IFACE_SETUP, &req_copy);
        }
    }
	
	if(Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)){
    	switch (request){
    		case GET_DESCRIPTOR:
    			if(pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){						
					if (pInformation->USBwValue1 == REPORT_DESCRIPTOR){
						CopyRoutine = HID_GetReportDescriptor;
					} else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE){
						CopyRoutine = usbGetConfigDescriptor;
					}					
				}
    			break;
    		case GET_PROTOCOL:
    			CopyRoutine = HID_GetProtocolValue;
    			break;
		}
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
    RESULT ret = USB_UNSUPPORT;
    
	if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
        switch(request) {
            case USBHID_CDCACM_SET_COMM_FEATURE:
	            /* We support set comm. feature, but don't handle it. */
	            if (haveSerial)
                    ret = USB_SUCCESS;
	            break;
	        case USBHID_CDCACM_SET_CONTROL_LINE_STATE:
	            /* Track changes to DTR and RTS. */
	            if (haveSerial) {
                    line_dtr_rts = (pInformation->USBwValues.bw.bb0 &
                                    (USBHID_CDCACM_CONTROL_LINE_DTR |
                                     USBHID_CDCACM_CONTROL_LINE_RTS));
                    ret = USB_SUCCESS;
                }
	            break;
            case SET_PROTOCOL:
                ProtocolValue = pInformation->USBwValue0;
                ret = USB_SUCCESS;
                break;
        }
    }
    /* Call the user hook. */
    if (haveSerial && iface_setup_hook) {
        uint8 req_copy = request;
        iface_setup_hook(USBHID_CDCACM_HOOK_IFACE_SETUP, &req_copy);
    }
    return ret;
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
    return Standard_GetDescriptorData(length, &Device_Descriptor);
}

static uint8* usbGetConfigDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &Config_Descriptor);
}

static uint8* usbGetStringDescriptor(uint16 length) {    
    uint8 wValue0 = pInformation->USBwValue0;
    
    if (wValue0 > numStringDescriptors) {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &String_Descriptor[wValue0]);
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

static uint8* HID_GetReportDescriptor(uint16 Length){
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}

static void usbSetConfiguration(void) {
    if (pInformation->Current_Configuration != 0) {
        USBLIB->state = USB_CONFIGURED;
    }
}

static void usbSetDeviceAddress(void) {
    USBLIB->state = USB_ADDRESSED;
}
