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

#ifndef _USBMultiXBox360_H
#define _USBMultiXBox360_H

#include <Print.h>
#include <boards.h>
#include "USBComposite.h"
#include "usb_generic.h"
#include "usb_multi_x360.h"

class USBXBox360Controller {
private:
	uint8_t xbox360_Report[20] = {0,0x14};//    3,0,0,0,0,0x0F,0x20,0x80,0x00,0x02,0x08,0x20,0x80};
    uint8 controller;
    bool manualReport = false;
	void safeSendReport(void);
	void sendReport(void);
    bool wait(void);
public:
    void setController(uint8 c) {
        controller = c;
    }
	void send(void);
    void setManualReportMode(bool manualReport);
    bool getManualReportMode();
	void stop(void);
	void button(uint8_t button, bool val);
    void buttons(uint16_t b);
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

template<const uint8 numControllers=1>class USBMultiXBox360 {
private:
    bool enabled;
    uint8 buffers[USB_X360_BUFFER_SIZE_PER_CONTROLLER * numControllers];
    
public:    
    static bool init(USBMultiXBox360<numControllers>* me) {
        usb_multi_x360_initialize_controller_data(numControllers, me->buffers);
        USBComposite.setVendorId(0x045e);
        USBComposite.setProductId(0x028e);
        return true;
    };

    bool registerComponent() {
        return USBComposite.add(&usbMultiX360Part, this, (USBPartInitializer)&USBMultiXBox360<numControllers>::init);
    };

    void begin(void){
        if(!enabled){
            USBComposite.clear();
            registerComponent();
            USBComposite.begin();

            enabled = true;
        }
    };

    void end() {
        if (enabled) {
            enabled = false;
            USBComposite.end();
        }
    };
    
    
    USBXBox360Controller controllers[numControllers];

    USBMultiXBox360() {
        for (uint8 i=0;i<numControllers;i++) controllers[i].setController(i);
    }
};

#define XBOX_A 13
#define XBOX_B 14
#define XBOX_X 15
#define XBOX_Y 16
#define XBOX_DUP 1
#define XBOX_DDOWN 2
#define XBOX_DLEFT 3
#define XBOX_DRIGHT 4
#define XBOX_START 5
#define XBOX_LSHOULDER 9
#define XBOX_RSHOULDER 10
#define XBOX_GUIDE  11

#endif
