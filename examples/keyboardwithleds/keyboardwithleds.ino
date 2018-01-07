#include <USBHID.h>

const uint8_t reportDescription[] = {
  HID_KEYBOARD_REPORT_DESCRIPTOR(HID_KEYBOARD_REPORT_ID, HID_KEYBOARD_LEDS_REPORT_DESCRIPTOR())
};

uint8 leds[HID_BUFFER_ALLOCATE_SIZE(1,1)] = {0,0}; // leds[1] is the current LED status

HIDBuffer_t ledData = {
  leds,
  HID_BUFFER_SIZE(1,1),
  HID_KEYBOARD_REPORT_ID,
  HID_BUFFER_MODE_NO_WAIT
};

void setup() {
  USBHID.begin(reportDescription, sizeof(reportDescription));
  USBHID.setOutputBuffers(&ledData, 1);
  delay(1000);
}

void loop() {
  CompositeSerial.println((unsigned)leds[1]); 
}


