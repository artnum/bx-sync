#ifndef BX_UTILS_H__
#define BX_UTILS_H__

#include <bx_mutex.h>

typedef struct {
    uint32_t seed;
    BXMutex mutex;
} BXUtilsPRNGState;

void bx_utils_init(void);
bool bx_utils_gen_id(uint64_t * id);

#endif /* BX_UTILS_H__ */