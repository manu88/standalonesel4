#include "sel4.hpp"
#include "runtime.h"

// FIXME: use seL4's new way of getting boot info
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
seL4_BootInfo *seL4::GetBootInfo(void) { return seL4_GetBootInfo(); }
#pragma GCC diagnostic pop

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

static bool operator==(const seL4_CapRights_t& lhs, const seL4_CapRights_t& rhs){
  return lhs.words[0] == rhs.words[0];
}

const char *seL4::rightsStr(seL4_CapRights_t rights){
  if(rights == seL4_ReadWrite){
    return "ReadWrite";
  }
  if(rights == seL4_AllRights){
    return "AllRights";
  }
  if(rights == seL4_CanRead){
    return "CanRead";
  }
  if(rights == seL4_CanWrite){
    return "CanWrite";
  }
  if(rights == seL4_CanGrant){
    return "CanGrant";
  }
  if(rights == seL4_CanGrantReply){
    return "CanGrantReply";
  }
  if(rights == seL4_NoWrite){
    return "NoWrite";
  }
  if(rights == seL4_NoRead){
    return "NoRead";
  }
  if(rights == seL4_NoRights){
    return "NoRights";
  }
  return nullptr;
}