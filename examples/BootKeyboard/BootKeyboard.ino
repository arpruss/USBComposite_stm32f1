#include <USBHID.h>

void setup() 
{
    USBHID.begin(HID_BOOT_KEYBOARD);
}

void loop() 
{
  BootKeyboard.press(KEY_F12);
  delay(100);
  BootKeyboard.release(KEY_F12);
  delay(1000);
}
