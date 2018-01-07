#ifndef _USB_GENERIC_h
#define _USB_GENERIC_H
#include <libmaple/libmaple_types.h>
#include <libmaple/usb.h>
#include "usb_lib_globals.h"
#include "usb_reg_map.h"
#include <usb_core.h>

void usb_generic_enable(DEVICE* deviceTable, DEVICE_PROP* deviceProperty, USER_STANDARD_REQUESTS* userStandardRequests,
    void (**ep_int_in)(void), void (**ep_int_out)(void));
void usb_generic_disable(void);
#endif
