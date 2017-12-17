#include "USBHID.h"
#include "HIDReportDescriptorGenerator.h"

REPORT(KeyboardMouse, HID_MOUSE_REPORT_DESCRIPTOR(), HID_KEYBOARD_REPORT_DESCRIPTOR());
