#ifndef _MULTI_SERIAL_H_
#define _MULTI_SERIAL_H_

#include "usb_multi_serial.h"

class USBMultiSerial {
private:
	bool enabled = false;
    uint8 numPorts = 3; //USB_MULTI_SERIAL_MAX_PORTS;
public:
	bool begin();
	void end();
	bool registerComponent();
    void setNumberOfPorts(uint8 portCount) {
        numPorts = portCount;
    }

	static bool init(USBMultiSerial* me);

	operator bool() { return true; } // Roger Clark. This is needed because in cardinfo.ino it does if (!Serial) . It seems to be a work around for the Leonardo that we needed to implement just to be compliant with the API

    class USBSerialPort : public Stream {
    public:
        uint32 txPacketSize = 44;
        uint32 rxPacketSize = 44;
        uint8 port;
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
        
        void setRXPacketSize(uint32 size=64) {
            rxPacketSize = size;
        }

        void setTXPacketSize(uint32 size=64) {
            txPacketSize = size;
        }
        
        void setPort(uint8 _port) {
            port = _port;
        }
        
        uint8 getPort(void) {
            return port;
        }
        
    };
    
    USBSerialPort ports[3];

    USBMultiSerial() {
        for (uint8 i=0;i<3;i++) ports[i].setPort(i);
    }
};

extern USBCompositeSerial CompositeSerial;
#endif
