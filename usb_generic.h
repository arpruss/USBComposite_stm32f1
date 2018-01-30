#ifndef _USB_GENERIC_H
#define _USB_GENERIC_H
#include <libmaple/libmaple_types.h>
//#include <usb_type.h>
//#include <usb_core.h>
#include <usb_lib_globals.h>
#include <usb.h>

#define PMA_MEMORY_SIZE 512
#define MAX_USB_DESCRIPTOR_DATA_SIZE 200

#define USB_EP0_BUFFER_SIZE       0x40
#define USB_EP0_TX_BUFFER_ADDRESS 0x40
#define USB_EP0_RX_BUFFER_ADDRESS (USB_EP0_TX_BUFFER_ADDRESS+USB_EP0_BUFFER_SIZE) 

#ifdef __cplusplus
extern "C" {
#endif

extern usb_descriptor_string usb_generic_default_iManufacturer;
extern usb_descriptor_string usb_generic_default_iProduct;

typedef struct USBEndpointInfo {
    void (*callback)(void);
    uint16 bufferSize;
    uint16 type; // bulk, interrupt, etc.
    uint8 tx; // 1 if TX, 0 if RX
    uint8 address;    
    uint16 pmaAddress;
} USBEndpointInfo;

typedef struct USBCompositePart {
    uint8 numInterfaces;
    uint8 numEndpoints;
    uint8 startInterface;
    uint8 startEndpoint;
    uint16 descriptorSize;
    void (*getPartDescriptor)(const struct USBCompositePart* part, uint8* out);
    void (*usbInit)(const struct USBCompositePart* part);
    void (*usbReset)(const struct USBCompositePart* part);
    RESULT (*usbDataSetup)(const struct USBCompositePart* part, uint8 request);
    RESULT (*usbNoDataSetup)(const struct USBCompositePart* part, uint8 request);
    USBEndpointInfo* endpoints;
} USBCompositePart;

void usb_generic_set_info(uint16 idVendor, uint16 idProduct, const uint8* iManufacturer, const uint8* iProduct, const uint8* iSerialNumber);
uint8 usb_generic_set_parts(USBCompositePart** _parts, unsigned _numParts);
void usb_generic_disable(void);
void usb_generic_enable(void);
extern volatile int8 usbGenericTransmitting;

#ifdef __cplusplus
}
#endif

#endif
