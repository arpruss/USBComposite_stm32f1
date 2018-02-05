/*
  Copyright (c) 2012 Arduino.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include <boards.h>
#include "Stream.h"
#include "usb_generic.h"
#include <libmaple/usb.h>

#define USB_MAX_PRODUCT_LENGTH 32
#define USB_MAX_MANUFACTURER_LENGTH 32
#define USB_MAX_SERIAL_NUMBER_LENGTH  20


// You could use this for a serial number, but you'll be revealing the device ID to the host,
// and hence burning it for cryptographic purposes.
const char* getDeviceIDString();

#define USB_COMPOSITE_MAX_PARTS 6

class USBCompositeDevice;

#define DEFAULT_SERIAL_STRING "00000000000000000001"

//typedef std::function<bool()> USBPartInitializer;
//typedef std::function<void()> USBPartStopper;
typedef bool(*USBPartInitializer)(void*);
typedef void(*USBPartStopper)(void*);

class USBCompositeDevice {
private:
	bool enabled = false;
    bool haveSerialNumber = false;
    uint8_t iManufacturer[USB_DESCRIPTOR_STRING_LEN(USB_MAX_MANUFACTURER_LENGTH)];
    uint8_t iProduct[USB_DESCRIPTOR_STRING_LEN(USB_MAX_PRODUCT_LENGTH)];
    uint8_t iSerialNumber[USB_DESCRIPTOR_STRING_LEN(USB_MAX_SERIAL_NUMBER_LENGTH)]; 
    uint16 vendorId;
    uint16 productId;
    USBCompositePart* parts[USB_COMPOSITE_MAX_PARTS];
	USBPartInitializer init[USB_COMPOSITE_MAX_PARTS];
	USBPartStopper stop[USB_COMPOSITE_MAX_PARTS];
	void* plugin[USB_COMPOSITE_MAX_PARTS];
    uint32 numParts;
public:
	USBCompositeDevice(void); 
    void setVendorId(uint16 vendor=0);
    void setProductId(uint16 product=0);
    void setManufacturerString(const char* manufacturer=NULL);
    void setProductString(const char* product=NULL);
    void setSerialString(const char* serialNumber=DEFAULT_SERIAL_STRING);
    bool begin(void);
    void end(void);
    void clear();
    bool add(USBCompositePart* part, USBPartInitializer init = NULL, USBPartStopper stop = NULL, void* plugin = NULL);
};

extern USBCompositeDevice USBComposite;
#endif
        