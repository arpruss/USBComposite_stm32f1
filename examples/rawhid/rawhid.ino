#include <USBHID.h>
#include <libmaple/usb.h>

#define TXSIZE 64
#define RXSIZE 32

HIDRaw<TXSIZE,RXSIZE> raw;
volatile uint8 buf[RXSIZE];
uint8 buf2[RXSIZE];
volatile HIDBuffer_t fb {buf, RXSIZE, 0};

const uint8_t reportDescription[] = {
   USB_HID_RAW_REPORT_DESCRIPTOR(TXSIZE,RXSIZE)
};

void setup(){
  pinMode(PB12,OUTPUT);
  digitalWrite(PB12,1);
  HID.begin(reportDescription, sizeof(reportDescription));  
  HID.setBuffers(HID_REPORT_TYPE_OUTPUT, &fb,1);
  delay(1000);
}

void loop() {
  if (raw.getOutput(buf2)) {
    for (int i=0;i<RXSIZE;i++) buf2[i]++;
    raw.send(buf2,RXSIZE);
  }
}


