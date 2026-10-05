#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void SWO_Init(void);
void debug(const char *fmt, ...);
size_t int64_to_string(int64_t value, char *buffer, size_t buffer_size);
size_t uint64_to_string(uint64_t value, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_H
