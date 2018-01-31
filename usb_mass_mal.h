#ifndef __USB_MASS_MAL_H
#define __USB_MASS_MAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  uint16_t usb_mass_mal_init(uint8_t lun);
  uint16_t usb_mass_mal_get_status(uint8_t lun);
  uint16_t usb_mass_mal_read_memory(uint8_t lun, uint32_t memoryOffset, uint8_t *readbuff, uint16_t transferLength);
  uint16_t usb_mass_mal_write_memory(uint8_t lun, uint32_t memoryOffset, uint8_t *writebuff, uint16_t transferLength);
  void usb_mass_mal_format();

#ifdef __cplusplus
}
#endif

#endif
