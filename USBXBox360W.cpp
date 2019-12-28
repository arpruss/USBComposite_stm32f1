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

#include <Arduino.h>
#include "USBComposite.h" 

bool USBXBox360WController::wait() {
    uint32_t t=millis();
	while (x360w_is_transmitting(controller) != 0 && (millis()-t)<500) ;
    return ! x360w_is_transmitting(controller);
}

void USBXBox360WController::sendData(void* data, uint32 length){
    if (wait()) {
        x360w_tx(controller, (uint8*)data, length);

        if(wait()) {
        /* flush out to avoid having the pc wait for more data */
            x360w_tx(controller, NULL, 0);
        }
    }
}

void USBXBox360WController::send(void){
    report.header[0] = 0x00;
    report.header[1] = 0x01;
    report.header[2] = 0x00;
    report.header[3] = 0xf0;
    sendData(&report, sizeof(report));
}

void USBXBox360WController::connect(bool state) {
    report.header[0] = 0x08;
    report.header[1] = state ? 0x80 : 0x00;
    sendData(&report.header, 2);
}

void USBXBox360WController::setRumbleCallback(void (*callback)(uint8 left, uint8 right)) {
    x360w_set_rumble_callback(controller,callback);
}

void USBXBox360WController::setLEDCallback(void (*callback)(uint8 pattern)) {
    x360w_set_led_callback(controller, callback);
}

void USBXBox360WController::stop(void){
	setRumbleCallback(NULL);
	setLEDCallback(NULL);
}

void USBXBox360WController::setManualReportMode(bool mode) {
    manualReport = mode;
}

bool USBXBox360WController::getManualReportMode() {
    return manualReport;
}

void USBXBox360WController::safeSendReport() {	
    if (!manualReport) 
        send();
}

void USBXBox360WController::button(uint8_t button, bool val){
	uint16_t mask = (1 << (button-1));
	if (val) {
        report.buttons |= mask;
	} else {
		report.buttons &= ~mask;
	}
	
    safeSendReport();
}

void USBXBox360WController::buttons(uint16_t buttons){
    report.buttons = buttons;
	
    safeSendReport();
}

void USBXBox360WController::X(int16_t val){
    report.x = val;
		
    safeSendReport();
}

void USBXBox360WController::Y(int16_t val){
    report.y = val;
		
    safeSendReport();
}

void USBXBox360WController::XRight(int16_t val){
    report.rx = val;
		
    safeSendReport();
}

void USBXBox360WController::YRight(int16_t val){
    report.ry = val;
		
    safeSendReport();
}

void USBXBox360WController::position(int16_t x, int16_t y){
    report.x = x;
    report.y = y;
		
    safeSendReport();
}

void USBXBox360WController::positionRight(int16_t x, int16_t y){
    report.rx = x;
    report.ry = y;
		
    safeSendReport();
}

void USBXBox360WController::sliderLeft(uint8_t val){
    report.sliderLeft = val;
	
    safeSendReport();
}

void USBXBox360WController::sliderRight(uint8_t val){
    report.sliderRight = val;
	
    safeSendReport();
}

