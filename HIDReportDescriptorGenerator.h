#ifndef _HIDReportDescriptorGenerator_H_
#define _HIDReportDescriptorGenerator_H_

#define REPORT(name, ...) \
    static uint8_t raw_ ## name[] = { __VA_ARGS__ }; \
    static const HIDReportDescriptor desc_ ## name = { raw_ ##name, sizeof(raw_ ##name) }; \
    const HIDReportDescriptor* hidReport ## name = & desc_ ## name;

#endif
