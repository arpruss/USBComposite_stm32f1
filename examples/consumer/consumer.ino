#include <USBHID.h>

const uint8_t reportDescription[] = {
   HID_CONSUMER_REPORT_DESCRIPTOR()
};

HIDConsumer Consumer;

void setup(){
  USBHID.begin(reportDescription, sizeof(reportDescription));
}

void loop() {
  Consumer.press(HIDConsumer::BRIGHTNESS_DOWN);
  Consumer.release();
  delay(500);
}

