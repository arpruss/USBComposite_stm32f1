#include "MassStorage.h"
#include "usb_mass.h"
#include "usb_mass_mal.h"
#include "usb_serial.h"
#include <string.h>

void USBMassStorageDevice::begin() {
	if(!enabled) {
		device->clear();
		device->add(this);
		device->begin();

		enabled = true;
	}
}

void USBMassStorageDevice::end() {
	device->end();
}

void USBMassStorageDevice::loop() {
	usb_mass_loop();
}

bool USBMassStorageDevice::registerParts() {
	return device->add(usbMassPart);
}

void USBMassStorageDevice::setDrive(uint32 driveNumber, uint32 blockCount, uint32 blockSize, MassStorageReader reader,
	MassStorageWriter writer, MassStorageStatuser statuser, MassStorageInitializer initializer) {
	if (driveNumber >= USB_MASS_MAX_DRIVES)
		return;
	usb_mass_drives[driveNumber].blockCount = blockCount;
	usb_mass_drives[driveNumber].blockSize = blockSize;
	usb_mass_drives[driveNumber].read = reader;
	usb_mass_drives[driveNumber].write = writer;
	usb_mass_drives[driveNumber].status = statuser;
	usb_mass_drives[driveNumber].init = initializer;
	usb_mass_drives[driveNumber].format = initializer;
}

void USBMassStorageDevice::setDrive(uint32 driveNumber, uint32 byteSize, MassStorageReader reader,
	MassStorageWriter writer, MassStorageStatuser statuser, MassStorageInitializer initializer) {
	setDrive(driveNumber, byteSize / 512, 512, reader, writer, statuser, initializer);
}

void USBMassStorageDevice::clearDrives() {
	memset(usb_mass_drives, 0, sizeof(usb_mass_drives));
}

USBMassStorageDevice USBMassStorage;
