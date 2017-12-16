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

uint32 ProtocolValue;
// Are we currently sending an IN packet?
static volatile int8 transmitting;

static void hidDataTxCb(void);
#ifdef USB_HID_RX_SUPPORT
static void hidDataRxCb(void);
#else
#define hidDataRxCb NOP_Process
#endif
static void hidStatusIn(void);

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
#ifdef COMPOSITE_SERIAL
static void vcomDataTxCb(void);
static void vcomDataRxCb(void);
#endif

volatile HIDBuffer_t* featureBuffers = NULL;
volatile HIDBuffer_t* outputBuffers = NULL;
static int featureBufferCount = 0;
static int outputBufferCount = 0;
volatile HIDBuffer_t* currentHIDBuffer = NULL;

//#define DUMMY_BUFFER_SIZE 0x40 // at least as big as a buffer size

#ifdef COMPOSITE_SERIAL
#define NUM_SERIAL_ENDPOINTS       3
#define NUMINTERFACES 			0x03
#define HID_INTERFACE_NUMBER 	0x02
#define CCI_INTERFACE_NUMBER 	0x00
#define DCI_INTERFACE_NUMBER 	0x01
#else
#define NUM_SERIAL_ENDPOINTS       0
#define NUMINTERFACES           0x01
#define HID_INTERFACE_NUMBER 	0x00
#endif

#ifdef USB_HID_RX_SUPPORT
#define NUM_HID_ENDPOINTS          2
#else
#define NUM_HID_ENDPOINTS          1
#endif


/*
 * Descriptors
 */
 

const uint8_t hid_report_descriptor[] = {
};

/* FIXME move to Wirish */
#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0024
static usb_descriptor_device usbCompositeDescriptor_Device =
    USB_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);
	

typedef struct {
    usb_descriptor_config_header Config_Header;
#ifdef COMPOSITE_SERIAL
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
#endif    
    //HID
    usb_descriptor_interface     	HID_Interface;
	HIDDescriptor			 	 	HID_Descriptor;
    usb_descriptor_endpoint      	HIDDataInEndpoint;
#ifdef USB_HID_RX_SUPPORT    
    usb_descriptor_endpoint      	HIDDataOutEndpoint;
#endif    
} __packed usb_descriptor_config;


#define MAX_POWER (100 >> 1)
static usb_descriptor_config usbCompositeDescriptor_Config = {
    .Config_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_descriptor_config),
        .bNumInterfaces       = NUMINTERFACES,
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
    },
#ifdef COMPOSITE_SERIAL    
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
                             USB_CDCACM_MANAGEMENT_ENDP),
        .bmAttributes     = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize   = USB_CDCACM_MANAGEMENT_EPSIZE,
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
                             USB_CDCACM_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_CDCACM_RX_EPSIZE,
        .bInterval        = 0x00,
    },

    .DataInEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_CDCACM_TX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_CDCACM_TX_EPSIZE,
        .bInterval        = 0x00,
    },
#endif    
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
#ifdef USB_HID_RX_SUPPORT    
    .HIDDataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT | USB_HID_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_HID_RX_EPSIZE,
        .bInterval        = 0x00,
    },
#endif    
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

#if 0
static const usb_descriptor_string usbHIDDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(3),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'H', 0, 'I', 0, 'D', 0},
};

static const usb_descriptor_string usbVcomDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(4),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'V', 0, 'C', 0, 'O', 0, 'M', 0},
};

static const usb_descriptor_string usbCompositeDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(9),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'C', 0, 'O', 0, 'M', 0, 'P', 0, 'O', 0, 'S', 0, 'I', 0, 'T', 0, 'E', 0},
};
#endif

static ONE_DESCRIPTOR Device_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR Config_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Config,
    sizeof(usb_descriptor_config)
};

static ONE_DESCRIPTOR HID_Report_Descriptor = {
    (uint8*)&hid_report_descriptor,
    sizeof(hid_report_descriptor)
};

#define N_STRING_DESCRIPTORS 3
static ONE_DESCRIPTOR String_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbHIDDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbHIDDescriptor_iManufacturer,         USB_DESCRIPTOR_STRING_LEN(default_iManufacturer_length)},
    {(uint8*)&usbHIDDescriptor_iProduct,              USB_DESCRIPTOR_STRING_LEN(default_iProduct_length)},
#if 0    
    {(uint8*)&usbVcomDescriptor_iInterface,              USB_DESCRIPTOR_STRING_LEN(4)},
    {(uint8*)&usbHIDDescriptor_iInterface,              USB_DESCRIPTOR_STRING_LEN(3)}
#endif    
};


/*
 * Etc.
 */

/* I/O state */

#ifdef USB_HID_RX_SUPPORT
#define HID_RX_BUFFER_SIZE	256 // must be power of 2
#define HID_RX_BUFFER_SIZE_MASK (HID_RX_BUFFER_SIZE-1)
/* Received data */
static volatile uint8 hidBufferRx[HID_RX_BUFFER_SIZE];
/* Write index to hidBufferRx */
static volatile uint32 hid_rx_head;
/* Read index from hidBufferRx */
static volatile uint32 hid_rx_tail;
#endif

#define HID_TX_BUFFER_SIZE	256 // must be power of 2
#define HID_TX_BUFFER_SIZE_MASK (HID_TX_BUFFER_SIZE-1)
// Tx data
static volatile uint8 hidBufferTx[HID_TX_BUFFER_SIZE];
// Write index to hidBufferTx
static volatile uint32 hid_tx_head;
// Read index from hidBufferTx
static volatile uint32 hid_tx_tail;

#ifdef COMPOSITE_SERIAL
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
    .bCharFormat = USB_CDCACM_STOP_BITS_1,
    .bParityType = USB_CDCACM_PARITY_NONE,
    .bDataBits   = 8,
};

/* DTR in bit 0, RTS in bit 1. */
static volatile uint8 line_dtr_rts = 0;

/*
 * Endpoint callbacks
 */

static void (*ep_int_in[7])(void) =
     {vcomDataTxCb,
     NOP_Process,
     NOP_Process,
     hidDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
   {NOP_Process,
     NOP_Process,
     vcomDataRxCb,
     NOP_Process,
     hidDataRxCb,
     NOP_Process,
     NOP_Process};
#else
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
     hidDataRxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};
#endif
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
    .Process_Status_IN           = hidStatusIn,
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

DEVICE saved_Device_Table;
DEVICE_PROP saved_Device_Property;
USER_STANDARD_REQUESTS saved_User_Standard_Requests;

uint8 usb_is_transmitting(void) {
    return transmitting;
}

#ifdef COMPOSITE_SERIAL
static void (*rx_hook)(unsigned, void*) = 0;
static void (*iface_setup_hook)(unsigned, void*) = 0;

void composite_cdcacm_set_hooks(unsigned hook_flags, void (*hook)(unsigned, void*)) {
    if (hook_flags & USB_CDCACM_HOOK_RX) {
        rx_hook = hook;
    }
    if (hook_flags & USB_CDCACM_HOOK_IFACE_SETUP) {
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
        usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
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
    return ((line_dtr_rts & USB_CDCACM_CONTROL_LINE_DTR) != 0);
}

uint8 composite_cdcacm_get_rts() {
    return ((line_dtr_rts & USB_CDCACM_CONTROL_LINE_RTS) != 0);
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
    // We can only send up to USB_CDCACM_TX_EPSIZE bytes in the endpoint.
    if (tx_unsent > USB_CDCACM_TX_EPSIZE) {
        tx_unsent = USB_CDCACM_TX_EPSIZE;
    }
	// copy the bytes from USB Tx buffer to PMA buffer
	uint32 *dst = usb_pma_ptr(USB_CDCACM_TX_ADDR);
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
    usb_set_ep_tx_count(USB_CDCACM_TX_ENDP, tx_unsent);
    usb_set_ep_tx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_TX_VALID);
}


static void vcomDataRxCb(void)
{
	uint32 head = vcom_rx_head; // load volatile variable

	uint32 ep_rx_size = usb_get_ep_rx_count(USB_CDCACM_RX_ENDP);
	// This copy won't overwrite unread bytes as long as there is 
	// enough room in the USB Rx buffer for next packet
	uint32 *src = usb_pma_ptr(USB_CDCACM_RX_ADDR);
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
	if ( rx_unread < (CDC_SERIAL_RX_BUFFER_SIZE-USB_CDCACM_RX_EPSIZE) ) {
		usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
	}

    if (rx_hook) {
        rx_hook(USB_CDCACM_HOOK_RX, 0);
    }
}

static uint8* vcomGetSetLineCoding(uint16 length) {
    if (length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(struct composite_cdcacm_line_coding);
    }
    return (uint8*)&line_coding;
}
#endif

/*
 * HID interface
 */

void usb_composite_enable(gpio_dev *disc_dev, uint8 disc_bit, const uint8* report_descriptor, uint16 report_descriptor_length, 
    uint16 idVendor, uint16 idProduct, const uint8* iManufacturer, const uint8* iProduct
    ) {
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
     
    saved_Device_Table = Device_Table;
    saved_Device_Property = Device_Property;
    saved_User_Standard_Requests = User_Standard_Requests;

    Device_Table = my_Device_Table;
    Device_Property = my_Device_Property;
    User_Standard_Requests = my_User_Standard_Requests;

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
    
#ifdef GENERIC_BOOTLOADER			
    //Reset the USB interface on generic boards - developed by Victor PV
    gpio_set_mode(GPIOA, 12, GPIO_OUTPUT_PP);
    gpio_write_bit(GPIOA, 12, 0);
    
    for(volatile unsigned int i=0;i<512;i++);// Only small delay seems to be needed
    gpio_set_mode(GPIOA, 12, GPIO_INPUT_FLOATING);
#endif			

    if (disc_dev != NULL) {
        gpio_set_mode(disc_dev, disc_bit, GPIO_OUTPUT_PP);
        gpio_write_bit(disc_dev, disc_bit, 0);
    }

    /* Initialize the USB peripheral. */
    usb_init_usblib(USBLIB, ep_int_in, ep_int_out);
}

static void usb_power_down() {
    USB_BASE->CNTR = USB_CNTR_FRES;
    USB_BASE->ISTR = 0;
    USB_BASE->CNTR = USB_CNTR_FRES + USB_CNTR_PDWN;
}

void usb_composite_disable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
    if (disc_dev != NULL) {
        gpio_write_bit(disc_dev, disc_bit, 1);
    }
    
    Device_Table = saved_Device_Table;
    Device_Property = saved_Device_Property;
    User_Standard_Requests = saved_User_Standard_Requests;
    
    featureBufferCount = 0;
    outputBufferCount = 0;

    usb_power_down();
}

void usb_hid_putc(char ch) {
    while (!usb_hid_tx((uint8*)&ch, 1))
        ;
}

static void hidStatusIn() {
    /*
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
    */
}

void usb_hid_set_feature(uint8_t reportID, uint8_t* data) {
    for(int i=0;i<featureBufferCount;i++) {
        if (featureBuffers[i].reportID == reportID) {
            usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_NAK);
            unsigned delta = reportID != 0;
            memcpy((uint8_t*)featureBuffers[i].buffer+delta, data, featureBuffers[i].bufferSize-delta);
            if (reportID)
                featureBuffers[i].buffer[0] = reportID;
            featureBuffers[i].currentDataSize = featureBuffers[i].bufferSize;
            featureBuffers[i].state = HID_BUFFER_READ;
            usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);
            return;
        }
    }
}

uint16_t usb_hid_get_data(uint8_t type, uint8_t reportID, uint8_t* out, uint8_t poll) {
    volatile HIDBuffer_t* bufs;
    int n;
    
    if (type == HID_REPORT_TYPE_FEATURE) {
        bufs = featureBuffers;
        n = featureBufferCount;
    }
    else if (type == HID_REPORT_TYPE_OUTPUT) {
        bufs = outputBuffers;
        n = outputBufferCount;
    }
    else {
        return 0;
    }
    
    for(int i=0;i<n;i++) {
        if (bufs[i].reportID == reportID) {
            if (bufs[i].state == HID_BUFFER_EMPTY || (poll && bufs[i].state == HID_BUFFER_READ)) {
                    
               return 0;
            }            
            
            if (bufs[i].bufferSize != bufs[i].currentDataSize) {
               usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);
               return 0;
            }
 
            nvic_irq_disable(NVIC_USB_LP_CAN_RX0);

            unsigned delta = reportID != 0;
            memcpy(out, (uint8*)bufs[i].buffer+delta, bufs[i].bufferSize-delta);
            if (poll) {
                bufs[i].state = HID_BUFFER_READ;
            }

            nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
            usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);
            
            return bufs[i].bufferSize-delta;
        }
    }

    return 0;
}

void usb_hid_set_buffers(uint8_t type, volatile HIDBuffer_t* bufs, int n) {
    if (type == HID_REPORT_TYPE_FEATURE) {
        featureBuffers = bufs;
        featureBufferCount = n;
    }
    else {
        outputBuffers = bufs;
        outputBufferCount = n;
    }
    for (int i=0; i<n; i++) {
        bufs[i].state = HID_BUFFER_EMPTY;
        bufs[i].buffer[0] = featureBuffers[i].reportID;
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
    // We can only send up to USB_CDCACM_TX_EPSIZE bytes in the endpoint.
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



#ifdef USB_HID_RX_SUPPORT
uint32 usb_hid_peek(uint8* buffer, uint32 n) {
    uint32 offset = rx_offset;
    uint32 toRead = n_unread_bytes;
    if (toRead == 0)
        return 0;
    if (n < toRead)
        toRead = n;
    for (unsigned i=0; i<toRead; i++)
        buffer[i] = hidBufferRx[(offset+i) % HID_BUFFER_SIZE];
    return toRead; 
}

uint32 usb_hid_data_available(void) {
    return (hid_rx_head - hid_rx_tail) & HID_RX_BUFFER_SIZE_MASK;
}

/* Non-blocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
/* Non-blocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 usb_hid_rx(uint8* buf, uint32 len)
{
    /* Copy bytes to buffer. */
    uint32 n_copied = usb_hid_peek(buf, len);

    /* Mark bytes as read. */
	uint16 tail = hid_rx_tail; // load volatile variable
	tail = (tail + n_copied) & HID_RX_BUFFER_SIZE_MASK;
	hid_rx_tail = tail; // store volatile variable

	uint32 rx_unread = (hid_rx_head - tail) & HID_RX_BUFFER_SIZE_MASK;
    // If buffer was emptied to a pre-set value, re-enable the RX endpoint
    if ( rx_unread <= 64 ) { // experimental value, gives the best performance
        usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
	}
    return n_copied;
}

/* Non-blocking byte lookahead.
 *
 * Looks at unread bytes without marking them as read. */
uint32 usb_hid_peek(uint8* buf, uint32 len)
{
    int i;
    uint32 tail = hid_rx_tail;
	uint32 rx_unread = (hid_rx_head-tail) & HID_RX_BUFFER_SIZE_MASK;

    if (len > rx_unread) {
        len = rx_unread;
    }

    for (i = 0; i < len; i++) {
        buf[i] = hidBufferRx[tail];
        tail = (tail + 1) & HID_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

static void hidDataRxCb(void) {
	uint32 ep_rx_size;
	uint32 tail = (rx_offset + n_unread_bytes) % HID_BUFFER_SIZE;
	uint8 ep_rx_data[USB_HID_RX_EPSIZE];
	uint32 i;

    usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_NAK);
    ep_rx_size = usb_get_ep_rx_count(USB_HID_RX_ENDP);
    /* This copy won't overwrite unread bytes, since we've set the RX
     * endpoint to NAK, and will only set it to VALID when all bytes
     * have been read. */
    usb_copy_from_pma((uint8*)ep_rx_data, ep_rx_size,
                      USB_HID_RX_ADDR);

	for (i = 0; i < ep_rx_size; i++) {
		hidBufferRx[tail] = ep_rx_data[i];
		tail = (tail + 1) % HID_BUFFER_SIZE;
	}

	n_unread_bytes += ep_rx_size;

    if (n_unread_bytes <= (HID_BUFFER_SIZE - USB_HID_RX_EPSIZE)) {
        usb_set_ep_rx_count(USB_HID_RX_ENDP, USB_HID_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
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
    usb_set_ep_rx_addr(USB_EP0, USB_CDCACM_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_CDCACM_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

#ifdef COMPOSITE_SERIAL    
    /* setup management endpoint 1  */
    usb_set_ep_type(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_EP_TYPE_INTERRUPT);
    usb_set_ep_tx_addr(USB_CDCACM_MANAGEMENT_ENDP,
                       USB_CDCACM_MANAGEMENT_ADDR);
    usb_set_ep_tx_stat(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_RX_DISABLED);

    /* TODO figure out differences in style between RX/TX EP setup */

    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_CDCACM_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_CDCACM_RX_ENDP, USB_CDCACM_RX_ADDR);
    usb_set_ep_rx_count(USB_CDCACM_RX_ENDP, USB_CDCACM_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_CDCACM_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_CDCACM_TX_ENDP, USB_CDCACM_TX_ADDR);
    usb_set_ep_tx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_RX_DISABLED);
#endif

#ifdef USB_HID_RX_SUPPORT	
    /* set up hid endpoint OUT (RX) */
    usb_set_ep_type(USB_HID_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_HID_RX_ENDP, USB_HID_RX_ADDR);
    usb_set_ep_rx_count(USB_HID_RX_ENDP, USB_HID_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
#endif
    
    /* set up hid endpoint IN (TX)  */
    usb_set_ep_type(USB_HID_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_HID_TX_ENDP, USB_HID_TX_ADDR);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_HID_TX_ENDP, USB_EP_STAT_RX_DISABLED);
    
    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

    /* Reset the RX/TX state */

#ifdef COMPOSITE_SERIAL    
    //VCOM
	vcom_rx_head = 0;
	vcom_rx_tail = 0;
	vcom_tx_head = 0;
	vcom_tx_tail = 0;
#endif
    
	transmitting = -1;
   
    /* Reset the RX/TX state */
	hid_tx_head = 0;
	hid_tx_tail = 0;
#ifdef USB_HID_RX_SUPPORT    
	hid_rx_head = 0;
	hid_rx_tail = 0;
#endif    
}

static uint8* HID_Set(uint16 length) {
    if (currentHIDBuffer == NULL)
        return NULL;
    
    if (length ==0) {
        if (currentHIDBuffer->state == HID_BUFFER_UNREAD && pInformation->Ctrl_Info.Usb_wOffset < pInformation->USBwLengths.w) {
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

static int get_buffer_index(volatile HIDBuffer_t* buf, int bufferCount, uint8_t reportID) {
    for (int i=0; i<bufferCount; i++) 
        if (reportID == buf[i].reportID) 
            return i;
    return -1;
}

static RESULT usbDataSetup(uint8 request) {
    uint8* (*CopyRoutine)(uint16) = 0;
    
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {        
        switch (request) {
#ifdef COMPOSITE_SERIAL            
        case USB_CDCACM_GET_LINE_CODING:
            CopyRoutine = vcomGetSetLineCoding;
            break;
        case USB_CDCACM_SET_LINE_CODING:
            CopyRoutine = vcomGetSetLineCoding;
            break;
#endif            
        case SET_REPORT:
            if (pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){                
                if (pInformation->USBwValue1 == HID_REPORT_TYPE_FEATURE) {
                    int i = get_buffer_index(featureBuffers, featureBufferCount, pInformation->USBwValue0);
                        
                    if (i < 0) {
                        return USB_UNSUPPORT;
                    }
                    
                    if (featureBuffers[i].state == HID_BUFFER_UNREAD) {
                        return USB_NOT_READY;
                    } 
                    else 
                    {
                        currentHIDBuffer = featureBuffers + i;
                        CopyRoutine = HID_Set;        
                    }
                }
                else if (pInformation->USBwValue1 == HID_REPORT_TYPE_OUTPUT) {
                    int i = get_buffer_index(outputBuffers, outputBufferCount, pInformation->USBwValue0);
                        
                    if (i < 0) {
                        return USB_UNSUPPORT;
                    }
                    
                    if (outputBuffers[i].state == HID_BUFFER_UNREAD) {
                        return USB_NOT_READY;
                    }
                    else  
                    {
                        currentHIDBuffer = outputBuffers + i;
                        CopyRoutine = HID_Set;        
                    }
                }
            }
            break;
        case GET_REPORT:
            if (pInformation->USBwValue1 == HID_REPORT_TYPE_FEATURE &&
                pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){						
            int i = get_buffer_index(featureBuffers, featureBufferCount, pInformation->USBwValue0);
            
            if (i < 0 || featureBuffers[i].state == HID_BUFFER_EMPTY) {
                return USB_UNSUPPORT;
            }

            currentHIDBuffer = featureBuffers + i;
            CopyRoutine = HID_GetFeature;        
            break;
        }
        default:
            break;
        }
#ifdef COMPOSITE_SERIAL            
        /* Call the user hook. */
        if (iface_setup_hook) {
            uint8 req_copy = request;
            iface_setup_hook(USB_CDCACM_HOOK_IFACE_SETUP, &req_copy);
        }
#endif        
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
#ifdef COMPOSITE_SERIAL        
            case USB_CDCACM_SET_COMM_FEATURE:
	            /* We support set comm. feature, but don't handle it. */
	            ret = USB_SUCCESS;
	            break;
	        case USB_CDCACM_SET_CONTROL_LINE_STATE:
	            /* Track changes to DTR and RTS. */
	            line_dtr_rts = (pInformation->USBwValues.bw.bb0 &
	                            (USB_CDCACM_CONTROL_LINE_DTR |
	                             USB_CDCACM_CONTROL_LINE_RTS));
	            ret = USB_SUCCESS;
	            break;
#endif                
            case SET_PROTOCOL:
                ProtocolValue = pInformation->USBwValue0;
                ret = USB_SUCCESS;
                break;
        }
    }
#ifdef COMPOSITE_SERIAL            
    /* Call the user hook. */
    if (iface_setup_hook) {
        uint8 req_copy = request;
        iface_setup_hook(USB_CDCACM_HOOK_IFACE_SETUP, &req_copy);
    }
#endif    
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

    if (wValue0 > N_STRING_DESCRIPTORS) {
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
