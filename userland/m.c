extern char __executable_start[];
extern char _end[];

int _start()
{
    volatile char *p = __executable_start;
    volatile char *pp = _end;
    while (1)
    {
    }    
}