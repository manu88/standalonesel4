#include "PCIScan.hpp"
#include "../runtime.h"
#include <stdint.h>
#include "pciDevices.h"

void PCIScanner::init(seL4_CPtr pciAddressSlot, seL4_CPtr pciDataSlot){
    _pciAddressSlot = pciAddressSlot;
    _pciDataSlot = pciDataSlot;
}


static uint16_t pciConfigReadWord(seL4_CNode pciConfigAddress, seL4_CNode pciDataAddress, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    auto err = seL4_X86_IOPort_Out32(pciConfigAddress, PCI_CONFIG_ADDRESS, address);
    if(err != seL4_NoError){
        kprintf("error seL4_X86_IOPort_Out8 %i\n", err);
    }
    assert(err == seL4_NoError);

    seL4_X86_IOPort_In32_t val = seL4_X86_IOPort_In32(pciDataAddress, PCI_CONFIG_DATA);
    if(err != seL4_NoError){
        kprintf("Read err=%i, val = 0X%X\n", val.error, val.result);
    }
    assert(err == seL4_NoError);
    return (uint16_t)((val.result >> ((offset & 2) * 8)) & 0xFFFF);
}

static uint16_t getPCIVendorID(seL4_CNode pciConfigAddress, seL4_CNode pciDataAddress, uint8_t bus, uint8_t slot){
    return pciConfigReadWord(pciConfigAddress, pciDataAddress, bus, slot, 0, 0);
}

void PCIScanner::scan(){
        union CommandReg
    {
        uint16_t value;
        struct Fields
        {
            uint8_t ioSpace: 1;
            uint8_t memSpace: 1;
            uint8_t busMaster: 1;
            uint8_t specialCycles: 1;
            uint8_t memoryWriteAndInvalidateEnable: 1;
            uint8_t vGAPaletteSnoop: 1;
            uint8_t parityErrorResponse: 1;
            uint8_t reserved0: 1;
            uint8_t serrEnable: 1;
            uint8_t fastBackToBackEnable: 1;
            uint8_t interruptDisable: 1;
        } fields;
    };
    static_assert(sizeof(CommandReg) == 2);

    union RevIdAndProgIf
    {
        uint16_t value;
        struct Fields
        {
            uint8_t revId;
            uint8_t progIf;
        } fields;  
    };
    static_assert(sizeof(RevIdAndProgIf) == 2);

    union SubClassAndClass
    {
        uint16_t value;
        struct Fields
        {
            uint8_t subclassCode;
            uint8_t classCode;
        } fields;  
    };
    static_assert(sizeof(SubClassAndClass) == 2);

    union BistAndHeaderType
    {
        uint16_t value;
        struct Fields
        {
            uint8_t headerType;
            uint8_t bist;
        } fields;  
    };
    static_assert(sizeof(BistAndHeaderType) == 2);
    
    
    for(int bus=0; bus<256;bus++){
        for(int slot=0; slot<256;slot++){
            uint16_t vendorID = getPCIVendorID(_pciAddressSlot, _pciDataSlot, bus, slot);
            if(vendorID == 0XFFFF){
                continue;
            }
            uint16_t deviceID = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 2);
            CommandReg cmdReg;

            cmdReg.value = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 4);

            uint16_t status = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 6);
            
            RevIdAndProgIf revIdAndProgIf;
            revIdAndProgIf.value = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 8);

            SubClassAndClass subClassAndClass;
            subClassAndClass.value = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 10);

            BistAndHeaderType bistAndHeaderType;
            bistAndHeaderType.value = pciConfigReadWord(_pciAddressSlot, _pciDataSlot, bus, slot, 0, 14);

            const char *devName = libpci_deviceID_str(vendorID, deviceID);
            kprintf("\n");
            kprintf("bus %X slot %X -> vendor = 0X%X device = 0X%X  -> '%s'\n", bus, slot, vendorID, deviceID, devName);
            kprintf("Class=0x%X subClass=0X%X\n", subClassAndClass.fields.classCode, subClassAndClass.fields.subclassCode);
            kprintf("status 0X%X revId=0X%X ProgIf=0X%X headerType=0X%X bist=0X%X\n", status, revIdAndProgIf.fields.revId, revIdAndProgIf.fields.progIf, bistAndHeaderType.fields.headerType, bistAndHeaderType.fields.bist);
            kprintf("\n");
            //kprintf("command reg: ioSpace %i  memSpace %i  busMaster %i  specialCycles %i  memoryWriteAndInvalidateEnable %i  vGAPaletteSnoop %i  parityErrorResponse %i  reserved0 %i  serrEnable %i  fastBackToBackEnable %i  interruptDisable %i",cmdReg.fields.ioSpace,cmdReg.fields.memSpace,cmdReg.fields.busMaster,cmdReg.fields.specialCycles,cmdReg.fields.memoryWriteAndInvalidateEnable,cmdReg.fields.vGAPaletteSnoop,cmdReg.fields.parityErrorResponse,cmdReg.fields.reserved0,cmdReg.fields.serrEnable,cmdReg.fields.fastBackToBackEnable,cmdReg.fields.interruptDisable);
        }
    }

}