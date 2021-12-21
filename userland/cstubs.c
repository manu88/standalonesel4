#include "runtime.h"
#include "sel4.hpp"
#include <stdlib.h>

int __errno_location = 0;
// TODO: actual implementation :)
extern "C" int __cxa_guard_acquire(int *)
{
    return 0;
}
extern "C" void __cxa_guard_release(int*)
{

}
extern "C" void __stack_chk_fail(void)
{
    
}


extern "C" void __assert_fail( char const* assertion,  char const* file, int line,  char const* function)
{
    kprintf("assertion failed %s file line %d in function %s %s\n", assertion, file, line, function);
    oops();
}


extern "C" char *strcpy(char *dest,  char const*src)
{
    char *save = dest;
    while((*dest++ = *src++));
    return save;
}

extern "C" void *memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    ((char *)dest)[i] = ((char *)src)[i];
  }
  return dest;
}

extern "C"  int memcmp(const void *str1, const void *str2, size_t count){
  const unsigned char *s1 = (const unsigned char*)str1;
  const unsigned char *s2 = (const unsigned char*)str2;
  while (count-- > 0)
    {
      if (*s1++ != *s2++)
          return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}


extern "C" void *memmove(void *dest, const void *src, size_t n)
{
	char* from = (char*) src;
	char* to = (char*) dest;

	if (from == to || n == 0)
		return dest;
	if (to > from && to-from < (int)n) {
		/* to overlaps with from */
		/*  <from......>         */
		/*         <to........>  */
		/* copy in reverse, to avoid overwriting from */
		int i;
		for(i=n-1; i>=0; i--)
			to[i] = from[i];
		return dest;
	}
	if (from > to && from-to < (int)n) {
		/* to overlaps with from */
		/*        <from......>   */
		/*  <to........>         */
		/* copy forwards, to avoid overwriting from */
		size_t i;
		for(i=0; i<n; i++)
			to[i] = from[i];
		return dest;
	}
	memcpy(dest, src, n);
	return dest;
}


extern void __assert (const char *msg, const char *file, int line){
  kprintf("assertion failed in file '%s' at line %i: %s\n", file, line, msg);
  float *f = NULL;
  *f = 42.f;
}