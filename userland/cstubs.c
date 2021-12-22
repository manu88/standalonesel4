#include "runtime.h"
#include "sel4.hpp"
#include <stdlib.h>
#include "klog.h"

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

extern "C" int strcmp(const char *X, const char *Y)
{
    while (*X)
    {
        // if characters differ, or end of the second string is reached
        if (*X != *Y) {
            break;
        }
 
        // move to the next pair of characters
        X++;
        Y++;
    }
 
    // return the ASCII difference after converting `char*` to `unsigned char*`
    return *(const unsigned char*)X - *(const unsigned char*)Y;
}

static bool is_delim(char c, char *delim)
{
  while(*delim != '\0')
  {
    if(c == *delim)
      return true;
    delim++;
  }
  return false;
}

extern "C" char *strtok(char *s, char *delim)
{
  static char *p; // start of the next search 
  if(!s)
  {
    s = p;
  }
  if(!s)
  {
    // user is bad user 
    return NULL;
  }

  // handle beginning of the string containing delims
  while(1)
  {
    if(is_delim(*s, delim))
    {
      s++;
      continue;
    }
    if(*s == '\0')
    {
      return NULL; // we've reached the end of the string
    }
    // now, we've hit a regular character. Let's exit the
    // loop, and we'd need to give the caller a string
    // that starts here.
    break;
  }

  char *ret = s;
  while(1)
  {
    if(*s == '\0')
    {
      p = s; // next exec will return NULL
      return ret;
    }
    if(is_delim(*s, delim))
    {
      *s = '\0';
      p = s + 1;
      return ret;
    }
    s++;
  }
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