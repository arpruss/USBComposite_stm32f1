#include <USBHID.h>

void setup() {
  USBHID.begin(HID_JOYSTICK);
}

void loop() {
  Joystick.X(0);
  delay(500);
  Joystick.X(1023);
  delay(500);
}

