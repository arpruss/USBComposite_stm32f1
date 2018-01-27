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

#include "usb_generic.h"

#include <string.h>
#include <libmaple/libmaple_types.h>
#include <libmaple/usb.h>
#include <libmaple/nvic.h>
#include <libmaple/delay.h>
#include <libmaple/gpio.h>
#include "usb_lib_globals.h"
#include "usb_reg_map.h"

//uint16 GetEPTxAddr(uint8 /*bEpNum*/);

/* usb_lib headers */
#include "usb_type.h"
#include "usb_core.h"
#include "usb_def.h"

#include <board/board.h>

struct {
    usb_descriptor_config_header Config_Header;
    uint8 descriptorData[MAX_USB_DESCRIPTOR_DATA_SIZE];
} __packed usb_descriptor_config;

static const usb_descriptor_config_header Base_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = 0,
        .bNumInterfaces       = 0, 
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
};

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


static USBCompositePart* parts;
static uint32 numParts;
static DEVICE saved_Device_Table;
static DEVICE_PROP saved_Device_Property;
static USER_STANDARD_REQUESTS saved_User_Standard_Requests;

static void (*ep_int_in[7])(void);
static void (*ep_int_out[7])(void);

uint16 endpoint_in_

uint8 usb_generic_composite_setup(USBCompositePart** _parts, unsigned _numParts) {
    parts = _parts;
    numParts = _numParts;
    unsigned numInterfaces = 0;
    unsigned numEndpoints = 1;
    uint16 usbDescriptorSize = 0;
    uint16 pmaOffset = USB_EP0_RX_BUFFER_ADDRESS + USB_EP0_BUFFER_SIZE;
    
    usbDescriptorSize = 0;
    for (unsigned i = 0 ; i < _numParts ; i++ ) {
        parts[i]->startInterface = numInterfaces;
        numInterfaces += parts[i]->numInterfaces;
        if (numEndpoints + parts[i]->numEndpoints > 8)
            return 0;
        if (usbDescriptorSize + parts[i]->descriptorSize > MAX_USB_DESCRIPTOR_SIZE) 
            return 0;
        parts[i]->getPartDescriptor(parts[i], usb_descriptor_config->descriptorData + usbDescriptorSize);
        usbDescriptorSize += parts[i]->descriptorSize;
        EndpointInfo* ep = parts[i]->endpoints;
        for (unsigned j = 0 ; j < numEndpoints ; j++) {
            if (ep[j].bufferSize + pmaOffset > PMA_MEMORY_SIZE) 
                return 0;
            ep[j].pmaAddress = pmaOffset;
            pmaOffset += ep[j].bufferSize;
            ep[j].endpoint = numEndpoints;
            if (ep[j].callback == NULL)
                ep[j].callback = NOP_Process;
            if (ep[j].tx) {
                ep_int_in[numEndpoints - 1] = ep[j].callback;
            }
            else {
                ep_int_out[numEndpoints - 1] = ep[j].callback;
            }
            numEndpoints++;
        }
    }
    
    for (unsigned i = numEndpoints ; i < 8 ; i++) {
        ep_int_in[i-1] = NOP_Process;
        ep_int_out[i-1] = NOP_Process;
    }
    
    usb_descriptor_config.Config_Header = Base_Header;    
    usb_descriptor_config.Config_Header.bNumInterfaces = numInterfaces;
    usb_descriptor_config.Config_Header.wTotalLength = usbDescriptorSize + sizeof(Base_Header);
    
    my_Device_Table.Total_Endpoint = numEndpoints;
    
    return 1;
}
 
void usb_generic_enable(DEVICE* deviceTable, DEVICE_PROP* deviceProperty, USER_STANDARD_REQUESTS* userStandardRequests,
    void (**ep_int_in)(void), void (**ep_int_out)(void)) {
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
     
    saved_Device_Table = Device_Table;
    saved_Device_Property = Device_Property;
    saved_User_Standard_Requests = User_Standard_Requests;

    Device_Table = *deviceTable;
    Device_Property = *deviceProperty;
    User_Standard_Requests = *userStandardRequests;
    
#ifdef GENERIC_BOOTLOADER			
    //Reset the USB interface on generic boards - developed by Victor PV
    gpio_set_mode(GPIOA, 12, GPIO_OUTPUT_PP);
    gpio_write_bit(GPIOA, 12, 0);
    
    for(volatile unsigned int i=0;i<512;i++);// Only small delay seems to be needed
    gpio_set_mode(GPIOA, 12, GPIO_INPUT_FLOATING);
#endif			

    if (BOARD_USB_DISC_DEV != NULL) {
        gpio_set_mode(BOARD_USB_DISC_DEV, (uint8)(uint32)BOARD_USB_DISC_BIT, GPIO_OUTPUT_PP);
        gpio_write_bit(BOARD_USB_DISC_DEV, (uint8)(uint32)BOARD_USB_DISC_BIT, 0);
    }

    /* Initialize the USB peripheral. */
    usb_init_usblib(USBLIB, ep_int_in, ep_int_out);
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

    for (unsigned i = 0 ; i < numParts ; i++)
        parts[i]->usbInit(parts[i]);

    USBLIB->state = USB_UNCONNECTED;
}

#define BTABLE_ADDRESS        0x00
static void usbReset(void) {
    pInformation->Current_Configuration = 0;

    /* current feature is current bmAttributes */
    pInformation->Current_Feature = (USB_CONFIG_ATTR_BUSPOWERED |
                                     USB_CONFIG_ATTR_SELF_POWERED);

    USB_BASE->BTABLE = BTABLE_ADDRESS;

    for (unsigned i = 0 ; i < numParts ; i++)
        parts[i]->usbReset(parts[i]);

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

}

static void usb_power_down(void) {
    USB_BASE->CNTR = USB_CNTR_FRES;
    USB_BASE->ISTR = 0;
    USB_BASE->CNTR = USB_CNTR_FRES + USB_CNTR_PDWN;
}

void usb_generic_disable(void) {
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
    
    if (BOARD_USB_DISC_DEV != NULL) {
        gpio_write_bit(BOARD_USB_DISC_DEV, (uint8)(uint32)BOARD_USB_DISC_BIT, 1);
    }
    
    usb_power_down();

    Device_Table = saved_Device_Table;
    Device_Property = saved_Device_Property;
    User_Standard_Requests = saved_User_Standard_Requests;    
}

static RESULT usbDataSetup(uint8 request) {
    for (unsigned i = 0 ; i < numParts ; i++) {
        RESULT r = parts[i]->usbDataSetup(parts[i], request);
        if (USB_UNSUPPORT != r)
            return r;
    }

    return USB_UNSUPPORT;
}

static RESULT usbNoDataSetup(uint8 request) {
    for (unsigned i = 0 ; i < numParts ; i++) {
        RESULT r = parts[i]->usbNoDataSetup(parts[i], request);
        if (USB_UNSUPPORT != r)
            return r;
    }

    return USB_UNSUPPORT;
}

