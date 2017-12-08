#include "usb_hid_device.h"

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



HIDMouse Mouse;
