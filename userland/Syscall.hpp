#pragma once
#include "sel4.hpp"

namespace Syscall {

enum class Id : seL4_Word { Unknown, VMStats };

void perform(Id syscall, seL4_Word endpoint);
} // namespace Syscall