
#ifndef USBMASSSTORAGE_H
#define	USBMASSSTORAGE_H

#include <boards.h>
#include "usb_generic.h"

#define USB_MASS_MAL_FAIL    1
#define USB_MASS_MAL_SUCCESS 0

class USBMassStorageDriver {
private:
  USBCompositePart* parts[4];
  uint16 numParts = 0;
  bool enabled = false;
public:
  void begin();
  void end();
  void loop();
};

extern USBMassStorageDriver USBMassStorage;

#endif	/* USBMASSSTORAGE_H */

