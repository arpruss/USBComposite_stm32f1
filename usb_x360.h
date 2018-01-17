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

#ifndef _USB_X360_H
#define _USB_X360_H

#include <libmaple/libmaple_types.h>
#include <libmaple/gpio.h>
#include <libmaple/usb.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Descriptors, etc.
 */
 
//extern const uint8_t hid_report_descriptor[];

/*
 * Endpoint configuration
 */

#define USB_X360_CTRL_ENDP            0
#define USB_X360_CTRL_RX_ADDR         0x40
#define USB_X360_CTRL_TX_ADDR         0x80
#define USB_X360_CTRL_EPSIZE          0x40

#define USB_X360_TX_ENDP              1
#define USB_X360_TX_ADDR              0xC0
#define USB_X360_TX_EPSIZE            0x40

#define USB_X360_RX_ENDP              2
#define USB_X360_RX_ADDR              0x100
#define USB_X360_RX_EPSIZE            0x40

#ifndef __cplusplus
#define USB_X360_DECLARE_DEV_DESC(vid, pid)                           \
  {                                                                     \
      .bLength            = sizeof(usb_descriptor_device),              \
      .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,                 \
      .bcdUSB             = 0x0200,                                     \
      .bDeviceClass       = 255,                       \
      .bDeviceSubClass    = 255,                    \
      .bDeviceProtocol    = 255,                                       \
      .bMaxPacketSize0    = 0x40,                                       \
      .idVendor           = vid,                                        \
      .idProduct          = pid,                                        \
      .bcdDevice          = 0x0200,                                     \
      .iManufacturer      = 0x01,                                       \
      .iProduct           = 0x02,                                       \
      .iSerialNumber      = 0x00,                                       \
      .bNumConfigurations = 0x01,                                       \
 }
#endif

/*
 * HID interface
 */

void x360_enable();
void x360_disable();

void   x360_putc(char ch);
uint32 x360_tx(const uint8* buf, uint32 len);
uint32 x360_rx(uint8* buf, uint32 len);
uint32 x360_hid_peek(uint8* buf, uint32 len);
uint32 x360_data_available(void); /* in RX buffer */
uint16 x360_get_pending(void);
uint8 x360_is_transmitting(void);
void x360_set_rx_callback(void (*callback)(const uint8* buffer, uint32 size));
void x360_set_rumble_callback(void (*callback)(uint8 left, uint8 right));
void x360_set_led_callback(void (*callback)(uint8 pattern));

#ifdef __cplusplus
}
#endif

#endif
