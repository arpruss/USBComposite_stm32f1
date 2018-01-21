#include <USBHID.h>


void setup() {
  USBHID.begin(HID_KEYBOARD);
  Keyboard.begin(); // needed for LED support
  delay(1000);
}

void loop() {
  CompositeSerial.println(Keyboard.getLEDs()); 
}


