#include <USBComposite.h>
#include "FAT16ReadOnly.h"

USBMassStorage MassStorage;
USBCompositeSerial CompositeSerial;

#define PRODUCT_ID 0x29

FAT16RootDirEntry rootDir[3];

const char readme_txt[] = "The FLASH.BIN file contains the 128kb of the stm32f103c8t6's flash and the INFO.TXT has some information on the chip.\n";
char info[FAT16_SECTOR_SIZE];

bool write(const uint8_t *writebuff, uint32_t memoryOffset, uint16_t transferLength) {
  return false;
}

bool fileReader(uint8_t *buf, const char* name, uint32_t sector, uint32_t sectorCount) {
  if (!strcmp(name,"README.TXT")) {
    return FAT16MemoryFileReader(buf, (uint8_t*)readme_txt, sizeof(readme_txt)-1, sector, sectorCount);
  }
  else if (!strcmp(name,"FLASH.BIN")) {
    return FAT16MemoryFileReader(buf, (uint8_t*)0x08000000, 128*1024, sector, sectorCount);
  }
  else if (!strcmp(name,"INFO.TXT")) {
    strcpy((char*)buf, info); 
    return true;
  }
  return false;
}

uint16_t setInfo(char* info) {
  uint16 *flashSize = (uint16 *) (0x1FFFF7E0);
  uint16 *idBase0 =  (uint16 *) (0x1FFFF7E8);
  uint16 *idBase1 =  (uint16 *) (0x1FFFF7E8+0x02);
  uint32 *idBase2 =  (uint32 *) (0x1FFFF7E8+0x04);
  uint32 *idBase3 =  (uint32 *) (0x1FFFF7E8+0x08);

  sprintf(info, "Flash size is %uk.\nUnique ID is %02x%02x%02x%02x\n", 
    *flashSize, *(idBase0),*(idBase1), *(idBase2), *(idBase3));

  return strlen(info);
}
    
void setup() {
  FAT16SetRootDir(rootDir, sizeof(rootDir)/sizeof(*rootDir), fileReader);
  FAT16AddFile("README.TXT", sizeof(readme_txt)-1);
  FAT16AddFile("INFO.TXT", setInfo(info));
  FAT16AddFile("FLASH.BIN", 128*1024);

  USBComposite.setProductId(PRODUCT_ID);
  MassStorage.setDriveData(0, FAT16_NUM_SECTORS, FAT16ReadSector, write);
  MassStorage.registerComponent();

  USBComposite.begin();
  delay(2000);
  while(!USBComposite);
}

void loop() {
  MassStorage.loop();
}

