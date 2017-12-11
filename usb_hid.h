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
 * @file libmaple/include/libmaple/usb_hid.h
 * @brief USB HID (human interface device) support
 *
 * IMPORTANT: this API is unstable, and may change without notice.
 */

#ifndef _LIBMAPLE_USB_HID_H_
#define _LIBMAPLE_USB_HID_H_

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

typedef enum _HID_REQUESTS
{
 
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,
 
  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
 
} HID_REQUESTS;

#define USB_ENDPOINT_IN(addr)           ((addr) | 0x80)
#define HID_ENDPOINT_INT 				1
#define USB_ENDPOINT_TYPE_INTERRUPT     0x03
 
#define HID_DESCRIPTOR_TYPE             0x21
 
#define REPORT_DESCRIPTOR               0x22

#define HID_SET_REPORT 0x09
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03

#define FEATURE_BUFFER_EMPTY    0
#define FEATURE_BUFFER_UNREAD   1
#define FEATURE_BUFFER_READ     2

typedef struct {
    uint8_t* buffer;
    uint8_t  bufferLength;
    uint8_t  reportID;
    uint8_t  dataSize;
    uint8_t  state;
} HIDFeatureBuffer_t;

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

#define USB_DEVICE_CLASS_HID              0x00
#define USB_DEVICE_SUBCLASS_HID           0x00
#define USB_INTERFACE_CLASS_HID           0x03
#define USB_INTERFACE_SUBCLASS_HID		  0x00
#define USB_INTERFACE_CLASS_DIC           0x0A

/*
 * Endpoint configuration
 */

#define USB_HID_CTRL_ENDP            0
#define USB_HID_CTRL_RX_ADDR         0x40
#define USB_HID_CTRL_TX_ADDR         0x80
#define USB_HID_CTRL_EPSIZE          0x40

#define USB_HID_TX_ENDP              1
#define USB_HID_TX_ADDR              0xC0
#define USB_HID_TX_EPSIZE            0x40

#define USB_HID_RX_ENDP              2
#define USB_HID_RX_ADDR              0x100
#define USB_HID_RX_EPSIZE            0x40

#ifndef __cplusplus
#define USB_HID_DECLARE_DEV_DESC(vid, pid)                           \
  {                                                                     \
      .bLength            = sizeof(usb_descriptor_device),              \
      .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,                 \
      .bcdUSB             = 0x0200,                                     \
      .bDeviceClass       = USB_DEVICE_CLASS_HID,                       \
      .bDeviceSubClass    = USB_DEVICE_SUBCLASS_HID,                    \
      .bDeviceProtocol    = 0x00,                                       \
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

void usb_hid_enable(gpio_dev *disc_dev, uint8 disc_bit, const uint8* report_descriptor, uint16 report_descriptor_length, 
    uint16 idVendor, uint16 idProduct, const uint8* iManufacturer, const uint8* iProduct);
void usb_hid_set_feature_buffers(HIDFeatureBuffer_t* featureBuffers, int count);    
uint8_t* usb_hid_get_feature(uint8_t reportID, uint8_t poll);
void usb_hid_disable(gpio_dev*, uint8);
void usb_hid_set_feature(uint8_t reportID, uint8_t* data);

uint32 usb_hid_peek(uint8* buffer, uint32 n);
void   usb_hid_putc(char ch);
uint32 usb_hid_tx(const uint8* buf, uint32 len);
uint32 usb_hid_tx_mod(const uint8* buf, uint32 len);
uint32 usb_hid_rx(uint8* buf, uint32 len);

uint32 usb_hid_data_available(void); /* in RX buffer */
uint16 usb_hid_get_pending(void);
uint8 usb_hid_is_transmitting(void);

void	HID_SendReport(uint8_t id, const void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
