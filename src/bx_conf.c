#include "include/bx_conf.h"
#include "include/bx_mutex.h"

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xxh3.h>

inline static BXConfValue *_bx_conf_get(BXConf *conf, const char *key) {
  size_t slen = strlen(key);
  assert(slen > 0);
  uint64_t ikey = XXH3_64bits(key, slen);

  BXConfValue *current = conf->head;
  while (current != NULL) {
    if (current->key == ikey) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

bool bx_conf_loadfile(BXConf *conf, const char *filepath) {
  assert(conf != NULL);
  assert(filepath != NULL);

  FILE *fp = NULL;
  fp = fopen(filepath, "r");
  if (fp == NULL) {
    return false;
  }

  json_error_t error;
  json_t *jroot = json_loadf(fp, 0, &error);
  fclose(fp);
  if (jroot == NULL) {
    return false;
  }

  if (!json_is_object(jroot)) {
    json_decref(jroot);
    return false;
  }

  const char *key;
  json_t *value;

  json_object_foreach(jroot, key, value) {
    int i_value = 0;
    double f_value = 0.0;
    const char *s_value = NULL;
    if (json_is_string(value)) {
      s_value = json_string_value(value);
      bx_conf_set(conf, key, (void *)s_value, StringType);
    } else if (json_is_integer(value)) {
      i_value = json_integer_value(value);
      bx_conf_set(conf, key, (void *)&i_value, IntegerType);
    } else if (json_is_real(value)) {
      f_value = json_real_value(value);
      bx_conf_set(conf, key, (void *)&f_value, FloatType);
    } else if (json_is_boolean(value)) {
      i_value = json_is_true(value) ? 1 : 0;
      bx_conf_set(conf, key, (void *)&i_value, BoolType);
    } else {
      continue;
    }
  }
  json_decref(jroot);

  return true;
}

BXConf *bx_conf_init() {
  BXConf *conf = NULL;

  conf = calloc(1, sizeof(*conf));
  if (conf == NULL) {
    return NULL;
  }

  conf->head = NULL;
  return conf;
}

void bx_conf_dump(BXConf *conf) {
  assert(conf != NULL);

  printf("=== BXCONF ===\n");
  BXConfValue *current = conf->head;
  while (current) {
    printf("\t0x%lx -> ", current->key);
    switch (current->type) {
    case StringType:
      printf(">%s<", current->s_value);
      break;
    case BoolType:
      printf("%s", current->i_value ? "TRUE" : "FALSE");
      break;
    case FloatType:
      printf("%e", current->f_value);
      break;
    case IntegerType:
      printf("%d", current->i_value);
      break;
    case ByteType:
      printf("%c", (char)current->i_value & 0xFF);
      break;
    }
    printf("\n");
    current = current->next;
  }
}

void bx_conf_destroy(BXConf **conf) {
  assert(conf != NULL);
  assert(*conf != NULL);

  BXConf *c = *conf;
  BXConfValue *current = c->head;
  while (current != NULL) {
    BXConfValue *next = current->next;

    if (current->s_value != NULL) {
      free(current->s_value);
    }
    free(current);
    current = next;
  }
  free(c);
  *conf = NULL;
}

bool bx_conf_set(BXConf *conf, const char *key, void *value,
                 enum e_BXConfValueType type) {
  assert(conf != NULL);
  assert(key != NULL);
  assert(value != NULL);
  size_t slen = 0;
  slen = strlen(key);
  assert(slen > 0);

  BXConfValue *v = NULL;
  v = calloc(1, sizeof(*v));
  if (v == NULL) {
    return false;
  }
  v->f_value = 0.0;
  v->s_value = NULL;
  v->i_value = 0;
  v->next = NULL;
  v->key = XXH3_64bits(key, slen);
  v->type = type;
  slen = 0;
  switch (type) {
  case BoolType:
    v->i_value = *(bool *)value ? 1 : 0;
    break;
  case IntegerType:
    v->i_value = *(int *)value;
    break;
  case StringType:
    slen = strlen((char *)value);
    if (slen <= 0) {
      free(v);
      return false;
    }
    v->s_value = calloc(slen + 1, sizeof(*v->s_value));
    if (v->s_value == NULL) {
      free(v);
      return false;
    }
    memcpy(v->s_value, (char *)value, slen);
    break;
  case FloatType:
    v->f_value = *(double *)value;
    break;
  case ByteType:
    v->i_value = *(unsigned char *)value;
    break;
  }

  if (conf->head == NULL) {
    conf->head = v;
  } else {
    BXConfValue *current;
    current = conf->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = v;
  }
  return true;
}

void bx_conf_release(BXConf *conf, const char *key) { return; }

const char *bx_conf_get_string(BXConf *conf, const char *key) {
  assert(conf != NULL);
  assert(key != NULL);

  BXConfValue *v = NULL;
  v = _bx_conf_get(conf, key);
  if (v == NULL || v->type != StringType) {
    return NULL;
  }

  return v->s_value;
}

int bx_conf_get_int(BXConf *conf, const char *key) {
  assert(conf != NULL);
  assert(key != NULL);

  BXConfValue *v = NULL;
  v = _bx_conf_get(conf, key);
  if (v == NULL || v->type != IntegerType) {
    return 0;
  }
  return v->i_value;
}
