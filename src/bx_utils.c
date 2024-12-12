#include <bx_net.h>
#include <bx_object_value.h>
#include <bx_decode.h>
#include <jansson.h>
#include <stdarg.h>
#include <stdio.h>
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
#include <ncurses.h>

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

    if (str == NULL) { return NULL; }
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

static inline char * _bx_item_to_path (const char * fmt, va_list ap)
{
    assert(fmt != NULL);
    size_t fmt_len = strlen(fmt);
    assert(fmt_len > 0);

    size_t total_len = 0;
    size_t origin = 0;
    size_t i = 0;
    char * path = NULL;
    for (i = 0; i < fmt_len; i++) {
        if (fmt[i] == '$') {
            BXGeneric * item = va_arg(ap, BXGeneric *);
            if (item == NULL) {
                break;
            }
            char * str = bx_object_value_to_string(item);
            if (str == NULL) {
                break;
            }
            size_t str_len = strlen(str);
            if (str_len == 0) {
                free(str);
                break;
            }
            total_len = i - origin + str_len + 1;
            void * tmp = realloc(path, total_len);
            if (tmp == NULL) {
                free(str);
                break;
            }
            path = tmp;
            if (origin == 0) { *path = '\0'; }
            strncat(path, &fmt[origin], i - origin);
            strncat(path, str, str_len);
            free(str);
            origin = i + 1;
        }
    }
    va_end(ap);

    if (i != origin) {
        total_len = i - origin + 1;
        void * tmp = realloc(path, total_len);
        if (tmp == NULL) {
            return path;
        }
        path = tmp;
        if (origin == 0) { *path = '\0'; }
        strncat(path, &fmt[origin], i - origin);
    }

    return path;
}

char * bx_item_to_path (const char * fmt, ...) 
{
    va_list ap;
    va_start(ap, fmt);
    return _bx_item_to_path(fmt, ap);
}

BXNetRequest * bx_do_request(
    BXNetRequestList * queue,
    json_t * body,
    char * path_fmt,
    ...
) 
{
    BXNetRequest * request = NULL;
    assert(path_fmt != NULL);
    assert(queue != NULL);

    va_list ap;
    va_start(ap, path_fmt);

    char * path = _bx_item_to_path(path_fmt, ap);
    if (path == NULL) {
        return NULL;
    }
    
    bx_log_debug("path %s", path);
    va_end(ap);
    request = bx_net_request_new(path, NULL);
    free(path);
    if (request == NULL) {
        return NULL;
    }

    uint64_t request_id = bx_net_request_list_add(queue, request);
    if (request_id == 0) {
        bx_net_request_free(request);
        return NULL;
    }
    while(atomic_load(&request->done) == false) {
        if (atomic_load(&request->cancel) == true) {
            return NULL;
        }
        usleep(100);
    }

    request = bx_net_request_list_get_finished(queue, request_id);
    if (request == NULL) {
        return NULL;
    }
    json_t * json = bx_decode_net(request);
    if (json == NULL) {
        bx_net_request_free(request);
        return NULL;
    }
    request->decoded = json;
    return request;
}

BXMutex io_mutex;
extern WINDOW *LOG_WINDOW;
FILE * fp;

void bx_log_init()
{
    bx_mutex_init(&io_mutex);
    fp = fopen("/tmp/bxnet.log", "a");
}

void _bx_log_debug(char * file, int line, const char *fmt, ...)
{
    char x[255];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(x, 255, fmt, ap);
    va_end(ap);
    assert(bx_mutex_lock(&io_mutex) != false);
    fprintf(fp, "[%s:%d] %s\n", file, line, x);
    wprintw(LOG_WINDOW, "[%s:%d] %s\n", file, line, x);
    wrefresh(LOG_WINDOW);
    bx_mutex_unlock(&io_mutex);
}

void _bx_log_info(char * file, int line, const char *fmt, ...)
{
    char x[255];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(x, 255, fmt, ap);
    va_end(ap);
    assert(bx_mutex_lock(&io_mutex) != false);
    fprintf(fp, "[%s:%d] %s\n", file, line, x);
    wprintw(LOG_WINDOW, "[%s:%d] %s\n", file, line, x);
    wrefresh(LOG_WINDOW);
    bx_mutex_unlock(&io_mutex);
}

void _bx_log_error(char * file, int line, const char *fmt, ...)
{
    char x[255];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(x, 255, fmt, ap);
    va_end(ap);
    assert(bx_mutex_lock(&io_mutex) != false);
    fprintf(fp, "[%s:%d] %s\n", file, line, x);
    wprintw(LOG_WINDOW, "[%s:%d] %s\n", file, line, x);
    wrefresh(LOG_WINDOW);
    bx_mutex_unlock(&io_mutex);
}

void bx_log_end() {
    fclose(fp);
}

bool bx_string_compare(const char * str1, const char * str2, size_t max)
{
    for (size_t i = 0; i < max && *str1 != '\0' && *str2 != '\0'; i++) {
        if (*str1 != *str2) { return false; }
        str1++;
        str2++;
    }
    return true;
}