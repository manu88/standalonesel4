#pragma once

extern "C" int printf_(const char *format, ...);
#ifndef kprintf
#define kprintf(fmt, ...) printf_(fmt, ##__VA_ARGS__)
#endif