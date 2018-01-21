#include <USBHID.h>
#include <libmaple/usb.h>

#define TXSIZE 256
#define RXSIZE 300

HIDRaw<TXSIZE,RXSIZE> raw;
uint8 buf[RXSIZE];

extern "C" {
  extern uint32 info;
};

const uint8_t reportDescription[] = {
   HID_RAW_REPORT_DESCRIPTOR(TXSIZE,RXSIZE)
};

void setup(){
  USBHID.begin(reportDescription, sizeof(reportDescription));  
  raw.begin();
}

void loop() {
  if (raw.getOutput(buf)) {
    for (int i=0;i<RXSIZE;i++) buf[i]++;
    raw.send(buf+RXSIZE-min(RXSIZE,TXSIZE),min(RXSIZE,TXSIZE));
  }
}

