#include <USBComposite.h>

USBMassStorage MassStorage;
USBCompositeSerial CompositeSerial;

#define PRODUCT_ID 0x29

#include "demoimage.h"

bool write(const uint8_t *writebuff, uint32_t memoryOffset, uint16_t transferLength) {
  //memcpy(image+SCSI_BLOCK_SIZE*memoryOffset, writebuff, SCSI_BLOCK_SIZE*transferLength);
  return false;
}

bool read(uint8_t *readbuff, uint32_t memoryOffset, uint16_t transferLength) {
  for (int i=0; i<transferLength; i++)
    image_fetch_sector(memoryOffset+i, readbuff+SCSI_BLOCK_SIZE*i);
  
  return true;
}


void setup() {
  USBComposite.setProductId(PRODUCT_ID);
  MassStorage.setDriveData(0, image_sectors, read, write);
  MassStorage.registerComponent();
  USBComposite.begin();
  delay(2000);
}

void loop() {
  MassStorage.loop();
}
