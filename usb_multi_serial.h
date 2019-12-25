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
 * @file libmaple/include/libmaple/usb_device.h
 * @brief USB Composite with CDC ACM and HID support
 *
 * IMPORTANT: this API is unstable, and may change without notice.
 */

#ifndef _USB_MULTI_SERIAL_H_
#define _USB_MULTI_SERIAL_H_

#include <libmaple/libmaple_types.h>
#include <libmaple/usb.h>
#include "usb_generic.h"
#include "usb_composite_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USB_MULTI_SERIAL_MAX_PORTS 3

void multi_serial_initialize_port_data(uint8 numPorts);

extern USBCompositePart usbMultiSerialPart;

/*
 * CDC ACM interface
 */

void   multi_serial_putc(uint8 port, char ch);
uint32 multi_serial_tx(uint8 port, const uint8* buf, uint32 len);
uint32 multi_serial_rx(uint8 port, uint8* buf, uint32 len);
uint32 multi_serial_peek(uint8 port, uint8* buf, uint32 len);
uint32 multi_serial_peek_ex(uint8 port, uint8* buf, uint32 offset, uint32 len);
void multi_serial_setTXEPSize(uint8 port, uint32_t size);
void multi_serial_setRXEPSize(uint8 port, uint32_t size);

uint32 multi_serial_data_available(uint8 port); /* in RX buffer */
uint16 multi_serial_get_pending(uint8 port);

uint8 multi_serial_get_dtr(uint8 port);
uint8 multi_serial_get_rts(uint8 port);

/* Retrieve a copy of the current line coding structure. */
void multi_serial_get_line_coding(uint8, composite_cdcacm_line_coding*);

/* Line coding conveniences. */
int multi_serial_get_baud(uint8 port);        /* dwDTERate */
int multi_serial_get_stop_bits(uint8 port);   /* bCharFormat */
int multi_serial_get_parity(uint8 port);      /* bParityType */
int multi_serial_get_n_data_bits(uint8 port); /* bDataBits */

/*
 * Hack: hooks for bootloader reset signalling
 */

void multi_serial_set_hooks(uint8 port, unsigned hook_flags, void (*hook)(unsigned, void*));

#define multi_serial_remove_hooks(port, hook_flags) multi_serial_remove_hooks(port, hook_flags, 0)

#ifdef __cplusplus
}
#endif

#endif
