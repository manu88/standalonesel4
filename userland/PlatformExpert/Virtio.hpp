#pragma once
#include "../sel4.hpp"
#include <stdint.h>

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS 18
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER 2
#define VIRTIO_CONFIG_S_DRIVER_OK 4
#define VIRTIO_PCI_GUEST_FEATURES 4 /* guest's supported features (32, RW) */
#define VIRTIO_PCI_HOST_FEATURES 0
#define VIRTIO_CONFIG_S_DRIVER_FEATURES_OK 8
#define VIRTIO_PCI_QUEUE_NUM 12 /* number of ring entries (16, RO) */
#define VIRTIO_PCI_QUEUE_SEL 14 /* current VQ selection (16, RW) */
#define VIRTIO_PCI_VRING_ALIGN 4096
#define VIRTIO_PCI_QUEUE_PFN 8

#define VIRTIO_PCI_QUEUE_NOTIFY 16

#define VIRTIO_PCI_ISR 19

typedef struct {
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_SCSI 2
#define VIRTIO_BLK_T_FLUSH 4
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
  uint8_t status;
} __attribute__((packed)) virtio_blk_req;

struct VirtioDevice {
  seL4_Error setStatus(uint8_t status);
  uint8_t getStatus();
  seL4_Error addStatus(uint8_t status);
  uint32_t getFeatures();
  void setFeatures(uint32_t features);

  seL4_Word base0cap = 0;
  uint32_t iobase0 = 0;

  seL4_Error writeReg8(uint16_t port, uint8_t val);
  seL4_Error writeReg16(uint16_t port, uint16_t val);
  seL4_Error writeReg32(uint16_t port, uint32_t val);

  uint32_t readReg32(uint16_t port);
  uint16_t readReg16(uint16_t port);
  uint8_t readReg8(uint16_t port);

  /* preallocated header. Since we do not actually use any features
   * in the header we put the same one before every send/receive packet */
  uintptr_t hdr_phys;

  virtio_blk_req *headerReq;
};
