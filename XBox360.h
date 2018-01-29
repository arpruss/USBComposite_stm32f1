/*
  Copyright (c) 2012 Arduino.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @brief Wirish USB HID port (HID USB).
 */

#ifndef _XBOX360_H
#define _XBOX360_H

#include <Print.h>
#include <boards.h>
#include "usb_generic.h"

class HIDXBox360{
private:
    USBCompositePart* parts[4];
    unsigned numParts;
	uint8_t xbox360_Report[20] = {0,0x14};//    3,0,0,0,0,0x0F,0x20,0x80,0x00,0x02,0x08,0x20,0x80};
    bool manualReport = false;
    bool enabled;
	void safeSendReport(void);
	void sendReport(void);
public:
	void send(void);
    void setManualReportMode(bool manualReport);
    bool getManualReportMode();
	HIDXBox360(void);
	void begin(void);
	void end(void);
	void button(uint8_t button, bool val);
	void X(int16_t val);
	void Y(int16_t val);
	void position(int16_t x, int16_t y);
	void positionRight(int16_t x, int16_t y);
	void XRight(int16_t val);
	void YRight(int16_t val);
	void sliderLeft(uint8_t val);
	void sliderRight(uint8_t val);
	void hat(int16_t dir);
    void setLEDCallback(void (*callback)(uint8 pattern));
    void setRumbleCallback(void (*callback)(uint8 left, uint8 right));
};

extern HIDXBox360 XBox360;

#endif

