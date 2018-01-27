#ifndef _USB_GENERIC_h
#define _USB_GENERIC_H
#include <libmaple/libmaple_types.h>
#include <libmaple/usb.h>
#include "usb_lib_globals.h"
#include "usb_reg_map.h"
#include <usb_core.h>

#define PMA_MEMORY_SIZE 512
#define MAX_USB_DESCRIPTOR_DATA_SIZE 256

#define USB_EP0_BUFFER_SIZE       0x40
#define USB_EP0_TX_BUFFER_ADDRESS 0x40
#define USB_EP0_RX_BUFFER_ADDRESS (USB_EP0_TX_BUFFER_ADDRESS+USB_EP0_BUFFER_SIZE) 

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
    void (*getPartDescriptor)(struct USBCompositePart* part, uint8* out);
    void (*usbInit)(struct USBCompositePart* part);
    void (*usbReset)(struct USBCompositePart* part);
    RESULT (*usbDataSetup)(struct USBCompositePart* part, uint8 request);
    RESULT (*usbNoDataSetup)(struct USBCompositePart* part, uint8 request);
    USBEndpointInfo* endpoints;
} USBCompositePart;

void usb_generic_set_info( uint16 idVendor, uint16 idProduct, const uint8* iManufacturer, const uint8* iProduct, const uint8* iSerialNumber);
uint8 usb_generic_set_parts(USBCompositePart** _parts, unsigned _numParts);
void usb_generic_disable(void);
void usb_generic_enable(void);


#endif
