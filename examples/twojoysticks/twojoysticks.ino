#include <USBHID.h>

const uint8_t reportDescription[] = {
   USB_HID_MOUSE_REPORT_DESCRIPTOR(USB_HID_MOUSE_REPORT_ID),
   USB_HID_KEYBOARD_REPORT_DESCRIPTOR(USB_HID_KEYBOARD_REPORT_ID),
   USB_HID_JOYSTICK_REPORT_DESCRIPTOR(USB_HID_JOYSTICK_REPORT_ID),
   USB_HID_JOYSTICK_REPORT_DESCRIPTOR(USB_HID_JOYSTICK_REPORT_ID+1),
};

HIDJoystick Joystick2(USB_HID_JOYSTICK_REPORT_ID+1);

void setup(){
  HID.begin(reportDescription, sizeof(reportDescription));
  Joystick.setManualReportMode(true);
  Joystick2.setManualReportMode(true);
}

void loop(){
  Joystick.X(0);
  Joystick.Y(0);
  Joystick.sliderRight(1023);
  Joystick.sendManualReport();
  delay(400);
  Joystick.X(1023);
  Joystick.Y(1023);
  Joystick.sliderRight(0);
  Joystick.sendManualReport();
  delay(400);
  Joystick2.X(0);
  Joystick2.Y(0);
  Joystick2.sliderRight(1023);
  Joystick2.sendManualReport();
  delay(400);
  Joystick2.X(1023);
  Joystick2.Y(1023);
  Joystick2.sliderRight(0);
  Joystick2.sendManualReport();
  delay(400);
}

