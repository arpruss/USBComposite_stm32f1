#include <usbMassStorage.h>
#include <usb_mass_mal.h>
#include <USBHID.h>

//#define READ_ONLY

#ifdef READ_ONLY
#include "image.h"
#define buffer image
#else
#define DISK_SIZE 12000 // sizeof(image) // 5120 // sizeof(image)
uint8 buffer[DISK_SIZE];
#endif

uint32_t MAL_massBlockCount[2];
uint32_t MAL_massBlockSize[2];

void setup() {
  pinMode(PB12,OUTPUT);
  digitalWrite(PB12,1);
  MAL_massBlockCount[0] = DISK_SIZE / 512;
  MAL_massBlockCount[1] = 0;
  MAL_massBlockSize[0] = 512;
  MAL_massBlockSize[1] = 0;
  USBMassStorage.begin();
  delay(2000);
 // CompositeSerial.println("hello");
}

extern "C" uint16_t usb_mass_mal_write_memory(uint8_t lun, uint32_t memoryOffset, uint8_t *writebuff, uint16_t transferLength) {
  if (lun != 0) {
    return USB_MASS_MAL_FAIL;
  }
#ifndef READ_ONLY  
  memcpy(buffer+memoryOffset, writebuff, transferLength);
#endif  
  CompositeSerial.println("write");
  return USB_MASS_MAL_SUCCESS;
}

extern "C" uint16_t usb_mass_mal_init(uint8_t lun) {
  return USB_MASS_MAL_SUCCESS;
}

extern "C" uint16_t usb_mass_mal_get_status(uint8_t lun) {  
  return USB_MASS_MAL_SUCCESS;
}

extern "C" uint16_t usb_mass_mal_read_memory(uint8_t lun, uint32_t memoryOffset, uint8_t *readbuff, uint16_t transferLength) {
  if (lun != 0) {
    return USB_MASS_MAL_FAIL;
  }
  CompositeSerial.println(String(memoryOffset)+" "+String(transferLength));
  memcpy(readbuff, buffer+memoryOffset, transferLength);
  return USB_MASS_MAL_SUCCESS;
}

extern "C" void usb_mass_mal_format() {
}

int state = 0;
void loop() {
  digitalWrite(PB12,state);
//  delay(20);
  state = !state;
  USBMassStorage.loop();
}

