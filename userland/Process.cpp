#include "Process.hpp"


Process::Process(InitialUntypedPool &pool):_vmspace(VMSpace::RootServerLayout::ReservedVaddr), _pageTable(pool){}