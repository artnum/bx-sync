#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <assert.h>
#include <bx_mutex.h>
#include <bx_utils.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define a 0x5DEECE66D
#define c 0xC

static BXUtilsPRNGState BX_GLOBAL_PRNG_STATE;
static uint16_t _bx_utils_prng(void)
{
    uint32_t xi = 0;
    bx_mutex_lock(&BX_GLOBAL_PRNG_STATE.mutex);
    xi = (a * BX_GLOBAL_PRNG_STATE.seed + c) % 0xFFFFFFFF;
    BX_GLOBAL_PRNG_STATE.seed = xi;
    bx_mutex_unlock(&BX_GLOBAL_PRNG_STATE.mutex);
    return (xi >> 16) & 0xFFFF;
}

void bx_utils_init(void)
{
    struct timeval now;
    
    bx_mutex_init(&BX_GLOBAL_PRNG_STATE.mutex);
    
    bx_mutex_lock(&BX_GLOBAL_PRNG_STATE.mutex);
    gettimeofday(&now, NULL);
    int ufd = open("/dev/urandom", O_RDONLY);
    if (ufd < 0) {
        BX_GLOBAL_PRNG_STATE.seed = (now.tv_sec << 17 | (now.tv_usec & 0x1FFFF)) & 0xFFFFFFFF;
    } else {
        if(read(ufd, &BX_GLOBAL_PRNG_STATE.seed, sizeof(BX_GLOBAL_PRNG_STATE.seed)) < 0) {
            BX_GLOBAL_PRNG_STATE.seed = (now.tv_sec << 17 | (now.tv_usec & 0x1FFFF)) & 0xFFFFFFFF;
        }
        close(ufd);
    }
    bx_mutex_unlock(&BX_GLOBAL_PRNG_STATE.mutex);
}


bool bx_utils_gen_id(uint64_t * id)
{
    assert(id != NULL);
    struct timeval now;
    *id = 0;

    if (gettimeofday(&now, NULL) != 0) {
        return false;
    }

    *id |= (uint64_t)(now.tv_sec & 0xFFFFFFFF) << 32;
    *id |= (uint64_t)(now.tv_usec >> 4) << 16;
    *id |= _bx_utils_prng();

    return true;
}

static inline void _bx_utils_buffer_into_array(int64_t ** array, char * buffer) 
{
    int64_t n = strtoll(buffer, NULL, 10);
    void *tmp = *array == NULL ? 
            calloc(2, sizeof(**array)) 
            : realloc(*array, (**array + 2) * sizeof(**array));
    if (tmp == NULL) {
        return;
    }
    *array = tmp;
    *(*array + ++**array) = n;
}

int64_t * bx_int_string_array_to_int_array(const char * str)
{
    char buffer[21]; /* up to 64 bits */
    int64_t * int_array = NULL;

    memset(buffer, 0, 21);
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        switch(str[i]) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if (j > 20) {
                    break;
                }
                buffer[j++] = str[i];
                break;
            default:
                if (j <= 0) { break; }
                _bx_utils_buffer_into_array(&int_array, buffer);
                memset(buffer, 0, 21);
                j = 0;
                break;
        }
    }
    if (j > 0) {
        _bx_utils_buffer_into_array(&int_array, buffer);
    }
    return int_array;
}