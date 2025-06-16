#include "include/bx_object_value.h"
#include "include/bx_object.h"
#include <assert.h>
#include <xxh3.h>

static inline char *_bx_uint2str(BXUInteger *value) {
  char *str = NULL;
  if (value->isset != false) {
    size_t len = snprintf(NULL, 0, "%lu", value->value);
    str = calloc(len + 1, sizeof(str));
    if (str) {
      snprintf(str, len + 1, "%lu", value->value);
    }
  } else {
    str = calloc(2, sizeof(*str));
    if (str != NULL) {
      str[0] = '0';
      return str;
    }
  }

  return str;
}

static inline char *_bx_int2str(BXInteger *value) {
  char *str = NULL;
  if (value->isset != false) {
    size_t len = snprintf(NULL, 0, "%ld", value->value);
    str = calloc(len + 1, sizeof(str));
    if (str) {
      snprintf(str, len + 1, "%ld", value->value);
    }
  } else {
    str = calloc(2, sizeof(*str));
    if (str != NULL) {
      str[0] = '0';
      return str;
    }
  }

  return str;
}

static inline char *_bx_str2str(BXString *value) {
  char *str = NULL;
  if (value->isset == false) {
    str = calloc(1, sizeof(*str));
  } else {
    str = calloc(value->value_len + 1, sizeof(*str));
    if (str) {
      memcpy(str, value->value, value->value_len);
    }
  }
  return str;
}

static inline char *_bx_bytes2str(BXBytes *value) {
  char *str = NULL;
  if (value->isset == false) {
    str = calloc(1, sizeof(*str));
  } else {
    int j = 0;
    str = calloc((value->value_len * 2) + 1, sizeof(*str));
    if (str) {
      for (int i = 0; i < value->value_len; i++) {
        snprintf(&str[j], 3, "%2x", str[i]);
        j += 2;
      }
    }
  }
  return str;
}

#define TRUE_STR "true"
#define FALSE_STR "false"
static inline char *_bx_bool2str(BXBool *value) {
  char *str = NULL;
  if (value->isset == false) {
    str = calloc(1, sizeof(*str));
  } else {
    str = calloc((value->value ? sizeof(TRUE_STR) : sizeof(FALSE_STR)) + 1,
                 sizeof(*str));
    if (str) {
      memcpy(str, value->value ? TRUE_STR : FALSE_STR,
             value->value ? sizeof(TRUE_STR) : sizeof(FALSE_STR));
    }
  }
  return str;
}

uint64_t bx_object_value_to_index(BXGeneric *value) {
  assert(value != NULL);
  switch (*(uint8_t *)value) {
  case BX_OBJECT_TYPE_INTEGER:
    if (!((BXInteger *)value)->isset) {
      return 0;
    }
    return (uint64_t)((BXInteger *)value)->value;
  case BX_OBJECT_TYPE_UINTEGER:
    if (!((BXUInteger *)value)->isset) {
      return 0;
    }
    return ((BXUInteger *)value)->value;
  case BX_OBJECT_TYPE_BOOL:
    if (!((BXBool *)value)->isset) {
      return 0;
    }
    return (uint64_t)((BXBool *)value)->value;
  case BX_OBJECT_TYPE_FLOAT:
    if (!((BXFloat *)value)->isset) {
      return 0;
    }
    return XXH3_64bits((void *)&((BXFloat *)value)->value, sizeof(double));
  case BX_OBJECT_TYPE_STRING:
    if (!((BXString *)value)->isset) {
      return 0;
    }
    return XXH3_64bits(((BXString *)value)->value,
                       ((BXString *)value)->value_len);
  case BX_OBJECT_TYPE_BYTES:
    if (!((BXBytes *)value)->isset) {
      return 0;
    }
    return XXH3_64bits(((BXBytes *)value)->value,
                       ((BXBytes *)value)->value_len);
  case BX_OBJECT_TYPE_UUID:
    if (!((BXUuid *)value)->isset) {
      return 0;
    }
    return XXH3_64bits((void *)&((BXUuid *)value)->value, sizeof(uint64_t) * 2);
  default:
    return 0;
  }
}

char *bx_object_value_to_string(BXGeneric *value) {
  assert(value != NULL);
  switch (*(uint8_t *)value) {
  case BX_OBJECT_TYPE_UINTEGER:
    return _bx_uint2str((BXUInteger *)value);
  case BX_OBJECT_TYPE_INTEGER:
    return _bx_int2str((BXInteger *)value);
  case BX_OBJECT_TYPE_FLOAT:
    break;
  case BX_OBJECT_TYPE_STRING:
    return _bx_str2str((BXString *)value);
  case BX_OBJECT_TYPE_BOOL:
    return _bx_bool2str((BXBool *)value);
  case BX_OBJECT_TYPE_BYTES:
    return _bx_bytes2str((BXBytes *)value);
  }
  return NULL;
}
