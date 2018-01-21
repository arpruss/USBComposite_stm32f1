#include <USBHID.h>

void setup() {
  USBHID.begin(HID_KEYBOARD);
  Keyboard.begin(); // needed in case you want LED support
  delay(1000);
}

void loop() {
  Keyboard.println("hello");
  delay(5000);
}
