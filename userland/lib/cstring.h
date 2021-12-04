#pragma once
#include <cerrno>
#include <climits>
#include <cstddef>
extern "C" {

extern void *kmalloc(size_t size);

static inline void *memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    ((char *)dest)[i] = ((char *)src)[i];
  }
  return dest;
}

static inline void *memset(void *s, int c, size_t len) {
  unsigned char *p = (unsigned char *)s;
  while (len--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

inline size_t strlen(const char *s) {
  size_t len = 0;
  for (len = 0; s[len]; (len)++)
    ;
  return len;
}

inline int isprint(int c) { return (unsigned)c - 0x20 < 0x5f; }

inline int isdigit(int c) { return (c >= '0') && (c <= '9'); }
inline int isalpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline int isspace(int c) {
  return c == ' ' || c == '\t'; // || whatever other char you consider space
}

int inline tolower(int c) {
  if (!isalpha(c))
    return c;
  return (c >= 'A' && c <= 'Z') ? c - 'A' : c;
}

int inline toupper(int ch) {
  if (ch >= 'a' && ch <= 'z')
    return ('A' + ch - 'a');
  else
    return ch;
}

inline long int strtol(const char *__restrict nPtr, char **__restrict endPtr,
                       int base) {
  const char *start;
  int number;
  long int sum = 0;
  int sign = 1;
  const char *pos = nPtr;
  if (*pos == '\0')
    return 0;
  start = pos;
  while (isspace(*pos)) {
    ++pos;
  }
  if (*pos == '-') {
    sign = -1;
    ++pos;
  }
  if (*pos == '+')
    ++pos;
  if (base == 16 || base == 8) {
    if (base == 16 && *pos == '0')
      ++pos;
    if (base == 16 && (*pos == 'x' || *pos == 'X'))
      ++pos;
    if (base == 8 && *pos == '0')
      ++pos;
  }
  if (base == 0) {
    base = 10;
    if (*pos == '0') {
      base = 8;
      ++pos;
      if (*pos == 'x' || *pos == 'X') {
        base = 16;
        ++pos;
      }
    }
  }
  if ((base < 2 || base > 36) && base != 0) {
    errno = EINVAL;
    return 0;
  }

  while (*pos != '\0') {
    number = -1;
    if ((int)*pos >= 48 && (int)*pos <= 57) {
      number = (int)*pos - 48;
    }
    if (isalpha(*pos)) {
      number = (int)toupper(*pos) - 55;
    }

    if (number < base && number != -1) {
      if (sign == -1) {
        if (sum >= ((LONG_MIN + number) / base))
          sum = sum * base - number;
        else {
          errno = ERANGE;
          sum = LONG_MIN;
        }
      } else {
        if (sum <= ((LONG_MAX - number) / base))
          sum = sum * base + number;
        else {
          errno = ERANGE;
          sum = LONG_MAX;
        }
      }
    } else if (base == 16 && number > base &&
               (*(pos - 1) == 'x' || *(pos - 1) == 'X')) {
      --pos;
      break;
    } else
      break;

    ++pos;
  }

  if (!isdigit(*(pos - 1)) && !isalpha(*(pos - 1)))
    pos = start;

  if (endPtr)
    *endPtr = (char *)pos;
  return sum;
}

inline char *strdup(const char *s) {
  size_t len = strlen(s) + 1;
  void *newS = kmalloc(len);
  if (newS == NULL)
    return NULL;
  return (char *)memcpy(newS, s, len);
}

inline char *strncpy(char *destination, const char *source, size_t num) {
  // return if no memory is allocated to the destination
  if (destination == NULL) {
    return NULL;
  }

  // take a pointer pointing to the beginning of the destination string
  char *ptr = destination;

  // copy first `num` characters of C-string pointed by source
  // into the array pointed by destination
  while (*source && num--) {
    *destination = *source;
    destination++;
    source++;
  }

  // null terminate destination string
  *destination = '\0';

  // the destination is returned by standard `strncpy()`
  return ptr;
}

} // extern "C"