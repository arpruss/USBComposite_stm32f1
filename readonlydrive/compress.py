import sys

with open(sys.argv[1],"rb") as f:
    data = f.read()
    sectors = (len(data) +  511) // 512
    data += b'\0' * (sectors * 512 - len(data))
    print("#include <stdint.h>")
    print("#include <string.h>")
    print("#define IMAGE_EMPTY_SECTOR 0xFFFF")
    print("static const uint32_t image_sectors = %d;" % sectors)
    stream = b''
    fullCount = 0
    sectorOffsets = []
    for i in range(sectors):
        emptySector = True
        for j in range(512):
            if data[i*512+j]:
                emptySector = False
                break
        if emptySector:
            sectorOffsets.append( " /* %04X */ IMAGE_EMPTY_SECTOR," % i )
        else:
            fullCount = i + 1
            if len(stream)>=0xFFFF:
                raise Exception("stream too big")
            sectorOffsets.append( " /* %04X */ %d," % (i,len(stream)) )
            stream += data[i*512:i*512+512]
    print("#define IMAGE_SECTOR_DATA_COUNT %d" % fullCount)
    print("static const uint16_t image_sector_offsets[IMAGE_SECTOR_DATA_COUNT] = {")
    for i in range(fullCount):
        print(sectorOffsets[i])
    print("};");
    print("static const uint8_t image_stream[%d] = {" % len(stream), end='');
    for i in range(len(stream)):
        if i % 16 == 0:
            print("\n ",end='');
        print("0x%02x," % stream[i],end='')
    print("\n};\n");

print("""uint8_t image_fetch_byte(uint32_t offset) {
    unsigned sector = offset / 512;
    if (sector >= IMAGE_SECTOR_DATA_COUNT || image_sector_offsets[sector] == IMAGE_EMPTY_SECTOR) 
        return 0;
    return image_stream[image_sector_offsets[sector] + offset % 512];
}

void image_fetch_sector(uint16_t sector, void* data) {
    if (sector >= IMAGE_SECTOR_DATA_COUNT || image_sector_offsets[sector] == IMAGE_EMPTY_SECTOR) {
        memset(data, 0, 512);
    }
    else {
        memcpy(data, image_stream + image_sector_offsets[sector], 512);
    }
}

// main() { for(int i=0;i<image_sectors*512;i++) putchar(image_fetch_byte(i)); }
""");
