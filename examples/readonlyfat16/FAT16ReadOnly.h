#ifndef _FAT16Readonly_H_
#define _FAT16Readonly_H_

#include <stdint.h>

#define FAT16_NUM_SECTORS          16384 // big enough to force FAT16
#define FAT16_MAX_ROOT_DIR_ENTRIES 224
#define FAT16_SECTOR_SIZE          512

typedef struct __attribute__ ((packed)) {
  char name[11]; // 0-10
  uint8_t attributes; // 11
  uint8_t reserved;   // 12
  uint8_t created[5]; // 13-18
  uint8_t accessed[2]; // 19-20
  uint16_t clusterHigh; // 21-22
  uint8_t  written[4];  
  uint16_t clusterLow;
  uint32_t size;
} FAT16RootDirEntry;

bool FAT16MemoryFileReader(uint8_t* out, const uint8_t* in, uint32_t inLength, uint32_t sector, uint32_t numSectors);
typedef bool (*FAT16FileReader)(uint8_t* buf, const char* name, uint32_t sector, uint32_t numSectors);
FAT16RootDirEntry* FAT16AddFile(const char* name, uint32_t size);
void FAT16SetRootDir(FAT16RootDirEntry* r, unsigned count, FAT16FileReader reader);
bool FAT16ReadSector(uint8_t *buf, uint32_t sector, uint16_t numSectors);

// TODO: names starting with E6

#endif
