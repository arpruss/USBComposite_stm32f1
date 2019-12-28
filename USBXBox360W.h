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

#ifndef _USBXBox360W_H
#define _USBXBox360W_H

#include <Print.h>
#include <boards.h>
#include "USBComposite.h"
#include "usb_generic.h"
#include "usb_x360w.h"

typedef struct {
    uint8_t header[4];
    uint8_t reportID;
    uint8_t length;
    uint16_t buttons;
    uint8_t sliderLeft;
    uint8_t sliderRight;
    int16_t x;
    int16_t y;
    int16_t rx;
    int16_t ry;
    uint8 unused[11];
} __packed XBox360WReport_t;

static_assert(sizeof(XBox360WReport_t)==29, "Wrong endianness/packing!");

class USBXBox360WController {
private:
    XBox360WReport_t report = {{0},0,19,0,0,0,0,0,0,0,{0}};
    uint32 controller;
    bool manualReport = false;
	void safeSendReport(void);
    bool wait(void);
    void sendData(const void* data, uint32 length);
public:
    void setController(uint32 c) {
        controller = c;
    }
    void connect(bool state);
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
    void setLEDCallback(void (*callback)(uint8 pattern));
    void setRumbleCallback(void (*callback)(uint8 left, uint8 right));
    
    USBXBox360WController(uint32 _controller=0) {
        controller = _controller;
    }
};

template<const uint32 numControllers=4>class USBXBox360W {
private:
    bool enabled = false;
    uint8 buffers[USB_X360W_BUFFER_SIZE_PER_CONTROLLER * numControllers];
    
public:    
    static bool init(USBXBox360W<numControllers>* me) {
        x360w_initialize_controller_data(numControllers, me->buffers);
        USBComposite.setVendorId(0x045e);
        USBComposite.setProductId(0x0719);
        return true;
    };

    bool registerComponent() {
        return USBComposite.add(&usbX360WPart, this, (USBPartInitializer)&USBXBox360W<numControllers>::init);
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
    
    USBXBox360WController controllers[numControllers];

    USBXBox360W() {
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
#define XBOX_BACK 6
#define XBOX_L3 7
#define XBOX_R3 8
#define XBOX_LSHOULDER 9
#define XBOX_RSHOULDER 10
#define XBOX_GUIDE  11

#endif
