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

static DEVICE saved_Device_Table;
static DEVICE_PROP saved_Device_Property;
static USER_STANDARD_REQUESTS saved_User_Standard_Requests;
 
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
    
    Device_Table = saved_Device_Table;
    Device_Property = saved_Device_Property;
    User_Standard_Requests = saved_User_Standard_Requests;
    
    usb_power_down();
}
