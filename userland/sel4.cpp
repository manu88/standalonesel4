#include "sel4.hpp"
#include "runtime.h"

// FIXME: use seL4's new way of getting boot info
seL4_BootInfo *seL4::GetBootInfo(void) { return seL4_GetBootInfo(); }

const char *seL4::errorStr(seL4_Error err) {
  switch (err) {
  case seL4_NoError:
    return "seL4_NoError";
  case seL4_InvalidArgument:
    return "seL4_InvalidArgument";
  case seL4_InvalidCapability:
    return "seL4_InvalidCapability";
  case seL4_IllegalOperation:
    return "seL4_IllegalOperation";
  case seL4_RangeError:
    return "seL4_RangeError";
  case seL4_AlignmentError:
    return "seL4_AlignmentError";
  case seL4_FailedLookup:
    return "seL4_FailedLookup";
  case seL4_TruncatedMessage:
    return "seL4_TruncatedMessage";
  case seL4_DeleteFirst:
    return "seL4_DeleteFirst";
  case seL4_RevokeFirst:
    return "seL4_RevokeFirst";
  case seL4_NotEnoughMemory:
    return "seL4_NotEnoughMemory";
  default:
    return nullptr;
  }
  NOT_REACHED();
}