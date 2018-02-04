#ifndef USBMASSSTORAGE_H
#define	USBMASSSTORAGE_H

#include <boards.h>
#include "USBComposite.h"
#include "usb_generic.h"
#include "usb_mass_mal.h"

class USBMassStorageDevice : public USBPlugin {
private:
  bool enabled = false;
public:
  void begin();
  void end();
  void loop();
  void clearDrives(void);
  bool registerParts();
  void setDrive(uint32 driveNumber, uint32 byteSize, MassStorageReader reader,
	MassStorageWriter writer = NULL, MassStorageStatuser = NULL, MassStorageInitializer = NULL);
  USBMassStorageDevice(USBCompositeDevice& device = USBComposite) : USBPlugin(device) {}
};

extern USBMassStorageDevice USBMassStorage;

#endif	/* USBMASSSTORAGE_H */

