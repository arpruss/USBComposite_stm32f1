#include "USBComposite.h" 

void HIDDesktop::begin(void) {}
void HIDDesktop::end(void) {}
void HIDDesktop::press(uint16_t button) {
    report.button = button;
    sendReport();
}

void HIDDesktop::release() {
    report.button = 0;
    sendReport();
}
