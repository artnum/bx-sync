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

const BXGeneric *bx_any_to_generic(BXAny *a) {
  if (a == NULL) {
    return NULL;
  }
  switch (*(uint8_t *)a) {
  case BX_OBJECT_TYPE_BYTES:
    return (BXGeneric *)&a->__bytes;
  case BX_OBJECT_TYPE_STRING:
    return (BXGeneric *)&a->__string;
  case BX_OBJECT_TYPE_FLOAT:
    return (BXGeneric *)&a->__float;
  case BX_OBJECT_TYPE_BOOL:
    return (BXGeneric *)&a->__bool;
  case BX_OBJECT_TYPE_INTEGER:
    return (BXGeneric *)&a->__int;
  case BX_OBJECT_TYPE_UUID:
    return (BXGeneric *)&a->__uuid;
  case BX_OBJECT_TYPE_UINTEGER:
    return (BXGeneric *)&a->__uint;
  }
  return NULL;
}

char *bx_any_to_str(BXAny *a) {
  char *a_str = NULL;
  if (a == NULL) {
    return NULL;
  }
  switch (*(uint8_t *)a) {
  case BX_OBJECT_TYPE_BYTES:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__bytes);
    break;
  case BX_OBJECT_TYPE_STRING:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__string);
    break;
  case BX_OBJECT_TYPE_FLOAT:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__float);
    break;
  case BX_OBJECT_TYPE_BOOL:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__bool);
    break;
  case BX_OBJECT_TYPE_INTEGER:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__int);
    break;
  case BX_OBJECT_TYPE_UUID:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__uuid);
    break;
  case BX_OBJECT_TYPE_UINTEGER:
    a_str = bx_object_value_to_string((BXGeneric *)&a->__uint);
    break;
  }
  return a_str;
}

int bx_object_value_compare(BXAny *a, BXGeneric *b) {
  assert(a != NULL);
  assert(b != NULL);

  uint8_t a_type = *(uint8_t *)a;
  uint8_t b_type = *(uint8_t *)b;
  int retVal = 0;

  if (a_type != b_type) {
    char *a_str = bx_any_to_str(a);
    char *b_str = bx_object_value_to_string(b);
    if (b_str == NULL || a_str == NULL) {
      if (b_str != NULL) {
        free(b_str);
      }
      if (a_str != NULL) {
        free(a_str);
      }
      return 0;
    }
    retVal = strcmp(a_str, b_str);
    if (a_str) {
      free(a_str);
    }
    if (b_str) {
      free(b_str);
    }
    return retVal;
  }

  switch (a_type) {
  case BX_OBJECT_TYPE_BOOL: {
    BXBool *a_val = (BXBool *)&a->__bool;
    BXBool *b_val = (BXBool *)b;
    if (a_val->value < b_val->value) {
      return -1;
    }
    if (a_val->value > b_val->value) {
      return 1;
    }
    return 0;
  }

  case BX_OBJECT_TYPE_INTEGER: {
    BXInteger *a_val = (BXInteger *)&a->__int;
    BXInteger *b_val = (BXInteger *)b;
    if (a_val->value < b_val->value) {
      return -1;
    }
    if (a_val->value > b_val->value) {
      return 1;
    }
    return 0;
  }
  case BX_OBJECT_TYPE_UINTEGER: {
    BXUInteger *a_val = (BXUInteger *)&a->__uint;
    BXUInteger *b_val = (BXUInteger *)b;
    if (a_val->value < b_val->value) {
      return -1;
    }
    if (a_val->value > b_val->value) {
      return 1;
    }
    return 0;
  }
  case BX_OBJECT_TYPE_FLOAT: {
    BXFloat *a_val = (BXFloat *)&a->__float;
    BXFloat *b_val = (BXFloat *)b;
    if (a_val->value < b_val->value) {
      return -1;
    }
    if (a_val->value > b_val->value) {
      return 1;
    }
    return 0;
  }
  case BX_OBJECT_TYPE_UUID: {
    BXUuid *a_val = (BXUuid *)&a->__uuid;
    BXUuid *b_val = (BXUuid *)b;
    if (a_val->value[0] < b_val->value[0]) {
      return -1;
    }
    if (a_val->value[0] > b_val->value[0]) {
      return 1;
    }
    if (a_val->value[1] < b_val->value[1]) {
      return -1;
    }
    if (a_val->value[1] > b_val->value[1]) {
      return 1;
    }
    return 0;
  }
  case BX_OBJECT_TYPE_STRING: {
    BXString *a_val = (BXString *)&a->__string;
    BXString *b_val = (BXString *)b;
    if (a_val->value_len == b_val->value_len) {
      return strncmp(a_val->value, b_val->value, a_val->value_len);
    }
    if (a_val->value_len < b_val->value_len) {
      return -1;
    }
    if (a_val->value_len > b_val->value_len) {
      return 1;
    }
  }
  case BX_OBJECT_TYPE_BYTES: {
    BXBytes *a_val = (BXBytes *)&a->__bytes;
    BXBytes *b_val = (BXBytes *)b;
    if (a_val->value_len == b_val->value_len) {
      for (size_t i = 0; i < a_val->value_len; i++) {
        if (a_val->value[i] < b_val->value[i]) {
          return -1;
        }
        if (a_val->value[i] > b_val->value[i]) {
          return 1;
        }
      }
      return 0;
    }
    if (a_val->value_len < b_val->value_len) {
      return -1;
    }
    if (a_val->value_len > b_val->value_len) {
      return 1;
    }
  }
  }

  return 0;
}

bool bx_object_value_copy(BXAny *dest, BXGeneric *src) {
  assert(dest != NULL);
  assert(src != NULL);
  *(uint8_t *)dest = *(uint8_t *)src;
  switch (*(uint8_t *)src) {
  case BX_OBJECT_TYPE_INTEGER:
    memcpy(&dest->__int, src, sizeof(BXInteger));
    return true;
  case BX_OBJECT_TYPE_UINTEGER:
    memcpy(&dest->__uint, src, sizeof(BXUInteger));
    return true;
  case BX_OBJECT_TYPE_UUID:
    memcpy(&dest->__uuid, src, sizeof(BXUuid));
    return true;
  case BX_OBJECT_TYPE_FLOAT:
    memcpy(&dest->__float, src, sizeof(BXFloat));
    return true;
  case BX_OBJECT_TYPE_BOOL:
    memcpy(&dest->__bool, src, sizeof(BXBool));
    return true;
  case BX_OBJECT_TYPE_BYTES: {
    memcpy(&dest->__bytes, src, sizeof(BXBytes));
    BXBytes *d = (BXBytes *)&dest->__bytes;
    d->value = calloc(d->value_len, sizeof(d->value));
    if (!d->value) {
      return false;
    }
    memcpy(d->value, ((BXBytes *)src)->value, d->value_len);
    return true;
  }
  case BX_OBJECT_TYPE_STRING: {
    memcpy(&dest->__string, src, sizeof(BXString));
    BXString *d = (BXString *)&dest->__string;
    d->value = calloc(d->value_len, sizeof(d->value));
    if (!d->value) {
      return false;
    }
    memcpy(d->value, ((BXString *)src)->value, d->value_len);
    return true;
  }
  }
  return false;
}
