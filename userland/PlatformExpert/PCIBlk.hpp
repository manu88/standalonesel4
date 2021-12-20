#pragma once
#include "DriverBase.hpp"
#include "Virtio.hpp"
#include "virtio_ring.h"
#include <cstddef>

class PCIBlk : public DriverBase {
public:
  bool probe(const PCIDevice &dev) override;
  const char *getName() const noexcept override { return "BLK PCI"; }
  bool addDevice(PlatformExpert &expert, const PCIDevice &) override;

  size_t queueSize = 0;
  uint8_t queueID = 0;
private:
  bool initializeDescRing(PlatformExpert &, const PCIDevice &);
  VirtioDevice _dev;
  struct vring rx_ring;
  /* R/T Descriptor Tail represents the next free slot to add
    * a descriptor */
  unsigned int rdt;
  uintptr_t rx_ring_phys;
};
