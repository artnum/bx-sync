#ifndef BX_OBJECT_VALUE_H__
#define BX_OBJECT_VALUE_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BX_OBJECT_TYPE_INTEGER 1
#define BX_OBJECT_TYPE_UINTEGER 2
#define BX_OBJECT_TYPE_FLOAT 3
#define BX_OBJECT_TYPE_STRING 4
#define BX_OBJECT_TYPE_BOOL 5
#define BX_OBJECT_TYPE_BYTES 6
#define BX_OBJECT_TYPE_UUID 7

typedef uint8_t BXGeneric;

typedef struct s_BXInteger BXInteger;
struct s_BXInteger {
  BXGeneric type;
  bool isset;
  int64_t value;
};

typedef struct s_BXUInteger BXUInteger;
struct s_BXUInteger {
  BXGeneric type;
  bool isset;
  uint64_t value;
};

typedef struct s_BXFloat BXFloat;
struct s_BXFloat {
  BXGeneric type;
  bool isset;
  double value;
};

typedef struct s_BXString BXString;
struct s_BXString {
  BXGeneric type;
  bool isset;
  char *value;
  size_t value_len;
};

typedef struct s_BXBool BXBool;
struct s_BXBool {
  BXGeneric type;
  bool isset;
  bool value;
};

typedef struct s_BXBytes BXBytes;
struct s_BXBytes {
  BXGeneric type;
  bool isset;
  uint8_t *value;
  size_t value_len;
};

typedef struct s_BXUuid BXUuid;
struct s_BXUuid {
  BXGeneric type;
  bool isset;
  uint64_t value[2];
};

char *bx_object_value_to_string(BXGeneric *value);
uint64_t bx_object_value_to_index(BXGeneric *value);

#define BX_KEY_MAX_LEN 32
inline static void _bx_dump_key(const char *key, int level) {
  int sp = BX_KEY_MAX_LEN - strlen(key) - ((level - 1) * 2);
  printf("\033[0m");
  for (int i = 0; i < level; i++) {
    printf("  ");
  }
  printf("\e[1;37m%s\e[0m", key);
  for (int i = 0; i < sp; i++) {
    printf(" ");
  }
}

inline static void _bx_dump_integer(const char *key, BXInteger integer,
                                    int level) {
  _bx_dump_key(key, level);
  if (!integer.isset) {
    printf("\e[0;35m[NOT SET]\e[0m\n");
  } else {
    printf("\e[0;32m%lo\e[0m\n", integer.value);
  }
}

inline static void _bx_dump_float(const char *key, BXFloat fl, int level) {
  _bx_dump_key(key, level);
  if (!fl.isset) {
    printf("\e[0;35m[NOT SET]\e[0m\n");
  } else {
    printf("\e[0;34m%.4f\e[0m\n", fl.value);
  }
}

inline static void _bx_dump_bool(const char *key, BXBool bl, int level) {
  _bx_dump_key(key, level);
  if (!bl.isset) {
    printf("\e[0;35m[NOT SET]\e[0m\n");
  } else {
    printf("\e[0;33m%s\e[0m\n", bl.value ? "true" : "false");
  }
}

inline static void _bx_dump_string(const char *key, BXString string,
                                   int level) {
  _bx_dump_key(key, level);
  if (!string.isset || string.value == NULL) {
    printf("\e[0;35m[NOT SET]\e[0m\n");
  } else {
    printf("[%03ld] \e[0;31m%s\e[0m\n", string.value_len, string.value);
  }
}

inline static void _bx_dump_print_title(const char *titlefmt, ...) {
  va_list ap;
  va_start(ap, titlefmt);
  printf("\e[1;30m\e[0;47m  ");
  vprintf(titlefmt, ap);
  printf("  \e[0m\n");
  va_end(ap);
}

inline static void _bx_dump_print_subtitle(const char *titlefmt, ...) {
  va_list ap;
  va_start(ap, titlefmt);
  printf("\e[4;35m  ");
  vprintf(titlefmt, ap);
  printf("  \e[0m\n");
  va_end(ap);
}

inline static void _bx_dump_any(const char *key, const void *value, int level) {
  switch (*(uint8_t *)value) {
  case BX_OBJECT_TYPE_INTEGER:
    _bx_dump_integer(key, *(BXInteger *)value, level);
    break;
  case BX_OBJECT_TYPE_FLOAT:
    _bx_dump_float(key, *(BXFloat *)value, level);
    break;
  case BX_OBJECT_TYPE_STRING:
    _bx_dump_string(key, *(BXString *)value, level);
    break;
  case BX_OBJECT_TYPE_BOOL:
    _bx_dump_bool(key, *(BXBool *)value, level);
    break;
  }
}

#endif /* BX_OBJECT_VALUE_H__ */
