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
 
#include "usb_hid_device.h"

#include <string.h>
#include <stdint.h>
#include <libmaple/nvic.h>
#include "usb_hid.h"
#include <libmaple/usb.h>
#include <string.h>

#include <wirish.h>

/*
 * USB HID interface
 */

template<int... args> static constexpr int countIntegers() {
    return sizeof...(args);
}

static uint8_t reportDescriptors[] = {
        USB_HID_MOUSE_REPORT_DESCRIPTOR(USB_HID_MOUSE_REPORT_ID),
        USB_HID_KEYBOARD_REPORT_DESCRIPTOR(USB_HID_KEYBOARD_REPORT_ID),
        USB_HID_JOYSTICK_REPORT_DESCRIPTOR(USB_HID_JOYSTICK_REPORT_ID)
};

static const uint16_t mouseSize = countIntegers<USB_HID_MOUSE_REPORT_DESCRIPTOR(0)>();
static const uint16_t keyboardSize = countIntegers<USB_HID_KEYBOARD_REPORT_DESCRIPTOR(0)>();
static const uint16_t joystickSize = countIntegers<USB_HID_JOYSTICK_REPORT_DESCRIPTOR(0)>();
static const uint16_t mouseOffset = 0;
static const uint16_t keyboardOffset = mouseOffset+mouseSize;
static const uint16_t joystickOffset = keyboardOffset+keyboardSize;

static_assert(mouseSize+keyboardSize+joystickSize == sizeof(reportDescriptors), "sizes match!");

// order matches enum in usb_hid_device.h
static const struct {
    const uint8_t* reportDescriptor;
    const uint16_t reportDescriptorSize;
} usbHIDDeviceReports[] = {
    { reportDescriptors+mouseOffset, mouseSize },
    { reportDescriptors+keyboardOffset, keyboardSize },
    { reportDescriptors+joystickOffset, joystickSize },
    { reportDescriptors+mouseOffset, mouseSize+keyboardSize },
    { reportDescriptors+keyboardOffset, keyboardSize+joystickSize },
    { reportDescriptors+mouseOffset, mouseSize+keyboardSize+joystickSize },
};

#define USB_TIMEOUT 50

HIDDevice::HIDDevice(void) {
}

static void generateUSBDescriptor(uint8* out, int maxLength, const char* in) {
    int length = strlen(in);
    if (length > maxLength)
        length = maxLength;
    int outLength = USB_DESCRIPTOR_STRING_LEN(length);
    
    out[0] = outLength;
    out[1] = USB_DESCRIPTOR_TYPE_STRING;
    
    for (int i=0; i<length; i++) {
        out[2+2*i] = in[i];
        out[3+2*i] = 0;
    }    
}

void HIDDevice::begin(const uint8_t* report_descriptor, uint16_t report_descriptor_length, uint16_t idVendor, uint16_t idProduct,
        const char* manufacturer, const char* product) {
            
    uint8_t* manufacturerDescriptor;
    uint8_t* productDescriptor;
	if(!enabled) {
        
#ifdef GENERIC_BOOTLOADER			
        //Reset the USB interface on generic boards - developed by Victor PV
        gpio_set_mode(PIN_MAP[PA12].gpio_device, PIN_MAP[PA12].gpio_bit, GPIO_OUTPUT_PP);
        gpio_write_bit(PIN_MAP[PA12].gpio_device, PIN_MAP[PA12].gpio_bit,0);
        
        for(volatile unsigned int i=0;i<512;i++);// Only small delay seems to be needed
        gpio_set_mode(PIN_MAP[PA12].gpio_device, PIN_MAP[PA12].gpio_bit, GPIO_INPUT_FLOATING);
#endif			

        if (manufacturer != NULL) {
            generateUSBDescriptor(iManufacturer, USB_HID_MAX_MANUFACTURER_LENGTH, manufacturer);
            manufacturerDescriptor = iManufacturer;
        }
        else {
            manufacturerDescriptor = NULL;
        }

        if (product != NULL) {
            generateUSBDescriptor(iProduct, USB_HID_MAX_PRODUCT_LENGTH, product);
        }
        else {
            productDescriptor = NULL;
        }

		usb_hid_enable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT, 
            report_descriptor, report_descriptor_length,
            idVendor, idProduct, manufacturerDescriptor, productDescriptor);
            
		enabled = true;
	}
}

void HIDDevice::begin(enum USBHIDDevice device, uint16_t idVendor, uint16_t idProduct,
        const char* manufacturer, const char* product) {
    begin(usbHIDDeviceReports[device].reportDescriptor, 
        usbHIDDeviceReports[device].reportDescriptorSize,
        idVendor, idProduct, manufacturer, product);
}

void HIDDevice::end(void){
	if(enabled){
	    usb_hid_disable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
		enabled = false;
	}
}


//================================================================================
//================================================================================
//	Mouse

HIDMouse::HIDMouse(uint8_t _reportID) : _buttons(0){
    reportID = _reportID;
}

void HIDMouse::begin(void){
}

void HIDMouse::end(void){
}

void HIDMouse::click(uint8_t b)
{
	_buttons = b;
	while (usb_hid_is_transmitting() != 0) {
    }
	move(0,0,0);
	_buttons = 0;
	while (usb_hid_is_transmitting() != 0) {
    }
	move(0,0,0);
}

void HIDMouse::move(signed char x, signed char y, signed char wheel)
{
	uint8_t m[4];
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	
	uint8_t buf[1+sizeof(m)];
	buf[0] = reportID;//report id
	uint8_t i;
	for(i=0;i<sizeof(m);i++){
		buf[i+1] = m[i];
	}
	
	usb_hid_tx(buf, sizeof(buf));
	
	while (usb_hid_is_transmitting() != 0) {
    }
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

void HIDMouse::buttons(uint8_t b)
{
	if (b != _buttons)
	{
		_buttons = b;
		while (usb_hid_is_transmitting() != 0) {
		}
		move(0,0,0);
	}
}

void HIDMouse::press(uint8_t b)
{
	while (usb_hid_is_transmitting() != 0) {
    }
	buttons(_buttons | b);
}

void HIDMouse::release(uint8_t b)
{
	while (usb_hid_is_transmitting() != 0) {
    }
	buttons(_buttons & ~b);
}

bool HIDMouse::isPressed(uint8_t b)
{
	if ((b & _buttons) > 0)
		return true;
	return false;
}



//================================================================================
//================================================================================
//	Keyboard

HIDKeyboard::HIDKeyboard(uint8_t _reportID) {
    reportID = _reportID;
}

void HIDKeyboard::sendReport(KeyReport* keys)
{
	//HID_SendReport(2,keys,sizeof(KeyReport));
	uint8_t buf[9];//sizeof(_keyReport)+1;
	buf[0] = reportID; 
	buf[1] = _keyReport.modifiers;
	buf[2] = _keyReport.reserved;
	
	uint8_t i;
	for(i=0;i<sizeof(_keyReport.keys);i++){
		buf[i+3] = _keyReport.keys[i];
	}
	usb_hid_tx(buf, sizeof(buf));
	
	while (usb_hid_is_transmitting() != 0) {
    }
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

void HIDKeyboard::begin(void){
}

void HIDKeyboard::end(void) {
}

size_t HIDKeyboard::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			//setWriteError();
			return 0;
		}
		if (k & SHIFT) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			//setWriteError();
			return 0;
		}
	}
	
	while (usb_hid_is_transmitting() != 0) {
    }
	
	sendReport(&_keyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t HIDKeyboard::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}
	
	while (usb_hid_is_transmitting() != 0) {
    }

	sendReport(&_keyReport);
	return 1;
}

void HIDKeyboard::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
	
	while (usb_hid_is_transmitting() != 0) {
    }
	
	sendReport(&_keyReport);
}

size_t HIDKeyboard::write(uint8_t c)
{
	uint8_t p = 0;

	p = press(c);	// Keydown
	release(c);		// Keyup
	
	return (p);		// Just return the result of press() since release() almost always returns 1
}

//================================================================================
//================================================================================
//	Joystick

void HIDJoystick::sendReport(void){
	usb_hid_tx(joystick_Report, sizeof(joystick_Report));
	
	while (usb_hid_is_transmitting() != 0) {
    }
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

HIDJoystick::HIDJoystick(uint8_t reportID) {
	joystick_Report[0] = reportID;
}

void HIDJoystick::begin(void){
}

void HIDJoystick::end(void){
}

void HIDJoystick::setManualReportMode(bool mode) {
    manualReport = mode;
}

void HIDJoystick::safeSendReport() {	
    if (!manualReport) {
        while (usb_hid_is_transmitting() != 0) {
        }
        sendReport();
    }
}

void HIDJoystick::sendManualReport() {
    while (usb_hid_is_transmitting() != 0) {
    }
    sendReport();
}
    
void HIDJoystick::button(uint8_t button, bool val){
	button--;
	uint8_t mask = (1 << (button & 7));
	if (val) {
		if (button < 8) joystick_Report[1] |= mask;
		else if (button < 16) joystick_Report[2] |= mask;
		else if (button < 24) joystick_Report[3] |= mask;
		else if (button < 32) joystick_Report[4] |= mask;
	} else {
		mask = ~mask;
		if (button < 8) joystick_Report[1] &= mask;
		else if (button < 16) joystick_Report[2] &= mask;
		else if (button < 24) joystick_Report[3] &= mask;
		else if (button < 32) joystick_Report[4] &= mask;
	}
	
    safeSendReport();
}

void HIDJoystick::X(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[5] = (joystick_Report[5] & 0x0F) | (val << 4);
	joystick_Report[6] = (joystick_Report[6] & 0xC0) | (val >> 4);
		
    safeSendReport();
}

void HIDJoystick::Y(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[6] = (joystick_Report[6] & 0x3F) | (val << 6);
	joystick_Report[7] = (val >> 2);
	
    safeSendReport();
}

void HIDJoystick::position(uint16_t x, uint16_t y){
	if (x > 1023) x = 1023;
	if (y > 1023) y = 1023;
	joystick_Report[5] = (joystick_Report[5] & 0x0F) | (x << 4);
	joystick_Report[6] = (x >> 4) | (y << 6);
	joystick_Report[7] = (y >> 2);
	
    safeSendReport();
}

void HIDJoystick::Xrotate(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[8] = val;
	joystick_Report[9] = (joystick_Report[9] & 0xFC) | (val >> 8);
	
    safeSendReport();
}

void HIDJoystick::Yrotate(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[9] = (joystick_Report[9] & 0x03) | (val << 2);
	joystick_Report[10] = (joystick_Report[10] & 0xF0) | (val >> 6);
	
    safeSendReport();
}

void HIDJoystick::sliderLeft(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[10] = (joystick_Report[10] & 0x0F) | (val << 4);
	joystick_Report[11] = (joystick_Report[11] & 0xC0) | (val >> 4);
	
    safeSendReport();
}

void HIDJoystick::sliderRight(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[11] = (joystick_Report[11] & 0x3F) | (val << 6);
	joystick_Report[12] = (val >> 2);
	
    safeSendReport();
}

void HIDJoystick::slider(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[10] = (joystick_Report[10] & 0x0F) | (val << 4);
	joystick_Report[11] = (val >> 4) | (val << 6);
	joystick_Report[12] = (val >> 2);
	
    safeSendReport();
}

void HIDJoystick::hat(int16_t dir){
	uint8_t val;
	if (dir < 0) val = 15;
	else if (dir < 23) val = 0;
	else if (dir < 68) val = 1;
	else if (dir < 113) val = 2;
	else if (dir < 158) val = 3;
	else if (dir < 203) val = 4;
	else if (dir < 245) val = 5;
	else if (dir < 293) val = 6;
	else if (dir < 338) val = 7;
	joystick_Report[5] = (joystick_Report[5] & 0xF0) | val;
	
    safeSendReport();
}


HIDDevice HID;
HIDMouse Mouse;
HIDKeyboard Keyboard;
HIDJoystick Joystick;
