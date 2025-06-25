#ifndef BX_UTILS_H__
#define BX_UTILS_H__

#include "bx_mutex.h"
#include "bx_net.h"
#include "bxill.h"
#include <stdio.h>

typedef struct {
  uint32_t seed;
  BXMutex mutex;
} BXUtilsPRNGState;

#define LOG_MSG_BUFFER_MAX 500
struct s_BXLogMsg {
  char msg[LOG_MSG_BUFFER_MAX];
  void *next;
};

#define LOG_LEVEL_DEBUG 0x80
#define LOG_LEVEL_INFO 0x08
#define LOG_LEVEL_ERROR 0x01

struct s_BXLog {
  FILE *fp;
  uint8_t level;
  BXMutex mutex;
  struct s_BXLogMsg *head;
};

bool bx_log_init(const char *path, int level);
void bx_utils_init(void);
bool bx_utils_gen_id(uint64_t *id);
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
int64_t *bx_int_string_array_to_int_array(const char *str);
char *bx_item_to_path(const char *fmt, ...);
BXNetRequest *bx_do_request(BXNetRequestList *queue, json_t *body,
                            char *path_fmt, ...);

/* Logging functions */
void _bx_log_error(char *file, int line, const char *fmt, ...);
void _bx_log_info(char *file, int line, const char *fmt, ...);
void _bx_log_debug(char *file, int line, const char *fmt, ...);
#define bx_log_error(fmt, ...)                                                 \
  _bx_log_error(__FILE__, __LINE__, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define bx_log_info(fmt, ...)                                                  \
  _bx_log_info(__FILE__, __LINE__, (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define bx_log_debug(fmt, ...)                                                 \
  _bx_log_debug(__FILE__, __LINE__, (fmt)__VA_OPT__(, ) __VA_ARGS__)
void bx_log_end();
bool bx_string_compare(const char *str1, const char *str2, size_t max);
void *bx_log_out_thread(void *arg);

char *bx_utils_cache_filename(bXill *app, const char *filename);
int bx_utils_cache_checkpoint(bXill *app);
#endif /* BX_UTILS_H__ */
