#include <stdio.h>

typedef unsigned char uint8;

static constexpr unsigned ENDPOINT_MASK = 0x1000;
static constexpr unsigned INTERFACE_MASK = 0x2000;

constexpr uint8 convertByte(unsigned x, unsigned interfaceStart, unsigned endpointStart) {
    if (x&ENDPOINT_MASK) 
        return (x&0xFF)+endpointStart;
    else if (x&INTERFACE_MASK)
        return (x&0xFF)+interfaceStart;
    else
        return x;
}

template<unsigned interfaces, unsigned endpoints, uint8 interfaceStart, uint8 endpointStart, unsigned... args>struct USBCompositeComponent {
    constexpr static unsigned numInterfaces = interfaces;
    constexpr static unsigned numEndpoints = endpoints;
    constexpr static unsigned descriptor_config_size = (unsigned int)sizeof...(args);
    constexpr static uint8 convertByte(unsigned x) {
        if (x&ENDPOINT_MASK) 
            return (x&0xFF)+endpointStart;
        else if (x&INTERFACE_MASK)
            return (x&0xFF)+interfaceStart;
        else
            return x;
    }
    constexpr static uint8 descriptor_config[] = { convertByte(args)... };
};

constexpr USBCompositeComponent<1,2, 10,20, 1,2|ENDPOINT_MASK,3|INTERFACE_MASK> alpha;
main() {
    printf("%d %d", alpha.descriptor_config_size, alpha.descriptor_config[1]);
}