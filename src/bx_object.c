#include "include/bx_object.h"
#include "include/bx_html_entities.h"
#include "include/bx_object_value.h"

#include <assert.h>
#include <jansson.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

BXBool bx_object_get_json_bool(json_t *object, const char *key,
                               XXH3_state_t *state) {
  json_t *value = NULL;
  BXBool retval = {.type = BX_OBJECT_TYPE_BOOL, .isset = false, .value = false};

  value = json_object_get(object, key);
  if (value == NULL || json_is_null(value)) {
    return retval;
  }

  retval.isset = true;
  if (json_is_string(value)) {
    if (strcasecmp("true", json_string_value(value)) == 0) {
      retval.value = true;
    } else if (strcasecmp("yes", json_string_value(value)) == 0) {
      retval.value = true;
    } else if (strcasecmp("on", json_string_value(value)) == 0) {
      retval.value = true;
    } else {
      retval.value = false;
    }
  } else if (json_is_boolean(value)) {
    if (json_is_false(value)) {
      retval.value = false;
    } else {
      retval.value = true;
    }
  } else if (json_is_integer(value)) {
    if (json_integer_value(value)) {
      retval.value = true;
    } else {
      retval.value = false;
    }
  }
  unsigned char hv = retval.value ? 0xFF : 0x00;
  if (state != NULL) {
    XXH3_64bits_update(state, &hv, sizeof(hv));
  }
  return retval;
}

BXInteger bx_object_get_json_int(json_t *object, const char *key,
                                 XXH3_state_t *state) {
  json_t *value = NULL;
  BXInteger i = {.type = BX_OBJECT_TYPE_INTEGER, .isset = false, .value = 0};
  value = json_object_get(object, key);

  if (value == NULL || json_is_null(value)) {
    return i;
  }

  i.isset = true;
  if (json_is_string(value)) {
    i.value = strtoll(json_string_value(value), NULL, 10);
  } else if (json_is_integer(value)) {
    i.value = json_integer_value(value);
  }
  if (state != NULL) {
    XXH3_64bits_update(state, &i.value, sizeof(i.value));
  }

  return i;
}

BXUInteger bx_object_get_json_uint(json_t *object, const char *key,
                                   XXH3_state_t *state) {
  json_t *value = NULL;
  BXUInteger i = {.type = BX_OBJECT_TYPE_UINTEGER, .isset = false, .value = 0};
  value = json_object_get(object, key);

  if (value == NULL || json_is_null(value)) {
    return i;
  }

  i.isset = true;
  if (json_is_string(value)) {
    i.value = (uint64_t)strtoll(json_string_value(value), NULL, 10);
  } else if (json_is_integer(value)) {
    i.value = (uint64_t)json_integer_value(value);
  }
  if (state != NULL) {
    XXH3_64bits_update(state, &i.value, sizeof(i.value));
  }

  return i;
}

BXFloat bx_object_get_json_double(json_t *object, const char *key,
                                  XXH3_state_t *state) {
  json_t *value = NULL;
  BXFloat d = {.type = BX_OBJECT_TYPE_FLOAT, .isset = false, .value = 0};

  value = json_object_get(object, key);
  if (value == NULL || json_is_null(value)) {
    return d;
  }

  d.isset = true;
  if (json_is_string(value)) {
    d.value = strtod(json_string_value(value), NULL);
  } else if (json_is_real(value)) {
    d.value = json_real_value(value);
  }
  if (state != NULL) {
    XXH3_64bits_update(state, &d.value, sizeof(d.value));
  }
  return d;
}

BXString bx_object_get_json_string(json_t *object, const char *key,
                                   XXH3_state_t *state) {
  json_t *value = NULL;
  BXString str = {.type = BX_OBJECT_TYPE_STRING,
                  .isset = false,
                  .value = NULL,
                  .value_len = 0};
  value = json_object_get(object, key);
  if (value == NULL || json_is_null(value)) {
    return str;
  }
  if (json_is_string(value)) {
    str.value_len = json_string_length(value);
    str.value = calloc(str.value_len + 1, sizeof(*str.value));
    if (str.value == NULL) {
      return str;
    }
    memcpy(str.value, json_string_value(value), str.value_len);
  } else if (json_is_integer(value)) {
    str.value_len = snprintf(NULL, 0, "%lld", json_integer_value(value)) - 1;
    str.value = calloc(str.value_len + 1, sizeof(*str.value));
    if (str.value == NULL) {
      return str;
    }
    snprintf(str.value, str.value_len + 1, "%lld", json_integer_value(value));
  } else if (json_is_real(value)) {
    str.value_len = snprintf(NULL, 0, "%e", json_real_value(value)) - 1;
    str.value = calloc(str.value_len + 1, sizeof(*str.value));
    if (str.value == NULL) {
      return str;
    }
    snprintf(str.value, str.value_len + 1, "%e", json_real_value(value));
  }

  if (str.value != NULL) {
    str.isset = true;
    bx_html_entities_replaces(&str);
    if (state != NULL) {
      XXH3_64bits_update(state, str.value, str.value_len);
    }
  }

  return str;
}
BXUuid bx_object_get_json_uuid(json_t *object, const char *key,
                               XXH3_state_t *state) {
  BXUuid uuid = {.type = BX_OBJECT_TYPE_UUID, .isset = false, .value = {0, 0}};

  // Get the JSON value for the key
  json_t *value = json_object_get(object, key);
  if (value == NULL || json_is_null(value)) {
    return uuid;
  }

  // Ensure the value is a string
  if (!json_is_string(value)) {
    return uuid;
  }

  // Get the string and its length
  const char *uuidstr = json_string_value(value);
  size_t len = json_string_length(value);
  if (len != 36) {
    return uuid;
  }

  // Validate UUID format: 8-4-4-4-12 hex digits with hyphens
  size_t segment_lengths[] = {8, 4, 4, 4, 12};
  size_t segment = 0, pos = 0, hex_count = 0;
  uint8_t bytes[16] = {0}; // Temporary buffer for 128-bit UUID

  for (size_t i = 0; i < len; i++) {
    if (segment < 5 && pos == segment_lengths[segment]) {
      if (uuidstr[i] != '-') {
        return uuid; // Expected hyphen missing
      }
      segment++;
      pos = 0;
      continue;
    }
    char c = tolower(uuidstr[i]);
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return uuid; // Invalid character
    }
    // Convert hex digit to value
    uint8_t nibble = (c >= '0' && c <= '9') ? (c - '0') : (c - 'a' + 10);
    bytes[hex_count / 2] |= nibble << ((1 - (hex_count % 2)) * 4);
    hex_count++;
    pos++;
  }

  // Ensure we processed exactly 32 hex digits
  if (hex_count != 32 || segment != 4 || pos != 12) {
    return uuid;
  }

  // Convert bytes to uint64_t[2]
  for (int i = 0; i < 8; i++) {
    uuid.value[0] = (uuid.value[0] << 8) | bytes[i];
    uuid.value[1] = (uuid.value[1] << 8) | bytes[i + 8];
  }

  if (state != NULL) {
    XXH3_64bits_update(state, &uuid.value, sizeof(uint64_t) * 2);
  }
  uuid.isset = true;
  return uuid;
}

inline static void bx_object_free_string(BXString string) {
  if (string.value != NULL) {
    free(string.value);
  }
  string.value_len = 0;
}
inline static void bx_object_free_bytes(BXBytes bytes) {
  if (bytes.value != NULL) {
    free(bytes.value);
  }
  bytes.value_len = 0;
}

void bx_object_free_value(void *value) {
  if (value == NULL) {
    return;
  }
  uint8_t type = *(uint8_t *)value;
  switch (type) {
  case BX_OBJECT_TYPE_STRING: {
    ((BXString *)value)->isset = false;
    return bx_object_free_string(*(BXString *)value);
  }
  case BX_OBJECT_TYPE_BYTES: {
    ((BXBytes *)value)->isset = false;
    return bx_object_free_bytes(*(BXBytes *)value);
  }
  case BX_OBJECT_TYPE_INTEGER: {
    ((BXInteger *)value)->isset = false;
    break;
  }
  case BX_OBJECT_TYPE_UINTEGER: {
    ((BXUInteger *)value)->isset = false;
    break;
  }
  case BX_OBJECT_TYPE_FLOAT: {
    ((BXFloat *)value)->isset = false;
    break;
  }
  case BX_OBJECT_TYPE_BOOL: {
    ((BXBool *)value)->isset = false;
    break;
  }
  case BX_OBJECT_TYPE_UUID: {
    ((BXUuid *)value)->isset = false;
    ((BXUuid *)value)->value[0] = 0;
    ((BXUuid *)value)->value[1] = 0;
    break;
  }
  }
}
