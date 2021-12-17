#pragma once
#include <stdint.h>
#include "../sel4.hpp"

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS 18
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
#define VIRTIO_CONFIG_S_DRIVER		2
#define VIRTIO_PCI_GUEST_FEATURES   4 /* guest's supported features (32, RW) */
#define VIRTIO_PCI_HOST_FEATURES	0
#define VIRTIO_CONFIG_S_DRIVER_FEATURES_OK 8

struct VirtioDevice{
    seL4_Error setStatus(uint8_t status);
    uint8_t getStatus();
    seL4_Error addStatus(uint8_t status);
    uint32_t getFeatures();
    void setFeatures(uint32_t features);

    seL4_Word base0cap = 0;
    uint32_t iobase0 = 0;

    seL4_Error writeReg8(uint16_t port, uint8_t val);
    uint32_t readReg32(uint16_t port);
    void writeReg32(uint16_t port, uint32_t val);
};
