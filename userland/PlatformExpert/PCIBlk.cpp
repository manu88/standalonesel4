#include "../Thread.hpp"
#include "PCIBlk.hpp"
#include "../runtime.h"
#include "../sel4.hpp"
#include "../Platform.hpp"
#include "PCIScanner.hpp"
#include "PlatformExpert.hpp"


#define VIRTIO_BLK_S_OK       0
#define VIRTIO_BLK_S_IOERR    1
#define VIRTIO_BLK_S_UNSUPP   2
#define VIRTIO_BLK_REQ_FOOTER_SIZE 1
#define VIRTIO_BLK_SECTOR_SIZE 512
#define VIRTIO_BLK_REQ_HEADER_SIZE 16

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
    kprintf("PCIBlk::initializeDescRing: sizeVRing=%zi\n", sizeVRing);
    // 5126
    auto rx_ring_bufOrErr = expert.allocDMARange(sizeVRing);// dma_alloc_pin(dma_man, sizeVRing, 1, VIRTIO_PCI_VRING_ALIGN);
    kprintf("allocDMARange returned err %s\n", seL4::errorStr(rx_ring_bufOrErr.error));
    auto rx_ring_buf = rx_ring_bufOrErr.value;
    if (!rx_ring_buf.phys) {
        kprintf("Failed to allocate rx_ring");
        return false;
    }
    memset(rx_ring_buf.virt, 0, vring_size(queueSize, VIRTIO_PCI_VRING_ALIGN));
    vring_init(&rx_ring, queueSize, (void*) rx_ring_buf.virt, VIRTIO_PCI_VRING_ALIGN);

    rx_ring_phys = rx_ring_buf.phys;
    kprintf("rx_ring_phys is at 0X%X\n", rx_ring_phys);
    assert(rx_ring_phys % PAGE_SIZE == 0);

    //dev->rdh = 0;
    rdt = 0;
    //dev->ruh = 0;

    return true;
}

bool PCIBlk::addDevice(PlatformExpert &expert, const PCIDevice &dev) {
  _expert = &expert;
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
    uint16_t _queueSize = _dev.readReg16(VIRTIO_PCI_QUEUE_NUM);
    if (_queueSize == 0) {
      continue;
    }
    queueSize = _queueSize;
    queueID = index;
    numQueues++;
    kprintf("Queue %i size %i\n", index, queueSize);
  }
  kprintf("Virtio blk has %i available queues, queueSize=%zi\n", numQueues,queueSize);

  bool ret = initializeDescRing(expert, dev);
  assert(ret);

  auto packetOrErr = expert.allocDMARange(sizeof(virtio_blk_req));
  if(!packetOrErr){
    kprintf("Error getting dma memory for request %s\n", seL4::errorStr(packetOrErr.error));
  }
  memset(packetOrErr.value.virt, 0, sizeof(virtio_blk_req));

  _dev.hdr_phys = packetOrErr.value.phys;
  assert(_dev.hdr_phys);
  _dev.headerReq = (virtio_blk_req*) packetOrErr.value.virt;
  assert(_dev.headerReq != nullptr);

  assert(rx_ring_phys);
  /* write the virtqueue locations */

  _dev.writeReg16(VIRTIO_PCI_QUEUE_SEL, 0);
  _dev.writeReg32(VIRTIO_PCI_QUEUE_PFN, ((uintptr_t)rx_ring_phys) / 4096);

  _dev.addStatus(VIRTIO_CONFIG_S_DRIVER_OK);

  uint8_t defStatus = _dev.getStatus();
  kprintf("Dev status %u\n", defStatus);

  kprintf("Test getting MSI IRQ\n");
  auto irqCapOrErr = expert.getIOAPICIRQHandle(dev);
  if(!irqCapOrErr){
    kprintf("Error getting irq cap from MSI: %s\n", seL4::errorStr(irqCapOrErr.error));
  }else{
    kprintf("Success!!\n");
    irqCap = irqCapOrErr.value;
  }
  return true;
}


ssize_t PCIBlk::read(size_t sector, char* buf, size_t bufSize){
    assert(bufSize % VIRTIO_BLK_SECTOR_SIZE == 0);
    if(bufSize <= VIRTIO_BLK_SECTOR_SIZE)
    {
        return blkReadSector(sector, buf, bufSize);
    }
    const size_t numSectors = bufSize / VIRTIO_BLK_SECTOR_SIZE;
    ssize_t tot = 0;
    for(size_t i=0;i <numSectors;i++)
    {
        ssize_t r = blkReadSector(sector+i, buf + (VIRTIO_BLK_SECTOR_SIZE*i), VIRTIO_BLK_SECTOR_SIZE);
        if(r <= 0)
        {
            return r;
        }
        tot += r;
    }
    return tot;
}

void* PCIBlk::blkCmd(int op, size_t sector, char* buf, size_t bufSize)
{
  memset(_dev.headerReq, 0, sizeof(virtio_blk_req));
  _dev.headerReq->type = op;// VIRTIO_BLK_T_IN or VIRTIO_BLK_T_OUT 
  _dev.headerReq->sector = sector;
  _dev.headerReq->status = 12; // set to a random val


  assert(_dev.hdr_phys);

  /* request a buffer */
  if(!readDMAPhys){
    auto dmaDataOrErr = _expert->allocDMARange(bufSize);
    if(!dmaDataOrErr){
      return NULL;
    }
    auto dma_data = dmaDataOrErr.value;

    readDMAPhys = dma_data.phys;// driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie);
    readDMAVirt = dma_data.virt;
  }
  if (!readDMAPhys) 
  {
      return NULL;
  }
  if(op == VIRTIO_BLK_T_OUT)
  {
      memcpy(readDMAVirt, buf, bufSize);
      //dev->headerReq->type |= VIRTIO_BLK_T_FLUSH;
  }
  else
  {
      memset(readDMAVirt, 0, bufSize);
  }

  assert(readDMAPhys % DMA_ALIGNMENT == 0);

  uintptr_t footerPhys = _dev.hdr_phys + VIRTIO_BLK_REQ_HEADER_SIZE;
  if(footerPhys % DMA_ALIGNMENT !=0)
  {
      footerPhys += DMA_ALIGNMENT - footerPhys % DMA_ALIGNMENT;
  }

  assert(footerPhys % DMA_ALIGNMENT == 0);

  unsigned int rdt_data = (rdt + 1) % queueSize;
  unsigned int rdt_footer = (rdt + 2) % queueSize;

  rx_ring.desc[rdt] = (struct vring_desc) {
      .addr = _dev.hdr_phys,
      .len = VIRTIO_BLK_REQ_HEADER_SIZE,
      .flags = VRING_DESC_F_NEXT,
      .next = rdt_data
  };

  int dataFlag = VRING_DESC_F_NEXT;
  if(op == VIRTIO_BLK_T_IN)
  {
      dataFlag |= VRING_DESC_F_WRITE;
  }
  rx_ring.desc[rdt_data] = (struct vring_desc) {
      .addr = readDMAPhys,
      .len = VIRTIO_BLK_SECTOR_SIZE,
      .flags = dataFlag,
      .next = rdt_footer
  };

  rx_ring.desc[rdt_footer] = (struct vring_desc) {
      .addr = footerPhys,
      .len = 1,
      .flags = VRING_DESC_F_WRITE,
      .next = 0
  };

  rx_ring.avail->ring[rx_ring.avail->idx % queueSize] = rdt;

  asm volatile("sfence" ::: "memory");
  rx_ring.avail->idx++;
  asm volatile("sfence" ::: "memory");
  assert(queueID == 0);
  _dev.writeReg16(VIRTIO_PCI_QUEUE_NOTIFY, queueID);
  rdt = (rdt + 1) % queueSize;
  kprintf("Start 'sleep'\n");
  seL4_Word sender = 0;
  //seL4_Wait(irqCap, &sender);
  for(uint64_t t = 0; t< UINT8_MAX; t++){}
  kprintf("End 'sleep' sender is %X\n", sender);
  return readDMAVirt;
}

ssize_t PCIBlk::blkReadSector(size_t sector, char* buf, size_t bufSize)
{
  assert(rx_ring.used);
  int i = rx_ring.used->idx % queueSize;
  void* dma_virt = blkCmd(VIRTIO_BLK_T_IN, sector, buf, bufSize);
  if(!dma_virt){
    return -1;
  }
  //blk_debug(dev);
  virtio_blk_req *req = _dev.headerReq;// .rx_ring.desc[desc1].addr;
  if(req->status != VIRTIO_BLK_S_OK)
  {
      kprintf("Error status not OK %i\n", req->status);
      //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);        
      return -1;
  }

  uint32_t desc1 = rx_ring.used->ring[i].id % queueSize;
  uint32_t desc2 = 0;
  uint32_t desc3 = 0;

  if(!(rx_ring.desc[desc1].flags & VRING_DESC_F_NEXT))
  {
      kprintf("desc1 (%i) is missing the Next desc flag!\n", desc1);
      kprintf("SHOULD RELEASE DMA PAGE\n");
      //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);
      return -1;
  }
  desc2 = rx_ring.desc[desc1].next % queueSize;

  if(!(rx_ring.desc[desc2].flags & VRING_DESC_F_NEXT))
  {
      kprintf("desc2 (%i) is missing the Next desc flag!\n", desc2);
      kprintf("Desc1 is %i\n", desc1);
      //blk_debug(dev);
      kprintf("SHOULD RELEASE DMA PAGE\n");
      //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);        
      return -1;
  }

  desc3 = rx_ring.desc[desc2].next % queueSize;
  if(rx_ring.desc[desc2].len != VIRTIO_BLK_SECTOR_SIZE)
  {
      kprintf("desc2' (%i) size is not VIRTIO_BLK_SECTOR_SIZE but %i\n", desc3, rx_ring.desc[desc2].len);
      kprintf("SHOULD RELEASE DMA PAGE\n");
      //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);        
      return -1;
  }

  if(rx_ring.desc[desc3].len != VIRTIO_BLK_REQ_FOOTER_SIZE)
  {
      kprintf("desc3' size is not VIRTIO_BLK_REQ_FOOTER_SIZE but %i\n", rx_ring.desc[desc3].len);
      kprintf("SHOULD RELEASE DMA PAGE\n");
      //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);        
      return -1;
  }
  memcpy(buf, dma_virt, bufSize);

  kprintf("SHOULD RELEASE DMA PAGE\n");
  //dma_unpin_free(&dev->dma_man, dma_virt, bufSize);      

  rx_ring.desc[desc1].addr = 0;
  rx_ring.desc[desc1].len = 0;
  rx_ring.desc[desc1].next = 0;
  rx_ring.desc[desc1].flags = 0;

  rx_ring.desc[desc2].addr = 0;
  rx_ring.desc[desc2].len = 0;
  rx_ring.desc[desc2].next = 0;
  rx_ring.desc[desc2].flags = 0;

  rx_ring.desc[desc3].addr = 0;
  rx_ring.desc[desc3].len = 0;
  rx_ring.desc[desc3].next = 0;
  rx_ring.desc[desc3].flags = 0;

  return bufSize;
}
