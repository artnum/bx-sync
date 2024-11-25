#ifndef BX_UTILS_H__
#define BX_UTILS_H__

#include <bx_mutex.h>
#include <bx_net.h>

typedef struct {
    uint32_t seed;
    BXMutex mutex;
} BXUtilsPRNGState;

void bx_utils_init(void);
bool bx_utils_gen_id(uint64_t * id);
/**
 * \brief Convert a serie of int into an int array.
 * A serie of int would be something like "1,2,3,4", "1/2/3/4", ...,
 * anything where numeric value are separated by any number of non-numeric value
 * and return an array of int with the index 0 containing the number of items
 * in the array : {4, 1, 2, 3, 4}.
 * Support up to 64 bits integer.
 *
 * \param str The string to convert.
 * \return The int array, index 0 is the number of items. NULL if no item.
 */
int64_t * bx_int_string_array_to_int_array(const char * str);
char * bx_item_to_path (const char * fmt, ...);
BXNetRequest * bx_do_request(
    BXNetRequestList * queue,
    json_t * body,
    char * path_fmt,
    ...
);
void _bx_log_error(char * file, int line, const char *fmt, ...);
#define bx_log_error(fmt, ...)  _bx_log_error(__FILE__, __LINE__, (fmt), __VA_ARGS__)

bool bx_string_compare(const char * str1, const char * str2, size_t max);

#endif /* BX_UTILS_H__ */