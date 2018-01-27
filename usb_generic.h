#ifndef _USB_GENERIC_h
#define _USB_GENERIC_H
#include <libmaple/libmaple_types.h>
#include <libmaple/usb.h>
#include "usb_lib_globals.h"
#include "usb_reg_map.h"
#include <usb_core.h>

#define PMA_MEMORY_SIZE 512
#define MAX_USB_DESCRIPTOR_DATA_SIZE 256

void usb_generic_enable(DEVICE* deviceTable, DEVICE_PROP* deviceProperty, USER_STANDARD_REQUESTS* userStandardRequests,
void (**ep_int_in)(void), void (**ep_int_out)(void));
void usb_generic_disable(void);

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
    uint16 startInterface;
    uint16 descriptorSize;
    uint8* (*getPartDescriptor)(struct USBCompositePart* part);
    void (*usbInit)(struct USBCompositePart* part);
    void (*usbReset)(struct USBCompositePart* part);
    RESULT (*usbDataSetup)(struct USBCompositePart* part, uint8 request);
    RESULT (*usbNoDataSetup)(struct USBCompositePart* part, uint8 request);
    USBEndpointInfo* endpoints;
} USBCompositePart;

uint8 usb_generic_setup(USBCompositePart** _parts, uint32 _numParts);

const uint16 USBEndpointAddresses[7] = {
    
};

#endif
