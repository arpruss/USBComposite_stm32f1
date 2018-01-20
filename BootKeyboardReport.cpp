#include "USBHID.h"
#include "HIDReportDescriptorGenerator.h"

REPORT(BootKeyboard, HID_BOOT_KEYBOARD_REPORT_DESCRIPTOR(HID_KEYBOARD_LEDS_REPORT_DESCRIPTOR()));
