#include "include/bx_utils.h"
#include "include/bx_decode.h"
#include "include/bx_mutex.h"
#include "include/bx_net.h"
#include "include/bx_object_value.h"
#include "include/bxill.h"
#include <assert.h>
#include <fcntl.h>
#include <jansson.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <threads.h>
#include <unistd.h>

#define a 0x5DEECE66D
#define c 0xC

static BXUtilsPRNGState BX_GLOBAL_PRNG_STATE;
static uint16_t _bx_utils_prng(void) {
  uint32_t xi = 0;
  bx_mutex_lock(&BX_GLOBAL_PRNG_STATE.mutex);
  xi = (a * BX_GLOBAL_PRNG_STATE.seed + c) % 0xFFFFFFFF;
  BX_GLOBAL_PRNG_STATE.seed = xi;
  bx_mutex_unlock(&BX_GLOBAL_PRNG_STATE.mutex);
  return (xi >> 16) & 0xFFFF;
}

void bx_utils_init(void) {
  struct timeval now;

  bx_mutex_init(&BX_GLOBAL_PRNG_STATE.mutex);

  bx_mutex_lock(&BX_GLOBAL_PRNG_STATE.mutex);
  gettimeofday(&now, NULL);
  int ufd = open("/dev/urandom", O_RDONLY);
  if (ufd < 0) {
    BX_GLOBAL_PRNG_STATE.seed =
        (now.tv_sec << 17 | (now.tv_usec & 0x1FFFF)) & 0xFFFFFFFF;
  } else {
    if (read(ufd, &BX_GLOBAL_PRNG_STATE.seed,
             sizeof(BX_GLOBAL_PRNG_STATE.seed)) < 0) {
      BX_GLOBAL_PRNG_STATE.seed =
          (now.tv_sec << 17 | (now.tv_usec & 0x1FFFF)) & 0xFFFFFFFF;
    }
    close(ufd);
  }
  bx_mutex_unlock(&BX_GLOBAL_PRNG_STATE.mutex);
}

bool bx_utils_gen_id(uint64_t *id) {
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

static inline void _bx_utils_buffer_into_array(int64_t **array, char *buffer) {
  int64_t n = strtoll(buffer, NULL, 10);
  void *tmp = *array == NULL ? calloc(2, sizeof(**array))
                             : realloc(*array, (**array + 2) * sizeof(**array));
  if (tmp == NULL) {
    return;
  }
  *array = tmp;
  *(*array + ++**array) = n;
}

int64_t *bx_int_string_array_to_int_array(const char *str) {
  char buffer[21]; /* up to 64 bits */
  int64_t *int_array = NULL;

  if (str == NULL) {
    return NULL;
  }
  memset(buffer, 0, 21);
  int j = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    switch (str[i]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (j > 20) {
        break;
      }
      buffer[j++] = str[i];
      break;
    default:
      if (j <= 0) {
        break;
      }
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

static inline char *_bx_item_to_path(const char *fmt, va_list ap) {
  assert(fmt != NULL);
  size_t fmt_len = strlen(fmt);
  assert(fmt_len > 0);

  char *path = NULL;
  size_t path_len =
      0; // Tracks total length of path (excluding null terminator)
  size_t origin = 0;
  size_t i = 0;

  for (i = 0; i < fmt_len; i++) {
    if (fmt[i] == '$') {
      BXGeneric *item = va_arg(ap, BXGeneric *);
      if (item == NULL) {
        free(path);
        va_end(ap);
        return NULL;
      }
      char *str = bx_object_value_to_string(item);
      if (str == NULL) {
        free(path);
        va_end(ap);
        return NULL;
      }
      size_t str_len = strlen(str);
      if (str_len == 0) {
        free(str);
        free(path);
        va_end(ap);
        return NULL;
      }

      // Calculate new length: existing path + new fmt segment + str + null
      // terminator
      size_t new_len = path_len + (i - origin) + str_len;
      char *tmp = realloc(path, new_len + 1); // +1 for null terminator
      if (tmp == NULL) {
        free(str);
        free(path);
        va_end(ap);
        return NULL;
      }
      path = tmp;

      // Copy fmt segment and str directly using memcpy
      if (i > origin) {
        memcpy(path + path_len, &fmt[origin], i - origin);
        path_len += i - origin;
      }
      memcpy(path + path_len, str, str_len);
      path_len += str_len;
      path[path_len] = '\0'; // Null-terminate

      free(str);
      origin = i + 1;
    }
  }

  // Append any remaining fmt segment
  if (i > origin) {
    size_t new_len = path_len + (i - origin);
    char *tmp = realloc(path, new_len + 1); // +1 for null terminator
    if (tmp == NULL) {
      free(path);
      va_end(ap);
      return NULL;
    }
    path = tmp;

    memcpy(path + path_len, &fmt[origin], i - origin);
    path_len += i - origin;
    path[path_len] = '\0'; // Null-terminate
  }

  va_end(ap);
  return path;
}

char *bx_item_to_path(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  return _bx_item_to_path(fmt, ap);
}

BXNetRequest *bx_do_request(BXNetRequestList *queue, json_t *body,
                            char *path_fmt, ...) {
  BXNetRequest *request = NULL;
  assert(path_fmt != NULL);
  assert(queue != NULL);

  va_list ap;
  va_start(ap, path_fmt);

  char *path = _bx_item_to_path(path_fmt, ap);
  if (path == NULL) {
    bx_log_debug("Request parsing faile %s", path_fmt);
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
  request = bx_net_request_list_get_finished(queue, request_id);
  if (request == NULL) {
    return NULL;
  }
  thrd_yield();
  if (request->cancel || request->response == NULL) {
    bx_net_request_free(request);
    return NULL;
  }
  json_t *json = bx_decode_net(request);
  if (json == NULL) {
    bx_net_request_free(request);
    return NULL;
  }
  request->decoded = json;
  return request;
}

struct s_BXLog LOG;

bool bx_log_init(const char *path, int level) {
  LOG.head = NULL;
  LOG.level = level;
  bx_mutex_init(&LOG.mutex);
  LOG.fp = fopen(path, "a");
  if (!LOG.fp) {
    return false;
  }
  return true;
}

void *bx_log_out_thread(void *arg) {
  bXill *app = (bXill *)arg;
  struct s_BXLogMsg *current = NULL;
  while (atomic_load(&app->logthread)) {
    bx_mutex_lock(&LOG.mutex);
    current = LOG.head;
    LOG.head = NULL;
    bx_mutex_unlock(&LOG.mutex);

    struct s_BXLogMsg *reversed = NULL;
    while (current) {
      struct s_BXLogMsg *next = (struct s_BXLogMsg *)current->next;
      current->next = reversed;
      reversed = current;
      current = next;
    }

    current = reversed;
    while (current) {
      struct s_BXLogMsg *next = (struct s_BXLogMsg *)current->next;
      fprintf(LOG.fp, "%s\n", current->msg);
      free(current);
      current = next;
    }
    fflush(LOG.fp);
    current = NULL;
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000000}, NULL);
  }

  bx_mutex_lock(&LOG.mutex);
  current = LOG.head;
  struct s_BXLogMsg *next = NULL;
  while (current) {
    next = (struct s_BXLogMsg *)current->next;
    fprintf(LOG.fp, "%s\n", current->msg);
    free(current);
    current = next;
  }
  fprintf(LOG.fp, "--- END OF LOG THREAD ---\n");
  fflush(LOG.fp);
  bx_mutex_unlock(&LOG.mutex);
  return 0;
}

void _bx_log_queue(char *msg, const char *err, char *file, int line) {
  struct s_BXLogMsg *new = NULL;
  new = malloc(sizeof(*new));
  if (new == NULL) {
    return;
  }
  if (err == NULL) {
    snprintf(new->msg, LOG_MSG_BUFFER_MAX, "%s", msg);
  } else {
    snprintf(new->msg, LOG_MSG_BUFFER_MAX, "%s [%s:%d] %s", err, file, line,
             msg);
  }
  if (bx_mutex_lock(&LOG.mutex) != false) {
    new->next = LOG.head;
    LOG.head = new;
    bx_mutex_unlock(&LOG.mutex);
  } else {
    free(new);
  }
}

void _bx_log_debug(char *file, int line, const char *fmt, ...) {
  if (LOG.level < LOG_LEVEL_DEBUG) {
    return;
  }
  char x[LOG_MSG_BUFFER_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(x, LOG_MSG_BUFFER_MAX, fmt, ap);
  va_end(ap);
  _bx_log_queue(x, "DEBUG", file, line);
}

void _bx_log_info(char *file, int line, const char *fmt, ...) {
  if (LOG.level < LOG_LEVEL_INFO) {
    return;
  }
  char x[LOG_MSG_BUFFER_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(x, LOG_MSG_BUFFER_MAX, fmt, ap);
  va_end(ap);
  _bx_log_queue(x, "INFO", file, line);
}

void _bx_log_error(char *file, int line, const char *fmt, ...) {
  if (LOG.level < LOG_LEVEL_ERROR) {
    return;
  }
  char x[LOG_MSG_BUFFER_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(x, LOG_MSG_BUFFER_MAX, fmt, ap);
  va_end(ap);
  _bx_log_queue(x, "ERROR", file, line);
}

void bx_log_end() { fclose(LOG.fp); }

bool bx_string_compare(const char *str1, const char *str2, size_t max) {
  for (size_t i = 0; i < max && *str1 != '\0' && *str2 != '\0'; i++) {
    if (*str1 != *str2) {
      return false;
    }
    str1++;
    str2++;
  }
  return true;
}

char *bx_utils_cache_filename(bXill *app, const char *filename) {
  const char *cdir = bx_conf_get_string(app->conf, "cache-directory");
  if (cdir == NULL) {
    cdir = BXILL_DEFAULT_CACHE_DIR;
  }

  size_t len = strlen(cdir) + strlen(filename) + 2;
  char *f = calloc(len, sizeof(*filename));
  if (!f) {
    return NULL;
  }
  snprintf(f, len + 1, "%s/%s", cdir, filename);
  bx_conf_release(app->conf, "cache-directory");
  return f;
}

int bx_utils_cache_checkpoint(bXill *app) {
  int cache_checkpoint = bx_conf_get_int(app->conf, "cache-checkpoint");
  if (cache_checkpoint == 0) {
    cache_checkpoint = BXILL_DEFAULT_CACHE_CHECKPOINT;
  }
  return cache_checkpoint;
}
