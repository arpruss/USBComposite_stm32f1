#ifndef _COMPOSITE_SERIAL_H_
#define _COMPOSITE_SERIAL_H_

#include "USBComposite.h"
#include "usb_serial.h"

class USBCompositeSerial : public USBPlugin, public Stream {
private:
	bool enabled = false;
public:
	USBCompositeSerial(USBCompositeDevice& device = USBComposite) : USBPlugin(device) {}

	void begin();
	void end();
	bool init();
	bool registerParts();

	operator bool() { return true; } // Roger Clark. This is needed because in cardinfo.ino it does if (!Serial) . It seems to be a work around for the Leonardo that we needed to implement just to be compliant with the API

    virtual int available(void);// Changed to virtual

    uint32 read(uint8 * buf, uint32 len);
   // uint8  read(void);

	// Roger Clark. added functions to support Arduino 1.0 API
    virtual int peek(void);
    virtual int read(void);
    int availableForWrite(void);
    virtual void flush(void);	
	
    size_t write(uint8);
    size_t write(const char *str);
    size_t write(const uint8*, uint32);

    uint8 getRTS();
    uint8 getDTR();
    uint8 isConnected();
    uint8 pending();
};

extern USBCompositeSerial CompositeSerial;
#endif
