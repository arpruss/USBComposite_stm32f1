/* Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

/**
 * @brief USB HID Keyboard device 
 */
 
#include "USBHID.h"

#ifdef COMPOSITE_SERIAL

#include <string.h>
#include <stdint.h>
#include <libmaple/nvic.h>
#include "usb_composite.h"
#include <libmaple/usb.h>
#include <string.h>
#include <libmaple/iwdg.h>

#include <wirish.h>

/*
 * USB HID interface
 */

#define USB_TIMEOUT 50

/*
 * USBSerial interface
 */

USBCompositeSerial::USBCompositeSerial(void) {
#if !BOARD_HAVE_SERIALUSB
    ASSERT(0);
#endif
}

void USBCompositeSerial::begin(void) {
}

//Roger Clark. Two new begin functions has been added so that normal Arduino Sketches that use Serial.begin(xxx) will compile.
void USBCompositeSerial::begin(unsigned long ignoreBaud) 
{
	volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
}
void USBCompositeSerial::begin(unsigned long ignoreBaud, uint8_t ignore)
{
	volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;
	volatile uint8_t removeCompilerWarningsIgnore=ignore;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
	ignore=removeCompilerWarningsIgnore;
}

void USBCompositeSerial::end(void) {
}

size_t USBCompositeSerial::write(uint8 ch) {
size_t n = 0;
    this->write(&ch, 1);
		return n;
}

size_t USBCompositeSerial::write(const char *str) {
size_t n = 0;
    this->write((const uint8*)str, strlen(str));
	return n;
}

size_t USBCompositeSerial::write(const uint8 *buf, uint32 len)
{
size_t n = 0;
    if (!this->isConnected() || !buf) {
        return 0;
    }

    uint32 txed = 0;
    while (txed < len) {
        txed += composite_cdcacm_tx((const uint8*)buf + txed, len - txed);
    }

	return n;
}

int USBCompositeSerial::available(void) {
    return composite_cdcacm_data_available();
}

int USBCompositeSerial::peek(void)
{
    uint8 b;
	if (composite_cdcacm_peek(&b, 1)==1)
	{
		return b;
	}
	else
	{
		return -1;
	}
}

void USBCompositeSerial::flush(void)
{
/*Roger Clark. Rather slow method. Need to improve this */
    uint8 b;
	while(composite_cdcacm_data_available())
	{
		this->read(&b, 1);
	}
    return;
}

uint32 USBCompositeSerial::read(uint8 * buf, uint32 len) {
    uint32 rxed = 0;
    while (rxed < len) {
        rxed += composite_cdcacm_rx(buf + rxed, len - rxed);
    }

    return rxed;
}

/* Blocks forever until 1 byte is received */
int USBCompositeSerial::read(void) {
    uint8 b;
	/*
	    this->read(&b, 1);
    return b;
	*/
	
	if (composite_cdcacm_rx(&b, 1)==0)
	{
		return -1;
	}
	else
	{
		return b;
	}
}

uint8 USBCompositeSerial::pending(void) {
    return composite_cdcacm_get_pending();
}

uint8 USBCompositeSerial::isConnected(void) {
    return usb_is_connected(USBLIB) && usb_is_configured(USBLIB) && composite_cdcacm_get_dtr();
}

uint8 USBCompositeSerial::getDTR(void) {
    return composite_cdcacm_get_dtr();
}

uint8 USBCompositeSerial::getRTS(void) {
    return composite_cdcacm_get_rts();
}

USBCompositeSerial CompositeSerial;
#endif
