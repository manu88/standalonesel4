#pragma once
#include "../lib/expected.hpp"
#include "../lib/vector.hpp"
#include "PCIBlk.hpp"
#include "PCIScanner.hpp"
#include <memory>

class ObjectFactory;

class PlatformExpert {
public:
  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;

  bool init(ObjectFactory *factory);
  void print() const noexcept;

  SlotOrError issuePortRange(seL4_Word first_port, seL4_Word last_port);

private:
  void tryAssociatePCIDrivers();
  PCIScanner _pciScanner;
  ObjectFactory *_factory = nullptr;
  PCIBlk _pciblkDriver;
};