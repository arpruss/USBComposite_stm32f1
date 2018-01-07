#include <USBHID.h>

void setup() {
  USBHID.begin(HID_KEYBOARD);
  delay(1000);
}

void loop() {
  Keyboard.println("hello");
  delay(5000);
}
