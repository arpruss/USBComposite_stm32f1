#include <USBHID.h>

void setup() {
  USBHID_begin_with_serial(HID_KEYBOARD);
  Keyboard.begin(); // needed in case you want LED support
  delay(1000);
}

void loop() {
  Keyboard.println("hello");
  delay(5000);
}
