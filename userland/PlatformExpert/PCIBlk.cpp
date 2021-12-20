#include "PCIBlk.hpp"
#include "../runtime.h"
#include "../sel4.hpp"
#include "PCIScanner.hpp"
#include "PlatformExpert.hpp"

struct virtio_cap {
  const char *name;
  uint32_t bit;
  bool support;
  const char *help;
};

struct virtio_cap indp_caps[] = {
    {"VIRTIO_F_RING_INDIRECT_DESC", 1 << 28, false,
     "Negotiating this feature indicates that the driver can use"
     " descriptors with the VIRTQ_DESC_F_INDIRECT flag set, as"
     " described in 2.4.5.3 Indirect Descriptors."},
    {"VIRTIO_F_RING_EVENT_IDX", 1 << 29, true,
     "This feature enables the used_event and the avail_event fields"
     " as described in 2.4.7 and 2.4.8."},
    /*{"VIRTIO_F_VERSION_1", 1<<32, false,
            "This indicates compliance with this specification, giving a"
            " simple way to detect legacy devices or drivers."},*/
};

struct virtio_cap blk_caps[] = {
    {"VIRTIO_BLK_F_SIZE_MAX", 1 << 1, true,
     "Maximum size of any single segment is in size_max."},
    {"VIRTIO_BLK_F_SEG_MAX", 1 << 2, true,
     "Maximum number of segments in a request is in seg_max."},
    {"VIRTIO_BLK_F_GEOMETRY", 1 << 4, false,
     "Disk-style geometry specified in geometry."},
    {"VIRTIO_BLK_F_RO", 1 << 5, false, "Device is read-only."},
    {"VIRTIO_BLK_F_BLK_SIZE", 1 << 6, true,
     "Block size of disk is in blk_size."},
    {"VIRTIO_BLK_F_FLUSH", 1 << 9, false, "Cache flush command support."},
    {"VIRTIO_BLK_F_TOPOLOGY", 1 << 10, false,
     "Device exports information on optimal I/O alignment."},
    {"VIRTIO_BLK_F_CONFIG_WCE", 1 << 11, false,
     "Device can toggle its cache between writeback and "
     "writethrough modes."},
};

static void virtio_check_capabilities(uint32_t *device, uint32_t *request,
                                      struct virtio_cap *caps, uint32_t n) {
  uint32_t i;
  for (i = 0; i < n; i++) {
    if (*device & caps[i].bit) {
      if (caps[i].support) {
        *request |= caps[i].bit;
      } else {
        kprintf("virtio supports unsupported option %s (%s)\n", caps[i].name,
                caps[i].help);
      }
    }
    *device &= ~caps[i].bit;
  }
}

bool PCIBlk::probe(const PCIDevice &dev) {
  if (dev.class_ == PCIDevice::Class::MassStorageController &&
      dev.subSystemID == 2) {
    return true;
  }
  return false;
}

bool PCIBlk::initializeDescRing(PlatformExpert &expert, const PCIDevice &dev) {
    unsigned sizeVRing = vring_size(queueSize, VIRTIO_PCI_VRING_ALIGN);

    auto rx_ring_bufOrErr = expert.allocDMARange(sizeVRing);// dma_alloc_pin(dma_man, sizeVRing, 1, VIRTIO_PCI_VRING_ALIGN);
    kprintf("allocDMARange returned err %s\n", seL4::errorStr(rx_ring_bufOrErr.error));
    if(rx_ring_bufOrErr.error == seL4_NoError){
      assert(0);
    }
    return false;
#if 0
    auto rx_ring_buf = rx_ring_bufOrErr.value;
    if (!rx_ring_buf.phys) {
        kprintf("Failed to allocate rx_ring");
        return false;
    }
    memset(rx_ring_buf.virt, 0, vring_size(queueSize, VIRTIO_PCI_VRING_ALIGN));
    vring_init(&rx_ring, queueSize, rx_ring_buf.virt, VIRTIO_PCI_VRING_ALIGN);
    rx_ring_phys = rx_ring_buf.phys;

    //dev->rdh = 0;
    rdt = 0;
    //dev->ruh = 0;

    return true;
#endif
}

bool PCIBlk::addDevice(PlatformExpert &expert, const PCIDevice &dev) {
  uint32_t iobase0 = dev.getBaseAddr32(0);
  uint32_t iobase0size = dev.getBaseAddrSize32(0);
  kprintf("PCIBlk::addDevice iobase0 is at 0X%X 0X%X\n", iobase0, iobase0size);
  auto base0cap = expert.issuePortRange(iobase0, iobase0 + iobase0size - 1);
  if (!base0cap) {
    kprintf("Get base0 error %s\n", seL4::errorStr(base0cap.error));
  }
  assert(base0cap);
  _dev.base0cap = base0cap.value;
  _dev.iobase0 = iobase0;
  _dev.setStatus(0);
  _dev.getStatus();
  _dev.addStatus(VIRTIO_CONFIG_S_ACKNOWLEDGE);
  _dev.addStatus(VIRTIO_CONFIG_S_DRIVER);
  kprintf("BLK status = 0X%X\n", _dev.getStatus());

  auto features = _dev.getFeatures();
  kprintf("BLK features = 0X%X\n", features);
  uint32_t request_features = 0;
  virtio_check_capabilities(&features, &request_features, blk_caps, 8);
  virtio_check_capabilities(&features, &request_features, indp_caps, 2);
  if (features) {
    kprintf("virtio supports undocumented options 0x%x!\n", features);
    for (int i = 0; i < 32; i++) {
      if ((features >> i) & 1U) {
        kprintf("FEATURES: feat %i set\n", i);
      }
    }
  }
  _dev.setFeatures(request_features);
  _dev.addStatus(VIRTIO_CONFIG_S_DRIVER_FEATURES_OK);

  asm volatile("mfence" ::: "memory");
  if (!(_dev.getStatus() & VIRTIO_CONFIG_S_DRIVER_FEATURES_OK)) {
    kprintf("HOST REFUSED OUR FEATURES\n");
    assert(0);
  } else {
    kprintf("HOST is ok with our features\n");
  }

  _dev.addStatus(VIRTIO_CONFIG_S_DRIVER_FEATURES_OK);
  asm volatile("mfence" ::: "memory");

  uint32_t totSectorCount = _dev.readReg32(0x14);
  uint8_t maxSegSize = _dev.readReg32(0x1C);
  uint8_t maxSegCount = _dev.readReg32(0x20);
  uint8_t cylinderCount = _dev.readReg32(0x24);
  uint8_t headCount = _dev.readReg32(0x26);
  uint8_t sectorCount = _dev.readReg32(0x27);
  uint8_t blockLen = _dev.readReg32(0x28);

  kprintf("totSectorCount %u\n", totSectorCount);
  kprintf("maxSegSize %u\n", maxSegSize);
  kprintf("maxSegCount %u\n", maxSegCount);
  kprintf("cylinderCount %u\n", cylinderCount);
  kprintf("headCount %u\n", headCount);
  kprintf("sectorCount %u\n", sectorCount);
  kprintf("blockLen %u\n", blockLen);

  uint8_t numQueues = 0;
  for (int index = 0; index < 16; index++) {
    _dev.writeReg16(VIRTIO_PCI_QUEUE_SEL, index);
    uint16_t queueSize = _dev.readReg16(VIRTIO_PCI_QUEUE_NUM);
    if (queueSize == 0) {
      continue;
    }
    queueSize = queueSize;
    queueID = index;
    numQueues++;
    kprintf("Queue %i size %i\n", index, queueSize);
  }
  kprintf("Virtio blk has %i available queues\n", numQueues);

  initializeDescRing(expert, dev);

  return false;
}