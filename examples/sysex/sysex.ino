#include <USBComposite.h>

//Header(0xF0)and ending(0xF7) are excluded.

class myMidi : public USBMIDI {
 virtual void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity) {
  CompositeSerial.print("Note off: ch:");
  CompositeSerial.print(channel);
  CompositeSerial.print(" note:");
  CompositeSerial.print(note);
  CompositeSerial.print(" velocity:");
  CompositeSerial.println(velocity);
 }
 virtual void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity) {
  CompositeSerial.print("Note on: ch:");
  CompositeSerial.print(channel);
  CompositeSerial.print(" note:");
  CompositeSerial.print(note);
  CompositeSerial.print(" velocity:");
  CompositeSerial.println(velocity);
  }
  virtual void handleSysex(uint8_t *sysexBuffer, uint32 len)
  {
    CompositeSerial.print("Sysex - len:");
    CompositeSerial.print(len);
    CompositeSerial.print(" ");
    for(int i = 0; i < len; i++)
    {
      CompositeSerial.print(sysexBuffer[i]);
      CompositeSerial.print(" ");
    }
    CompositeSerial.println();
        USBMIDI::sendSysex(sysexBuffer, len);
     CompositeSerial.println("Sysex returned");
  }
};

myMidi midi;
USBCompositeSerial CompositeSerial;

void setup() {
    USBComposite.setProductId(0x0030);
    midi.registerComponent();
    CompositeSerial.registerComponent();
    USBComposite.begin();
}

void loop() {
  midi.poll();
}
