#pragma once
#include "DriverBase.hpp"
#include "Virtio.hpp"
#include "virtio_ring.h"
#include <cstddef>
#include <sys/types.h> // ssize_t
#include "../BlockDevice.hpp"

class PCIBlk : public DriverBase, public BlockDevice {
public:
  bool probe(const PCIDevice &dev) override;
  const char *getName() const noexcept override { return "BLK PCI"; }
  bool addDevice(PlatformExpert &expert, const PCIDevice &) override;

  size_t queueSize = 0;
  uint8_t queueID = 0;

  ssize_t read(size_t sector, char* buf, size_t bufSize) final;
private:
  ssize_t blkReadSector(size_t sector, char* buf, size_t bufSize);
  void* blkCmd(int op, size_t sector, char* buf, size_t bufSize);

  bool initializeDescRing(PlatformExpert &, const PCIDevice &);
  VirtioDevice _dev;
  struct vring rx_ring;
  /* R/T Descriptor Tail represents the next free slot to add
    * a descriptor */
  unsigned int rdt;
  uintptr_t rx_ring_phys;

  PlatformExpert *_expert = nullptr; // TEMP
  enum {DMA_ALIGNMENT = 16};
  seL4_Word irqCap = 0;

  void* readDMAVirt = nullptr;
  seL4_Word readDMAPhys = 0;
};
