#include "USBHID.h"
#include "HIDReportDescriptorGenerator.h"

REPORT(KeyboardJoystick, HID_KEYBOARD_REPORT_DESCRIPTOR(), HID_JOYSTICK_REPORT_DESCRIPTOR());
