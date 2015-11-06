#ifndef __UTILS_H__
#define __UTILS_H__

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ERROR_MSG(...) do{fprintf(stderr,"(%s:%u)\tERROR: ", \
        __FILE__, __LINE__);fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr);} while(0)

#define ERROR() do{fprintf(stderr, "(%s:%u)\tERROR: %s\n", __FILE__, \
        __LINE__, strerror(errno));}while(0)

#define FAIL_MSG(x, y, ...) do{if(!(x)){fprintf(stderr,"(%s:%u)\tFAIL: ", \
        __FILE__, __LINE__);fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr);return y;}} while(0)

#define FAIL(x, y) do{if(!(x)){fprintf(stderr, "(%s:%u)\tFAIL: %s\n", __FILE__, \
        __LINE__, strerror(errno)); return y;}}while(0)

#define FATAL_MSG(x, y, ...) do{if(!(x)){fprintf(stderr,"(%s:%u)\tFATAL: ", \
        __FILE__, __LINE__);fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr);exit(y);}} while(0)

#define FATAL(x, y) do{if(!(x)){fprintf(stderr, "(%s:%u)\tFATAL: %s\n", \
        __FILE__, __LINE__, strerror(errno)); exit(y);}}while(0)

#ifdef DEBUG
#define DMSG(...) do{fprintf(stderr, "(%s:%u)\tDMSG: ", __FILE__, __LINE__); \
fprintf(stderr, __VA_ARGS__);fputc('\n', stderr);}while(0)
#else
#define DMSG(...) do{}while(0)
#endif

#define SET_BIT(x, y) do {x |= 1 << y;} while(0)
#define CLEAR_BIT(x, y) do {x &= ~(1U << y);} while(0)
#define TOGGLE_BIT(x, y) do {x ^= 1 << y;} while(0)
#define GET_BIT(x, y) ((x & (1 << y)) >> y)

#endif /* __UTILS_H__ */
