#include "Virtio.hpp"

seL4_Error VirtioDevice::setStatus(uint8_t status) {
  return writeReg8(VIRTIO_PCI_STATUS, status);
}

uint8_t VirtioDevice::getStatus() {
  auto val = seL4_X86_IOPort_In16(base0cap, iobase0 + VIRTIO_PCI_STATUS);
  return val.result;
}

seL4_Error VirtioDevice::addStatus(uint8_t status) {
  return writeReg8(VIRTIO_PCI_STATUS, getStatus() | status);
}

uint32_t VirtioDevice::getFeatures() {
  return readReg32(VIRTIO_PCI_HOST_FEATURES);
}

void VirtioDevice::setFeatures(uint32_t features) {
  writeReg32(VIRTIO_PCI_GUEST_FEATURES, features);
}

seL4_Error VirtioDevice::writeReg32(uint16_t port, uint32_t val) {
  return seL4_X86_IOPort_Out32(base0cap, iobase0 + port, val);
}

seL4_Error VirtioDevice::writeReg16(uint16_t port, uint16_t val) {
  return seL4_X86_IOPort_Out16(base0cap, iobase0 + port, val);
}

seL4_Error VirtioDevice::writeReg8(uint16_t port, uint8_t val) {
  return seL4_X86_IOPort_Out16(base0cap, iobase0 + port, val);
}

uint32_t VirtioDevice::readReg32(uint16_t port) {
  auto r = seL4_X86_IOPort_In32(base0cap, iobase0 + port);
  return r.result;
}

uint16_t VirtioDevice::readReg16(uint16_t port) {
  auto r = seL4_X86_IOPort_In16(base0cap, iobase0 + port);
  return r.result;
}

uint8_t VirtioDevice::readReg8(uint16_t port) {
  auto r = seL4_X86_IOPort_In8(base0cap, iobase0 + port);
  return r.result;
}