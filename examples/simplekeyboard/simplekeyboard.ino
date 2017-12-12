#include <USBHID.h>

void setup() {
  USB.begin(USB_HID_KEYBOARD);
  delay(1000);
}

void loop() {
  Keyboard.println("hello");
  delay(5000);
}
