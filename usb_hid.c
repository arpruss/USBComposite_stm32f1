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

#include "usb_hid.h"

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

static void hidDataTxCb(void);
static void hidDataRxCb(void);

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
static uint8* HID_GetHIDDescriptor(uint16 Length);
static uint8* HID_GetProtocolValue(uint16 Length);

/*
 * Descriptors
 */
 

const uint8_t hid_report_descriptor[] = {
};

/* FIXME move to Wirish */
#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0024
static usb_descriptor_device usbHIDDescriptor_Device =
    USB_HID_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);
	

typedef struct {
    usb_descriptor_config_header Config_Header;
    usb_descriptor_interface     HID_Interface;
	HIDDescriptor			 	 HID_Descriptor;
    usb_descriptor_endpoint      DataInEndpoint;
    usb_descriptor_endpoint      DataOutEndpoint;
} __packed usb_descriptor_config;


#define MAX_POWER (100 >> 1)
static usb_descriptor_config usbHIDDescriptor_Config =
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
	
	.DataInEndpoint = {
		.bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_HID_TX_ENDP),//0x81,//USB_HID_TX_ADDR,
        .bmAttributes     = USB_ENDPOINT_TYPE_INTERRUPT,
        .wMaxPacketSize   = USB_HID_TX_EPSIZE,//0x40,//big enough for a keyboard 9 byte packet and for a mouse 5 byte packet
        .bInterval        = 0x0A,
	},

    .DataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT | USB_HID_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_HID_RX_EPSIZE,
        .bInterval        = 0x02,
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
 
static ONE_DESCRIPTOR HID_Report_Descriptor = {
    (uint8*)&hid_report_descriptor,
    sizeof(hid_report_descriptor)
};

#define N_STRING_DESCRIPTORS 4
static ONE_DESCRIPTOR usbHIDString_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbHIDDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbHIDDescriptor_iManufacturer,         USB_DESCRIPTOR_STRING_LEN(default_iManufacturer_length)},
    {(uint8*)&usbHIDDescriptor_iProduct,              USB_DESCRIPTOR_STRING_LEN(default_iProduct_length)},
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
     hidDataRxCb,//NOP_Process,
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

#define NUM_ENDPTS                0x02
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

DEVICE saved_Device_Table;
DEVICE_PROP saved_Device_Property;
USER_STANDARD_REQUESTS saved_User_Standard_Requests;

/*
 * HID interface
 */

void usb_hid_enable(gpio_dev *disc_dev, uint8 disc_bit, const uint8* report_descriptor, uint16 report_descriptor_length, 
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

    HID_Report_Descriptor.Descriptor = report_descriptor;
    HID_Report_Descriptor.Descriptor_Size = report_descriptor_length;        
    usbHIDDescriptor_Config.HID_Descriptor.descLenL = (uint8_t)report_descriptor_length;
    usbHIDDescriptor_Config.HID_Descriptor.descLenH = (uint8_t)(report_descriptor_length>>8);
        
    if (idVendor != 0)
        usbHIDDescriptor_Device.idVendor = idVendor;
    else
        usbHIDDescriptor_Device.idVendor = LEAFLABS_ID_VENDOR;
     
    if (idProduct != 0)
        usbHIDDescriptor_Device.idProduct = idProduct;
    else
        usbHIDDescriptor_Device.idProduct = MAPLE_ID_PRODUCT;
    
    if (iManufacturer == NULL) {
        iManufacturer = (uint8*)&usbHIDDescriptor_iManufacturer;
    }
           
    usbHIDString_Descriptor[1].Descriptor = iManufacturer;
    usbHIDString_Descriptor[1].Descriptor_Size = iManufacturer[0];
     
    if (iProduct == NULL) {
        iProduct = (uint8*)&usbHIDDescriptor_iProduct;
    }
           
    usbHIDString_Descriptor[2].Descriptor = iProduct;
    usbHIDString_Descriptor[2].Descriptor_Size = iProduct[0];
    
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

void usb_hid_disable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
    if (disc_dev != NULL) {
        gpio_write_bit(disc_dev, disc_bit, 1);
    }
    
    Device_Table = saved_Device_Table;
    Device_Property = saved_Device_Property;
    User_Standard_Requests = saved_User_Standard_Requests;

    usb_power_down();
}

void usb_hid_putc(char ch) {
    while (!usb_hid_tx((uint8*)&ch, 1))
        ;
}


 /* TODO these could use some improvement; they're fairly
 * straightforward ports of the analogous ST code.  The PMA blit
 * routines in particular are obvious targets for performance
 * measurement and tuning. */

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


/* This function is non-blocking.
 *
 * It copies data from a usercode buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 usb_hid_tx(const uint8* buf, uint32 len) {
    /* Last transmission hasn't finished, so abort. */
    if (usb_hid_is_transmitting()) {
        return 0;
    }

    /* We can only put USB_HID_TX_EPSIZE bytes in the buffer. */
    if (len > USB_HID_TX_EPSIZE) {
        len = USB_HID_TX_EPSIZE;
    }

    /* Queue bytes for sending. */
    if (len) {
        usb_copy_to_pma(buf, len, GetEPTxAddr(USB_HID_TX_ENDP));//USB_HID_TX_ADDR);
    }
    // We still need to wait for the interrupt, even if we're sending
    // zero bytes. (Sending zero-size packets is useful for flushing
    // host-side buffers.)
    usb_set_ep_tx_count(USB_HID_TX_ENDP, len);
    n_unsent_bytes = len;
    transmitting = 1;
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_VALID);

    return len;
}

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
    return n_unread_bytes;
}

uint8 usb_hid_is_transmitting(void) {
    return transmitting;
}

uint16 usb_hid_get_pending(void) {
    return n_unsent_bytes;
}

/* Nonblocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 usb_hid_rx(uint8* buf, uint32 len) {
    /* Copy bytes to buffer. */
    uint32 n_copied = usb_hid_peek(buf, len);

    /* Mark bytes as read. */
    n_unread_bytes -= n_copied;
    rx_offset = (rx_offset + n_copied) % HID_BUFFER_SIZE;

    /* If all bytes have been read, re-enable the RX endpoint, which
     * was set to NAK when the current batch of bytes was received. */
    if (n_unread_bytes <= (HID_BUFFER_SIZE - USB_HID_RX_EPSIZE)) {
        usb_set_ep_rx_count(USB_HID_RX_ENDP, USB_HID_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
		rx_offset = 0;
    }

    return n_copied;
}

/*
 * Callbacks
 */

static void hidDataTxCb(void) {
    n_unsent_bytes = 0;
    transmitting = 0;
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
    usb_set_ep_rx_addr(USB_EP0, USB_HID_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_HID_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* TODO figure out differences in style between RX/TX EP setup */

    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_HID_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_HID_RX_ENDP, USB_HID_RX_ADDR);
    usb_set_ep_rx_count(USB_HID_RX_ENDP, USB_HID_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_HID_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_HID_TX_ENDP, USB_HID_TX_ADDR);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_HID_TX_ENDP, USB_EP_STAT_RX_DISABLED);

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
			
		if (pInformation->USBwValue1 == REPORT_DESCRIPTOR){
			CopyRoutine = HID_GetReportDescriptor;
		} else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE){
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

    if (wValue0 > N_STRING_DESCRIPTORS) {
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

static uint8* HID_GetReportDescriptor(uint16 Length){
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}

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
