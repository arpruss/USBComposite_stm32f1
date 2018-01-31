#include "usbMassStorage.h"
#include "usb_mass.h"

void USBMassStorageDriver::begin() {
	if(!enabled){
        parts[0] = &usbMassPart;
        numParts = 1;
        
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
