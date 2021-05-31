#pragma once
#include "sel4.hpp"

class Thread
{
    public:
        Thread();
        Thread(seL4_SlotPos tcbObject);

        bool isValid() const
        {
            return !_invalid;
        }

        void setName(const char* name);
        
        int resume();
        
        int setPriority(seL4_Word prio);

        seL4_Word getPriority() const 
        {
            return _prio;
        }

    private:
        bool _invalid;
        seL4_SlotPos _tcbObject = -1;
        seL4_Word _prio = 0; // seL4 gives us no get_priority, so cache the value
};
