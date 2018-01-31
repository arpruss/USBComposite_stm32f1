#include "usbMassStorage.h"
#include "usb_mass.h"
#include "usb_serial.h"

void USBMassStorageDriver::begin() {
	if(!enabled){
        parts[1] = &usbSerialPart;
        parts[0] = &usbMassPart;
        numParts = 2;
        
        usb_generic_set_parts(parts, numParts);
        usb_generic_enable();

		enabled = true;
	}
}

void USBMassStorageDriver::end() {
}

void USBMassStorageDriver::loop() {
  usb_mass_loop();
}

USBMassStorageDriver USBMassStorage;
