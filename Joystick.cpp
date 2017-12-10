#include "usb_hid_device.h"

//================================================================================
//================================================================================
//	Joystick

void HIDJoystick::begin(void){
}

void HIDJoystick::end(void){
}

void HIDJoystick::setManualReportMode(bool mode) {
    manualReport = mode;
}

void HIDJoystick::safeSendReport() {	
    if (!manualReport) {
        sendReport();
    }
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
    else val = 15;
	joystick_Report[5] = (joystick_Report[5] & 0xF0) | val;
	
    safeSendReport();
}

HIDJoystick Joystick;
