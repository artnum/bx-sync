#include "include/bx_conf.h"
#include "include/bx_mutex.h"

#include <assert.h>
#include <jansson.h>
#include <stdbool.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <xxh3.h>

static const char BX_CONF_TABLE_SQL[] =
    "CREATE TABLE IF NOT EXISTS _configuration ("
    " id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
    " name VARCHAR(191) NOT NULL,"
    " type VARCHAR(32) NOT NULL,"
    " ival INTEGER NULL,"
    " cval TEXT NULL,"
    " bval BLOB NULL,"
    " fval DOUBLE NULL,"
    " PRIMARY KEY (id),"
    " UNIQUE KEY configuration_name (name)"
    ")";

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
    case BlobType:
      printf("blob(%zu)", current->b_len);
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
    if (current->b_value != NULL) {
      free(current->b_value);
    }
    free(current);
    current = next;
  }
  free(c);
  *conf = NULL;
}

static void clear_value(BXConfValue *v) {
  free(v->s_value);
  free(v->b_value);
  v->s_value = NULL;
  v->b_value = NULL;
  v->b_len = 0;
  v->i_value = 0;
  v->f_value = 0.0;
}

static bool fill_value(BXConfValue *v, void *value, enum e_BXConfValueType type) {
  size_t slen = 0;

  clear_value(v);
  v->type = type;
  switch (type) {
  case BoolType:
    v->i_value = *(bool *)value ? 1 : 0;
    break;
  case IntegerType:
    v->i_value = *(int *)value;
    break;
  case StringType:
    slen = strlen((char *)value);
    v->s_value = calloc(slen + 1, sizeof(*v->s_value));
    if (v->s_value == NULL) {
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
  case BlobType: {
    BXConfBlob *blob = value;
    if (blob->data == NULL || blob->len == 0) {
      break;
    }
    v->b_value = malloc(blob->len);
    if (v->b_value == NULL) {
      return false;
    }
    memcpy(v->b_value, blob->data, blob->len);
    v->b_len = blob->len;
    break;
  }
  }
  return true;
}

bool bx_conf_set(BXConf *conf, const char *key, void *value,
                 enum e_BXConfValueType type) {
  assert(conf != NULL);
  assert(key != NULL);
  assert(value != NULL);
  size_t slen = strlen(key);
  assert(slen > 0);

  BXConfValue *v = _bx_conf_get(conf, key);
  bool existing = v != NULL;
  if (!existing) {
    v = calloc(1, sizeof(*v));
    if (v == NULL) {
      return false;
    }
    v->key = XXH3_64bits(key, slen);
  }
  if (!fill_value(v, value, type)) {
    if (!existing) {
      free(v);
    }
    return false;
  }
  if (existing) {
    return true;
  }
  if (conf->head == NULL) {
    conf->head = v;
  } else {
    BXConfValue *current = conf->head;
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
  if (v == NULL) {
    return 0;
  }
  if (v->type != IntegerType && v->type != BoolType) {
    return 0;
  }
  return v->i_value;
}

static bool type_is(const char *type, const char *a, const char *b) {
  if (type == NULL) {
    return false;
  }
  if (strcasecmp(type, a) == 0) {
    return true;
  }
  return b != NULL && strcasecmp(type, b) == 0;
}

static bool apply_database_row(BXConf *conf, const char *name, const char *type,
                               const char *ival, const char *cval,
                               const char *bval, unsigned long bval_len,
                               const char *fval) {
  int i_value = 0;
  double f_value = 0.0;
  bool b_value = false;
  BXConfBlob blob;
  char *end = NULL;
  long parsed = 0;

  if (name == NULL || name[0] == '\0' || strncmp(name, "mysql-", 6) == 0) {
    return true;
  }
  if (type_is(type, "string", "text") || type_is(type, "char", "varchar")) {
    if (cval == NULL) {
      fprintf(stderr, "configuration: %s has no text value\n", name);
      return false;
    }
    return bx_conf_set(conf, name, (void *)cval, StringType);
  }
  if (type_is(type, "integer", "int")) {
    if (ival == NULL) {
      fprintf(stderr, "configuration: %s has no integer value\n", name);
      return false;
    }
    parsed = strtol(ival, &end, 10);
    if (end == ival) {
      fprintf(stderr, "configuration: %s is not an integer\n", name);
      return false;
    }
    i_value = (int)parsed;
    return bx_conf_set(conf, name, &i_value, IntegerType);
  }
  if (type_is(type, "bool", "boolean")) {
    if (ival == NULL) {
      fprintf(stderr, "configuration: %s has no boolean value\n", name);
      return false;
    }
    b_value = strtol(ival, NULL, 10) != 0;
    return bx_conf_set(conf, name, &b_value, BoolType);
  }
  if (type_is(type, "float", "double") || type_is(type, "real", NULL)) {
    if (fval == NULL) {
      fprintf(stderr, "configuration: %s has no float value\n", name);
      return false;
    }
    f_value = strtod(fval, &end);
    if (end == fval) {
      fprintf(stderr, "configuration: %s is not a float\n", name);
      return false;
    }
    return bx_conf_set(conf, name, &f_value, FloatType);
  }
  if (type_is(type, "blob", "byte") || type_is(type, "bytes", "binary")) {
    blob.data = bval;
    blob.len = bval == NULL ? 0 : bval_len;
    return bx_conf_set(conf, name, &blob, BlobType);
  }
  fprintf(stderr, "configuration: %s has unknown type '%s'\n", name,
          type == NULL ? "" : type);
  return false;
}

static MYSQL *conf_connect(BXConf *conf) {
  const char *host = NULL;
  const char *user = NULL;
  const char *password = NULL;
  const char *database = NULL;
  MYSQL *conn = NULL;
  unsigned int timeout = 5;

  host = bx_conf_get_string(conf, "mysql-host");
  user = bx_conf_get_string(conf, "mysql-user");
  password = bx_conf_get_string(conf, "mysql-password");
  database = bx_conf_get_string(conf, "mysql-database");
  if (host == NULL || user == NULL || database == NULL) {
    fprintf(stderr, "configuration: mysql-host, mysql-user and mysql-database "
                    "are required in the file\n");
    return NULL;
  }
  conn = mysql_init(NULL);
  if (conn == NULL) {
    fprintf(stderr, "configuration: mysql_init failed\n");
    return NULL;
  }
  mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
  if (!mysql_real_connect(conn, host, user, password, database, 0, NULL, 0)) {
    fprintf(stderr, "configuration: %s\n", mysql_error(conn));
    mysql_close(conn);
    return NULL;
  }
  mysql_set_character_set(conn, "utf8mb4");
  if (mysql_query(conn, BX_CONF_TABLE_SQL) != 0) {
    fprintf(stderr, "configuration: %s\n", mysql_error(conn));
    mysql_close(conn);
    return NULL;
  }
  return conn;
}

bool bx_conf_load_database(BXConf *conf) {
  MYSQL *conn = NULL;
  MYSQL_RES *result = NULL;
  MYSQL_ROW row;

  assert(conf != NULL);
  conn = conf_connect(conf);
  if (conn == NULL) {
    return false;
  }
  if (mysql_query(conn, "SELECT name, type, ival, cval, bval, fval "
                        "FROM _configuration ORDER BY id") != 0) {
    fprintf(stderr, "configuration: %s\n", mysql_error(conn));
    mysql_close(conn);
    return false;
  }
  result = mysql_store_result(conn);
  if (result == NULL) {
    fprintf(stderr, "configuration: %s\n", mysql_error(conn));
    mysql_close(conn);
    return false;
  }
  while ((row = mysql_fetch_row(result)) != NULL) {
    unsigned long *lengths = mysql_fetch_lengths(result);
    unsigned long bval_len = 0;
    if (lengths != NULL) {
      bval_len = lengths[4];
    }
    apply_database_row(conf, row[0], row[1], row[2], row[3], row[4], bval_len,
                       row[5]);
  }
  mysql_free_result(result);
  mysql_close(conn);
  return true;
}

bool bx_conf_put_database(BXConf *conf, const char *name,
                          enum e_BXConfValueType type, void *value) {
  static const char *sql =
      "INSERT INTO _configuration (name, type, ival, cval) "
      "VALUES (?, ?, ?, ?) "
      "ON DUPLICATE KEY UPDATE type = VALUES(type), ival = VALUES(ival), "
      "cval = VALUES(cval), bval = NULL, fval = NULL";
  MYSQL *conn = NULL;
  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND bind[4];
  const char *type_name = NULL;
  char *cval = NULL;
  unsigned long name_len = 0;
  unsigned long type_len = 0;
  unsigned long cval_len = 0;
  int ival = 0;
  my_bool ival_null = 1;
  my_bool cval_null = 1;
  bool ok = false;

  assert(conf != NULL);
  assert(name != NULL);
  assert(value != NULL);
  if (strncmp(name, "mysql-", 6) == 0) {
    fprintf(stderr, "configuration: %s stays in the JSON file\n", name);
    return false;
  }
  if (type == StringType) {
    type_name = "string";
    cval = value;
    cval_len = strlen(cval);
    cval_null = 0;
  } else if (type == IntegerType) {
    type_name = "integer";
    ival = *(int *)value;
    ival_null = 0;
  } else if (type == BoolType) {
    type_name = "bool";
    ival = *(bool *)value ? 1 : 0;
    ival_null = 0;
  } else if (type == FloatType) {
    fprintf(stderr, "configuration: float values are not written here\n");
    return false;
  } else {
    fprintf(stderr, "configuration: unsupported value type for %s\n", name);
    return false;
  }

  conn = conf_connect(conf);
  if (conn == NULL) {
    return false;
  }
  stmt = mysql_stmt_init(conn);
  if (stmt == NULL || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
    fprintf(stderr, "configuration: %s\n",
            stmt != NULL ? mysql_stmt_error(stmt) : mysql_error(conn));
    if (stmt != NULL) {
      mysql_stmt_close(stmt);
    }
    mysql_close(conn);
    return false;
  }
  name_len = strlen(name);
  type_len = strlen(type_name);
  if (cval == NULL) {
    cval = (char *)"";
    cval_len = 0;
  }
  memset(bind, 0, sizeof(bind));
  bind[0].buffer_type = MYSQL_TYPE_STRING;
  bind[0].buffer = (char *)name;
  bind[0].buffer_length = name_len;
  bind[0].length = &name_len;
  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer = (char *)type_name;
  bind[1].buffer_length = type_len;
  bind[1].length = &type_len;
  bind[2].buffer_type = MYSQL_TYPE_LONG;
  bind[2].buffer = &ival;
  bind[2].is_null = &ival_null;
  bind[3].buffer_type = MYSQL_TYPE_STRING;
  bind[3].buffer = cval;
  bind[3].buffer_length = cval_len + 1;
  bind[3].length = &cval_len;
  bind[3].is_null = &cval_null;
  if (mysql_stmt_bind_param(stmt, bind) != 0 || mysql_stmt_execute(stmt) != 0) {
    fprintf(stderr, "configuration: %s\n", mysql_stmt_error(stmt));
  } else if (!bx_conf_set(conf, name, value, type)) {
    fprintf(stderr, "configuration: could not keep %s in memory\n", name);
  } else {
    ok = true;
  }
  mysql_stmt_close(stmt);
  mysql_close(conn);
  return ok;
}
