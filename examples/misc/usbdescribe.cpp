#include <stdio.h>

typedef unsigned char uint8;

template<unsigned interfaces, unsigned endpoints, uint8... args>struct USBCompositeComponent {
    constexpr static unsigned numInterfaces = interfaces;
    constexpr static unsigned numEndpoints = endpoints;
    constexpr static unsigned descriptor_config_size = (unsigned int)sizeof...(args);
    constexpr static uint8 descriptor_config[] = { args... };
};

constexpr USBCompositeComponent<1,2, 1,2,3> alpha;
main() {
    printf("%d %d", alpha.descriptor_config_size, alpha.descriptor_config[1]);
}