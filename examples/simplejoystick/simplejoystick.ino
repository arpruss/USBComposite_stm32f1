#include <USBComposite.h>
#include <USBCompositeSerial.h>
#include <USBHID.h>

void setup() {
//  CompositeSerial.registerPart();
//  USBHID.setReportDescriptor(HID_JOYSTICK);
//  USBHID.registerPart();
//  USBComposite.begin();
  USBHID_begin_with_serial(HID_JOYSTICK);
}

void loop() {
  Joystick.X(0);
  delay(500);
  Joystick.X(1023);
  delay(500);
}

