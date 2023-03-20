#ifndef SAFESTRING_H

#define SAFESTRING_H

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern char* ourstrcpy(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source);

extern char* ourstrcat(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* source);

extern int oursprintf(const char* fileName, const int lineNum, char* dest, size_t destSize, const char* fmt, ...);


#define OUR_OPEN_PAREN (
#define OUR_CLOSE_PAREN )

#define OSTRCPY(...)  ourstrcpy  OUR_OPEN_PAREN __FILE__, __LINE__, __VA_ARGS__ OUR_CLOSE_PAREN
#define OSTRCAT(...)  ourstrcat  OUR_OPEN_PAREN __FILE__, __LINE__, __VA_ARGS__ OUR_CLOSE_PAREN
#define OSPRINTF(...) oursprintf OUR_OPEN_PAREN __FILE__, __LINE__, __VA_ARGS__ OUR_CLOSE_PAREN

#endif
