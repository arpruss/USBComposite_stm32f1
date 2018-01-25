/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs LLC.
 * Copyright (c) 2013 Magnus Lundin.
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
 * @file libmaple/usb/stm32f1/usb_midi_device.c
 * @brief USB MIDI.
 *
 * FIXME: this works on the STM32F1 USB peripherals, and probably no
 * place else. Nonportable bits really need to be factored out, and
 * the result made cleaner.
 */

#include "usb_generic.h"
#include "usb_midi_device.h"
#include <MidiSpecs.h>
#include <MinSysex.h>

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

static void midiDataTxCb(void);
static void midiDataRxCb(void);

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

/*
 * Descriptors
 */

/* FIXME move to Wirish */
#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0014
/* 0x7D = ED/FREE next two DIGITS MUST BE LESS THAN 0x7f */

static const usb_descriptor_device usbMIDIDescriptor_Device =
    USB_MIDI_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);

typedef struct {
    usb_descriptor_config_header       Config_Header;
    /* Control Interface */
    usb_descriptor_interface           AC_Interface;
    AC_CS_INTERFACE_DESCRIPTOR(1)      AC_CS_Interface;
    /* Control Interface */
    usb_descriptor_interface           MS_Interface;
    MS_CS_INTERFACE_DESCRIPTOR         MS_CS_Interface;
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_1;
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_2;
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_3;
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_4;
    usb_descriptor_endpoint            DataOutEndpoint;
    MS_CS_BULK_ENDPOINT_DESCRIPTOR(1)  MS_CS_DataOutEndpoint;
    usb_descriptor_endpoint            DataInEndpoint;
    MS_CS_BULK_ENDPOINT_DESCRIPTOR(1)  MS_CS_DataInEndpoint;
} __packed usb_descriptor_config;

#define MAX_POWER (100 >> 1)
static const usb_descriptor_config usbMIDIDescriptor_Config = {
    .Config_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_descriptor_config),
        .bNumInterfaces       = 0x02,
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
    },

    .AC_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0x00,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x00,
        .bInterfaceClass    = USB_INTERFACE_CLASS_AUDIO,
        .bInterfaceSubClass = USB_INTERFACE_AUDIOCONTROL,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0x00,
    },

    .AC_CS_Interface = {
        .bLength            = AC_CS_INTERFACE_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = 0x01,
        .bcdADC             = 0x0100,
        .wTotalLength       = AC_CS_INTERFACE_DESCRIPTOR_SIZE(1),
        .bInCollection      = 0x01,
        .baInterfaceNr      = {0x01},
    },

    .MS_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0x01,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = USB_INTERFACE_CLASS_AUDIO,
        .bInterfaceSubClass = USB_INTERFACE_MIDISTREAMING,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0x04,
    },

    .MS_CS_Interface = {
        .bLength            = sizeof(MS_CS_INTERFACE_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = 0x01,
        .bcdADC             = 0x0100,
        .wTotalLength       = sizeof(MS_CS_INTERFACE_DESCRIPTOR)
                              +sizeof(MIDI_IN_JACK_DESCRIPTOR)
                              +sizeof(MIDI_IN_JACK_DESCRIPTOR)
                              +MIDI_OUT_JACK_DESCRIPTOR_SIZE(1)
                              +MIDI_OUT_JACK_DESCRIPTOR_SIZE(1)
                              +sizeof(usb_descriptor_endpoint)
                              +MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(1)
                              +sizeof(usb_descriptor_endpoint)
                              +MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(1)                             
                                 /* 0x41-4 */,
    },

    .MIDI_IN_JACK_1 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x01,
        .iJack              = 0x05,
    },

    .MIDI_IN_JACK_2 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x02,
        .iJack              = 0x00,
    },

    .MIDI_OUT_JACK_3 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x03,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x02},
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },

    .MIDI_OUT_JACK_4 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
//        .bJackId            = 0x04,
        .bJackId            = 0x03,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x01},
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },

    .DataOutEndpoint = {
        .bLength            = sizeof(usb_descriptor_endpoint),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress   = (USB_DESCRIPTOR_ENDPOINT_OUT |
                             USB_MIDI_RX_ENDP),
        .bmAttributes       = USB_EP_TYPE_BULK,
        .wMaxPacketSize     = USB_MIDI_RX_EPSIZE,
        .bInterval          = 0x00,
    },

    .MS_CS_DataOutEndpoint = {
      .bLength              = MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(1),
      .bDescriptorType      = USB_DESCRIPTOR_TYPE_CS_ENDPOINT,
      .SubType              = 0x01,
      .bNumEmbMIDIJack      = 0x01,
      .baAssocJackID        = {0x01},
	},

    .DataInEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_MIDI_TX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_MIDI_TX_EPSIZE,
        .bInterval        = 0x00,
    },

    .MS_CS_DataInEndpoint = {
      .bLength              = MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(1),
      .bDescriptorType      = USB_DESCRIPTOR_TYPE_CS_ENDPOINT,
      .SubType              = 0x01,
      .bNumEmbMIDIJack      = 0x01,
      .baAssocJackID        = {0x03},
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
static const usb_descriptor_string usbMIDIDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

static const usb_descriptor_string usbMIDIDescriptor_iManufacturer = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(8),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'L', 0, 'e', 0, 'a', 0, 'f', 0,
                'L', 0, 'a', 0, 'b', 0, 's', 0},
};

static const usb_descriptor_string usbMIDIDescriptor_iProduct = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(10),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'a', 0, 'p', 0, 'l', 0, 'e', 0, ' ', 0, 'M', 0, 'I', 0, 'D', 0, 'I', 0},
  //  .bString = {'D', 0, 'i', 0, 'r', 0, 'o', 0, ' ', 0, 'S', 0, 'y', 0, 'n', 0, 't', 0, 'h', 0},
};

static const usb_descriptor_string usbMIDIDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(4),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'I', 0, 'D', 0, 'I', 0},
};

static const usb_descriptor_string usbMIDIDescriptor_iJack1 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(5),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'J', 0, 'a', 0, 'c', 0, 'k', 0, '1', 0},
};


static ONE_DESCRIPTOR usbMidiDevice_Descriptor = {
    (uint8*)&usbMIDIDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR usbMidiConfig_Descriptor = {
    (uint8*)&usbMIDIDescriptor_Config,
    sizeof(usb_descriptor_config)
};

#define N_STRING_DESCRIPTORS 5
static ONE_DESCRIPTOR String_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbMIDIDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbMIDIDescriptor_iManufacturer,USB_DESCRIPTOR_STRING_LEN(8)},
    {(uint8*)&usbMIDIDescriptor_iProduct,     USB_DESCRIPTOR_STRING_LEN(10)},
    {(uint8*)&usbMIDIDescriptor_iInterface,     USB_DESCRIPTOR_STRING_LEN(4)},
    {(uint8*)&usbMIDIDescriptor_iJack1,     USB_DESCRIPTOR_STRING_LEN(5)}
};

/*
 * Etc.
 */

/* I/O state */

/* Received data */
static volatile uint32 midiBufferRx[USB_MIDI_RX_EPSIZE/4];
/* Read index into midiBufferRx */
static volatile uint32 rx_offset = 0;
/* Transmit data */
static volatile uint32 midiBufferTx[USB_MIDI_TX_EPSIZE/4];
/* Write index into midiBufferTx */
static volatile uint32 tx_offset = 0;
/* Number of bytes left to transmit */
static volatile uint32 n_unsent_packets = 0;
/* Are we currently sending an IN packet? */
static volatile uint8 transmitting = 0;
/* Number of unread bytes */
static volatile uint32 n_unread_packets = 0;


// eventually all of this should be in a place for settings which can be written to flash.
volatile uint8 myMidiChannel = DEFAULT_MIDI_CHANNEL;
volatile uint8 myMidiDevice = DEFAULT_MIDI_DEVICE;
volatile uint8 myMidiCable = DEFAULT_MIDI_CABLE;
volatile uint8 myMidiID[] = { LEAFLABS_MMA_VENDOR_1,LEAFLABS_MMA_VENDOR_2,LEAFLABS_MMA_VENDOR_3,0};

/*
 * Endpoint callbacks
 */

static void (*ep_int_in[7])(void) =
    {midiDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
    {NOP_Process,
     midiDataRxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

/*
 * Globals required by usb_lib/
 *
 * These override core USB functionality which was declared __weak.
 */

#define NUM_ENDPTS                0x04
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
 * MIDI interface
 */

void usb_midi_enable(void) {
    usb_generic_enable(&my_Device_Table, &my_Device_Property, &my_User_Standard_Requests,  ep_int_in, ep_int_out);
}

void usb_midi_disable(void) {
    usb_generic_disable();
}

//void usb_midi_putc(char ch) {
//    while (!usb_midi_tx((uint8*)&ch, 1))
//        ;
//}

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
uint32 usb_midi_tx(const uint32* buf, uint32 packets) {
    uint32 bytes=packets*4;
    /* Last transmission hasn't finished, so abort. */
    if (usb_midi_is_transmitting()) {
		/* Copy to TxBuffer */
		
        return 0;  /* return len */
    }

    /* We can only put USB_MIDI_TX_EPSIZE bytes in the buffer. */
    if (bytes > USB_MIDI_TX_EPSIZE) {
        bytes = USB_MIDI_TX_EPSIZE;
        packets=bytes/4;
    }

    /* Queue bytes for sending. */
    if (packets) {
        usb_copy_to_pma((uint8 *)buf, bytes, USB_MIDI_TX_ADDR);
    }
    // We still need to wait for the interrupt, even if we're sending
    // zero bytes. (Sending zero-size packets is useful for flushing
    // host-side buffers.)
    usb_set_ep_tx_count(USB_MIDI_TX_ENDP, bytes);
    n_unsent_packets = packets;
    transmitting = 1;
    usb_set_ep_tx_stat(USB_MIDI_TX_ENDP, USB_EP_STAT_TX_VALID);

    return packets;
}

uint32 usb_midi_data_available(void) {
    return n_unread_packets;
}

uint8 usb_midi_is_transmitting(void) {
    return transmitting;
}

uint16 usb_midi_get_pending(void) {
    return n_unsent_packets;
}

/* Nonblocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 usb_midi_rx(uint32* buf, uint32 packets) {
    /* Copy bytes to buffer. */
    uint32 n_copied = usb_midi_peek(buf, packets);

    /* Mark bytes as read. */
    n_unread_packets -= n_copied;
    rx_offset += n_copied;

    /* If all bytes have been read, re-enable the RX endpoint, which
     * was set to NAK when the current batch of bytes was received. */
    if (n_unread_packets == 0) {
        usb_set_ep_rx_count(USB_MIDI_RX_ENDP, USB_MIDI_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_MIDI_RX_ENDP, USB_EP_STAT_RX_VALID);
        rx_offset = 0;
    }

    return n_copied;
}

/* Nonblocking byte lookahead.
 *
 * Looks at unread bytes without marking them as read. */
uint32 usb_midi_peek(uint32* buf, uint32 packets) {
    uint32 i;
    if (packets > n_unread_packets) {
        packets = n_unread_packets;
    }

    for (i = 0; i < packets; i++) {
        buf[i] = midiBufferRx[i + rx_offset];
    }

    return packets;
}

/*
 * Callbacks
 */

static void midiDataTxCb(void) {
    n_unsent_packets = 0;
    transmitting = 0;
}

static void midiDataRxCb(void) {
    usb_set_ep_rx_stat(USB_MIDI_RX_ENDP, USB_EP_STAT_RX_NAK);
    n_unread_packets = usb_get_ep_rx_count(USB_MIDI_RX_ENDP) / 4;
    /* This copy won't overwrite unread bytes, since we've set the RX
     * endpoint to NAK, and will only set it to VALID when all bytes
     * have been read. */
    
    usb_copy_from_pma((uint8*)midiBufferRx, n_unread_packets * 4,
                      USB_MIDI_RX_ADDR);
    
    // discard volatile
    LglSysexHandler((uint32*)midiBufferRx,(uint32*)&rx_offset,(uint32*)&n_unread_packets);
    
    if (n_unread_packets == 0) {
        usb_set_ep_rx_count(USB_MIDI_RX_ENDP, USB_MIDI_RX_EPSIZE);
        usb_set_ep_rx_stat(USB_MIDI_RX_ENDP, USB_EP_STAT_RX_VALID);
        rx_offset = 0;
    }

}

/* NOTE: Nothing specific to this device class in this function, move to usb_lib */
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
    usb_set_ep_rx_addr(USB_EP0, USB_MIDI_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_MIDI_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* TODO figure out differences in style between RX/TX EP setup */

    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_MIDI_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_MIDI_RX_ENDP, USB_MIDI_RX_ADDR);
    usb_set_ep_rx_count(USB_MIDI_RX_ENDP, USB_MIDI_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_MIDI_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_MIDI_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_MIDI_TX_ENDP, USB_MIDI_TX_ADDR);
    usb_set_ep_tx_stat(USB_MIDI_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_MIDI_TX_ENDP, USB_EP_STAT_RX_DISABLED);

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

    /* Reset the RX/TX state */
    n_unread_packets = 0;
    n_unsent_packets = 0;
    rx_offset = 0;
}

static RESULT usbDataSetup(uint8 request) {
    (void)request;//unused
    uint8* (*CopyRoutine)(uint16) = 0;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
    }

    if (CopyRoutine == NULL) {
        return USB_UNSUPPORT;
    }

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

static RESULT usbNoDataSetup(uint8 request) {
    (void)request;//unused
    RESULT ret = USB_UNSUPPORT;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
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
    return Standard_GetDescriptorData(length, &usbMidiDevice_Descriptor);
}

static uint8* usbGetConfigDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &usbMidiConfig_Descriptor);
}

static uint8* usbGetStringDescriptor(uint16 length) {
    uint8 wValue0 = pInformation->USBwValue0;

    if (wValue0 >= N_STRING_DESCRIPTORS) {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &String_Descriptor[wValue0]);
}

static void usbSetConfiguration(void) {
    if (pInformation->Current_Configuration != 0) {
        USBLIB->state = USB_CONFIGURED;
    }
}

static void usbSetDeviceAddress(void) {
    USBLIB->state = USB_ADDRESSED;
}
// .............THIS IS NOT WORKING YET................
// send debugging information to 
static uint8_t sysexbuffer[80]={CIN_SYSEX,0xF0,0x7D,0x33,CIN_SYSEX,0x33,0x00,0xf7}; // !!!bad hardcoded number foo !!!
uint8_t iSysHexLine(uint8_t rectype, uint16_t address, uint8_t *payload,uint8_t payloadlength, uint8_t *buffer);
void sendThroughSysex(char *printbuffer, int bufferlength) {
    int n;
    n = iSysHexLine(1, 0 , (uint8_t *) printbuffer, (uint8_t) bufferlength , sysexbuffer+6);
    usb_midi_tx((uint32*)sysexbuffer,n/4);
}

#define HIGHBYTE(x) ((uint8_t) (((x) >> 8) &  0x00ff) )
#define LOWBYTE(x) ((uint8_t) ((x) & 0x00ff) )
#define HIGHNIBBLE(x) ((((uint8_t)(x)) & 0xF0) >> 4)
#define LOWNIBBLE(x) (((uint8_t)(x)) & 0x0F)
#define HEXCHAR(c) ((c>9)?55+c:48+c)


uint8_t iSysHexLine(uint8_t rectype, uint16_t address, uint8_t *payload,uint8_t payloadlength, uint8_t *buffer) {
    
    int i=0; int j; int thirdone;
    uint8_t n=0;
    uint16_t checksum=0;
    //uint16_t length=0;
    
    buffer[i++]=':';
    
    checksum+=payloadlength;
    buffer[i++]=HEXCHAR(HIGHNIBBLE(payloadlength));
    buffer[i++]=HEXCHAR(LOWNIBBLE(payloadlength));
    buffer[i++]=CIN_SYSEX;
    
    n=HIGHBYTE(address);
    checksum+=n;
    buffer[i++]=HEXCHAR(HIGHNIBBLE(n));
    buffer[i++]=HEXCHAR(LOWNIBBLE(n));
    
    n=LOWBYTE(address);
    checksum+=n;
    buffer[i++]=HEXCHAR(HIGHNIBBLE(n));
    buffer[i++]=CIN_SYSEX;
    buffer[i++]=HEXCHAR(LOWNIBBLE(n));
    
    n=rectype;
    checksum+=n;
    buffer[i++]=HEXCHAR(HIGHNIBBLE(n));
    buffer[i++]=HEXCHAR(LOWNIBBLE(n));
    buffer[i++]=CIN_SYSEX;
    thirdone=0;
    for (j=0; j<payloadlength ; j++) {
        n=payload[j];
        checksum+=n;
        buffer[i++]=HEXCHAR(HIGHNIBBLE(n));
        if (++thirdone==3) {
            buffer[i++]=CIN_SYSEX;
            thirdone=0;
        }
        buffer[i++]=HEXCHAR(LOWNIBBLE(n));
        if (++thirdone==3) {
            buffer[i++]=CIN_SYSEX;
            thirdone=0;
        }
    }
    if (thirdone==0) {
        buffer[i-1]=CIN_SYSEX_ENDS_IN_3;
    } 
    n=~((uint8_t) checksum&0x00ff)+1;
    buffer[i++]=HEXCHAR(HIGHNIBBLE(n));
    if (thirdone==1) {
        buffer[i++]=CIN_SYSEX_ENDS_IN_2;
    }
    buffer[i++]=HEXCHAR(LOWNIBBLE(n));
    if (thirdone==2) {
        buffer[i++]=CIN_SYSEX_ENDS_IN_1;
    }
    buffer[i++]=0xf7;
    return i+thirdone;
}

