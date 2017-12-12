// This is a super silly project: you send a feature
// report from the PC, and it becomes a joystick setting.
// I guess it's not completely useless as it lets you
// get vJoy functionality without special vJoy drivers.

#include <USBHID.h>

class HIDJoystickRawData : public HIDJoystick {
  private:
    uint8_t featureData[sizeof(JoystickReport_t)];
    volatile HIDFeatureBuffer_t fb;
  public:
    HIDJoystickRawData(uint8_t reportID=USB_HID_JOYSTICK_REPORT_ID) : HIDJoystick(reportID) {
      fb.buffer = featureData;
      fb.bufferLength = sizeof(featureData);
      fb.reportID = reportID;
    }
    
    void begin() {
      HID.setFeatureBuffers(&fb, 1);
    }
    
    void setRawData(JoystickReport_t* p) {
      joyReport = *p;
      send();
    }
};

HIDJoystickRawData joy;

const uint8_t reportDescription[] = {
   USB_HID_JOYSTICK_REPORT_DESCRIPTOR(USB_HID_JOYSTICK_REPORT_ID, 
        USB_HID_FEATURE_REPORT_DESCRIPTOR(sizeof(JoystickReport_t)-1))
};

void setup() {
  HID.begin(reportDescription, sizeof(reportDescription));
  joy.begin();
}

void loop() {
  JoystickReport_t report;
  if (joy.getFeature((uint8_t*)&report)) {
    joy.setRawData(&report);
  }
  delay(5);
}

