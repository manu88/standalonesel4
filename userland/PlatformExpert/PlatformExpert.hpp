#pragma once
#include "PCIScanner.hpp"
#include "../lib/vector.hpp"
#include <memory>
#include "PCIBlk.hpp"


class ObjectFactory;

class PlatformExpert {
public:
  bool init(ObjectFactory *factory);
  void print() const noexcept;

private:
  void tryAssociatePCIDrivers();
  PCIScanner _pciScanner;
  ObjectFactory *_factory = nullptr;
  PCIBlk _pciblkDriver;
};