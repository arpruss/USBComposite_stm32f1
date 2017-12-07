#include <usb_serial.h>

class USBSerialNOP : public USBSerial {
public:
    USBSerialNOP(void) {}

    void begin(void) {}

    void begin(unsigned long) {}
    void begin(unsigned long, uint8_t) {}
    void end(void) {}

    virtual int available(void) {return 0;}

    size_t readBytes(char *buf, const size_t& len) {return 0;}
    uint32 read(uint8 * buf, uint32 len) {return 0;}
    int peek(void) {return -1;}
    int read(void) {return -1;}
    int availableForWrite(void) {return 0;}
    void flush(void) {return;}


    size_t write(uint8) {return 0;}
    size_t write(const char *str) {return 0;}
    size_t write(const uint8*, uint32) {return 0;}

    uint8 getRTS() {return 0;}
    uint8 getDTR() {return 0;}
    uint8 pending() {return 0;}

    /* SukkoPera: This is the Arduino way to check if an USB CDC serial
     * connection is open.
     * Used for instance in cardinfo.ino.
     */
    operator bool() {return 0;}

    /* Old libmaple way to check for serial connection.
     *
     * Deprecated, use the above.
     */
    uint8 isConnected() {return 0;}
};
