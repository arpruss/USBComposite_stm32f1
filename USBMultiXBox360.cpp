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

#include <Arduino.h>
#include <string.h>
#include <stdint.h>
#include <libmaple/nvic.h>
#include <libmaple/usb.h>
#include "USBComposite.h" 
#include "usb_multi_x360.h"

bool USBXBox360Controller::wait() {
    uint32_t t=millis();
	while (usb_multi_x360_is_transmitting(controller) != 0 && (millis()-t)<500) ;
    return ! usb_multi_x360_is_transmitting(controller);
}

void USBXBox360Controller::sendReport(void){
	usb_multi_x360_tx(controller, (uint8*)&report, sizeof(report));

    if(wait()) {
	/* flush out to avoid having the pc wait for more data */
        usb_multi_x360_tx(controller, NULL, 0);
    }
}

void USBXBox360Controller::setRumbleCallback(void (*callback)(uint8 left, uint8 right)) {
    usb_multi_x360_set_rumble_callback(controller,callback);
}

void USBXBox360Controller::setLEDCallback(void (*callback)(uint8 pattern)) {
    usb_multi_x360_set_led_callback(controller, callback);
}

void USBXBox360Controller::stop(void){
	setRumbleCallback(NULL);
	setLEDCallback(NULL);
}

void USBXBox360Controller::setManualReportMode(bool mode) {
    manualReport = mode;
}

bool USBXBox360Controller::getManualReportMode() {
    return manualReport;
}

void USBXBox360Controller::safeSendReport() {	
    if (!manualReport) {
        if (wait())
            sendReport();
    }
}

void USBXBox360Controller::send() {
    if (wait())
        sendReport();
}
    
void USBXBox360Controller::button(uint8_t button, bool val){
	button--;
	uint16_t mask = (1 << button);
	if (val) {
        report.buttons |= mask;
	} else {
		report.buttons &= ~mask;
	}
	
    safeSendReport();
}

void USBXBox360Controller::buttons(uint16_t buttons){
    report.buttons = buttons;
	
    safeSendReport();
}

void USBXBox360Controller::X(int16_t val){
    report.x = val;
		
    safeSendReport();
}

void USBXBox360Controller::Y(int16_t val){
    report.y = val;
		
    safeSendReport();
}

void USBXBox360Controller::XRight(int16_t val){
    report.rx = val;
		
    safeSendReport();
}

void USBXBox360Controller::YRight(int16_t val){
    report.ry = val;
		
    safeSendReport();
}

void USBXBox360Controller::position(int16_t x, int16_t y){
    report.x = x;
    report.y = y;
		
    safeSendReport();
}

void USBXBox360Controller::positionRight(int16_t x, int16_t y){
    report.rx = x;
    report.ry = y;
		
    safeSendReport();
}

void USBXBox360Controller::sliderLeft(uint8_t val){
    report.sliderLeft = val;
	
    safeSendReport();
}

void USBXBox360Controller::sliderRight(uint8_t val){
    report.sliderRight = val;
	
    safeSendReport();
}

