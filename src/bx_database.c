#include "include/bx_database.h"
#include "include/bx_object_value.h"
#include "include/bx_utils.h"
#include <assert.h>
#include <mariadb/mariadb_com.h>
#include <mysql/mysql.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BX_DB_STATE_COLON 0x01
#define BX_DB_STATE_VARNAME 0x02

static inline bool _is_variable_char(unsigned char c) {
  if (c >= '0' && c <= '9') {
    return true;
  }
  if (c >= 'a' && c <= 'z') {
    return true;
  }
  if (c >= 'A' && c <= 'Z') {
    return true;
  }
  if (c == '_') {
    return true;
  }
  return false;
}
static inline void _bx_database_free_column(BXDatabaseColumn *column) {
  if (column == NULL) {
    return;
  }
  if (column->c_value != NULL) {
    free(column->c_value);
  }
}

static inline void _bx_database_free_row(BXDatabaseRow *row) {
  if (row == NULL) {
    return;
  }
  if (row->columns != NULL) {
    for (int i = 0; i < row->column_count; i++) {
      _bx_database_free_column(&row->columns[i]);
    }
  }
  free(row->columns);
  free(row);
}

static inline void _bx_database_free_result(BXDatabaseQuery *query) {
  if (query->results != NULL) {
    BXDatabaseRow *current = query->results;
    BXDatabaseRow *next = NULL;
    while (current != NULL) {
      next = current->next;
      _bx_database_free_row(current);
      current = next;
    }
  }

  query->results = NULL;
  if (query->fields != NULL) {
    for (int i = 0; i < query->field_count; i++) {
      if (query->fields[i].name != NULL) {
        free(query->fields[i].name);
      }
    }
    free(query->fields);
  }
  query->fields = NULL;
  query->field_count = 0;
}

static void free_query_metadata(BXDatabaseQuery *query) {
  if (query->result_metadata != NULL) {
    mysql_free_result(query->result_metadata);
    query->result_metadata = NULL;
    query->has_dataset = false;
  }
}

void bx_database_free_query(BXDatabaseQuery *query) {
  if (query == NULL) {
    return;
  }
  _bx_database_free_result(query);
  free_query_metadata(query);
  if (query->parameters) {
    free(query->parameters);
  }
  if (query->query) {
    free(query->query);
  }
  if (query->stmt) {
    mysql_stmt_close(query->stmt);
  }
  if (query->binds) {
    free(query->binds);
  }

  free(query);
}

BXDatabaseQuery *bx_database_new_query(MYSQL *mysql, const char *query) {
  assert(query != NULL);
  BXDatabaseQuery *new = NULL;
  new = calloc(1, sizeof(*new));
  if (new == NULL) {
    return NULL;
  }
  new->stmt = mysql_stmt_init(mysql);
  if (new->stmt == NULL) {
    bx_log_error("[MYSQL ERROR] %s", mysql_stmt_error(new->stmt));
    free(new);
    return NULL;
  }

  new->query_length = strlen(query) + 1;
  if (new->query_length <= 1) {
    mysql_stmt_close(new->stmt);
    free(new);
    return NULL;
  }
  new->query = calloc(new->query_length, sizeof(*new->query));
  if (new->query == NULL) {
    mysql_stmt_close(new->stmt);
    free(new);
    return NULL;
  }
  memcpy(new->query, query, new->query_length);
  int param_count = 0;
  unsigned char state = 0;
  for (int i = 0; i < new->query_length; i++) {
    if (BX_DB_STATE_COLON & state && _is_variable_char(new->query[i])) {
      state |= BX_DB_STATE_VARNAME;
    }
    if (BX_DB_STATE_VARNAME & state && _is_variable_char(new->query[i])) {
      param_count++;
      state = 0;
    }
    if (BX_DB_STATE_COLON & state && !_is_variable_char(new->query[i])) {
      state = 0;
    }
    if (new->query[i] == ':') {
      state |= BX_DB_STATE_COLON;
    }
  }
  new->param_count = param_count;
  new->parameters = calloc(new->param_count, sizeof(*new->parameters));
  if (new->parameters == NULL) {
    mysql_stmt_close(new->stmt);
    free(new->query);
    free(new);
    return NULL;
  }

  int j = 0;
  param_count = 0;
  for (int i = 0; i < new->query_length; i++) {
    new->query[j] = new->query[i];
    if (new->query[i] == ':' &&
        i + 1 < new->query_length &&_is_variable_char(new->query[i + 1])) {
      int k = i + 1;
      for (; k < new->query_length &&_is_variable_char(new->query[k]); k++)
        ;
      memcpy(new->parameters[param_count].name, &new->query[i], k - i);
      new->parameters[param_count].name[k - i + 1] = '\0';
      new->parameters[param_count].name_length = k - i;
      param_count++;
      new->query[j] = '?';
      i = k - 1;
    }
    j++;
  }
  if (j < new->query_length) {
    new->query[j] = '\0';
  }
  new->exectued = false;
  return new;
}

static inline BXDatabaseParameter *_find_param(BXDatabaseQuery *query,
                                               const char *name) {
  for (int i = 0; i < query->param_count; i++) {
    if (strncmp(name, query->parameters[i].name,
                BX_DATABASE_PARAMETER_MAX_NAME_LENGTH) == 0) {
      return &query->parameters[i];
    }
  }
  return NULL;
}

bool bx_database_replace_param(BXDatabaseQuery *query, const char *name,
                               void *value, size_t value_len) {
  assert(query != NULL);
  assert(value != NULL);
  BXDatabaseParameter *parameter = _find_param(query, name);
  if (parameter == NULL) {
    return false;
  }
  parameter->value = value;
  parameter->value_length = value_len;

  return true;
}

bool bx_database_add_param_str(BXDatabaseQuery *query, const char *name,
                               void *value, size_t value_len,
                               enum enum_field_types type) {
  assert(query != NULL);
  assert(name != NULL);

  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }

  param->value = value;
  param->value_length = value_len;
  param->type = type;
  param->is_null = value == NULL ? 1 : 0;
  param->is_unsigned = 0;
  param->error = 0;
  return true;
}

bool bx_database_add_param_int(BXDatabaseQuery *query, const char *name,
                               void *value, size_t value_len,
                               enum enum_field_types type, bool is_unsigned) {
  assert(query != NULL);
  assert(name != NULL);

  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }

  param->value = value;
  param->value_length = value_len;
  param->type = type;
  param->is_null = value == NULL ? 1 : 0;
  param->is_unsigned = is_unsigned;
  return true;
}

static bool set_null(BXDatabaseParameter *param) {
  param->is_null = 1;
  param->value = NULL;
  param->type = MYSQL_TYPE_NULL;
  param->value_length = 0;
  return true;
}

static bool add_bxint(BXDatabaseQuery *query, const char *name,
                      const BXInteger *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset) {
    return set_null(param);
  }

  param->value = (void *)&value->value;
  param->value_length = sizeof(value->value);
  param->type = MYSQL_TYPE_LONGLONG;
  param->is_null = 0;
  param->is_unsigned = 0;

  return true;
}

static bool add_bxuint(BXDatabaseQuery *query, const char *name,
                       const BXUInteger *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset) {
    return set_null(param);
  }

  param->value = (void *)&value->value;
  param->value_length = sizeof(value->value);
  param->type = MYSQL_TYPE_LONGLONG;
  param->is_null = 0;
  param->is_unsigned = 0;

  return true;
}

static bool add_bxstr(BXDatabaseQuery *query, const char *name,
                      const BXString *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset || value->value == NULL) {
    return set_null(param);
  }
  param->value = (void *)value->value;
  param->value_length = value->value_len;
  param->type = MYSQL_TYPE_STRING;
  param->is_null = 0;
  param->is_unsigned = 0;
  return true;
}

static bool add_bxfloat(BXDatabaseQuery *query, const char *name,
                        const BXFloat *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset) {
    return set_null(param);
  }
  param->value = (void *)&value->value;
  param->value_length = sizeof(value->value);
  param->type = MYSQL_TYPE_DOUBLE;
  param->is_null = 0;
  param->is_unsigned = 0;
  return true;
}

static bool add_bxbool(BXDatabaseQuery *query, const char *name,
                       const BXBool *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset) {
    return set_null(param);
  }

  param->type = MYSQL_TYPE_TINY;
  param->value = (void *)&value->value;
  param->value_length = sizeof(value->value);
  param->is_null = 0;
  param->is_unsigned = 0;
  return true;

  return true;
}

static bool add_bxbytes(BXDatabaseQuery *query, const char *name,
                        const BXBytes *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset || value->value == NULL) {
    return set_null(param);
  }
  param->value = (void *)value->value;
  param->value_length = value->value_len;
  param->type = MYSQL_TYPE_BLOB;
  param->is_null = 0;
  param->is_unsigned = 0;
  return true;
}

static bool add_bxuuid(BXDatabaseQuery *query, const char *name,
                       const BXUuid *value) {
  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  if (!value->isset) {
    return set_null(param);
  }
  param->value = (void *)&value->value;
  param->value_length = sizeof(uint64_t) * 2;
  param->type = MYSQL_TYPE_BLOB;
  param->is_null = 0;
  param->is_unsigned = 0;
  return true;
}

bool bx_database_add_bxtype(BXDatabaseQuery *query, const char *name,
                            const BXGeneric *value) {
  assert(query != NULL);
  assert(name != NULL);

  switch (*(uint8_t *)value) {
  case BX_OBJECT_TYPE_INTEGER:
    return add_bxint(query, name, (BXInteger *)value);
  case BX_OBJECT_TYPE_UINTEGER:
    return add_bxuint(query, name, (BXUInteger *)value);
  case BX_OBJECT_TYPE_STRING:
    return add_bxstr(query, name, (BXString *)value);
  case BX_OBJECT_TYPE_FLOAT:
    return add_bxfloat(query, name, (BXFloat *)value);
  case BX_OBJECT_TYPE_BOOL:
    return add_bxbool(query, name, (BXBool *)value);
  case BX_OBJECT_TYPE_BYTES:
    return add_bxbytes(query, name, (BXBytes *)value);
  case BX_OBJECT_TYPE_UUID:
    return add_bxuuid(query, name, (BXUuid *)value);
  }

  BXDatabaseParameter *param = _find_param(query, name);
  if (param == NULL) {
    return false;
  }
  printf("NULL PARAM %s\n\tFOR QUERY %s\n", name, query->query);
  return set_null(param);
}

static inline void _bx_database_param_to_bind(MYSQL_BIND *bind,
                                              BXDatabaseParameter *parameter) {

  bind->is_null = (char *)parameter->is_null;
  if (bind->is_null) {
    bind->buffer_type = MYSQL_TYPE_NULL;
    parameter->value_length = 0;
    bind->buffer_length = parameter->value_length;
    bind->length = &parameter->value_length;
    bind->buffer = NULL;
    return;
  }

  bind->buffer_type = parameter->type;
  bind->buffer = parameter->value;
  switch (parameter->type) {
  default:
    break;
  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_LONGLONG:
    bind->is_unsigned = parameter->is_unsigned;
    break;

  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_BLOB:
    if (parameter->value == NULL) {
      parameter->is_null = true;
      parameter->value_length = 0;
      bind->buffer_type = MYSQL_TYPE_NULL;
    }
    bind->buffer_length = parameter->value_length;
    bind->length = &parameter->value_length;
    break;
  }
}

bool bx_database_execute(BXDatabaseQuery *query) {
  assert(query != NULL);
  if (query->exectued) {
    mysql_stmt_reset(query->stmt);
    _bx_database_free_result(query);
    memset(query->binds, 0, query->param_count * sizeof(*query->binds));
  } else {
    query->binds = calloc(query->param_count, sizeof(*query->binds));
    if (query->binds == NULL) {
      return false;
    }

    if (mysql_stmt_prepare(query->stmt, query->query, strlen(query->query)) !=
        0) {
      bx_log_error("[MYSQL ERROR] %s", mysql_stmt_error(query->stmt));
      return false;
    }
  }

  for (int i = 0; i < query->param_count; i++) {
    _bx_database_param_to_bind(&query->binds[i], &query->parameters[i]);
  }

  if (mysql_stmt_bind_param(query->stmt, query->binds) != 0) {
    bx_log_error("[MYSQL ERROR] %s", mysql_stmt_error(query->stmt));
    free(query->binds);
    return false;
  }

  query->has_failed = false;
  if (mysql_stmt_execute(query->stmt) != 0) {
    query->has_failed = true;
    bx_log_error("[MYSQL ERROR] %s %s", mysql_stmt_error(query->stmt),
                 query->query);
    return false;
  }
  query->affected_rows = mysql_stmt_affected_rows(query->stmt);
  query->warning_rows = mysql_stmt_warning_count(query->stmt);
  if (query->warning_rows > 0) {
    bx_log_debug("MYSQL Warning %d, query %s", query->warning_rows,
                 query->query);
    bx_database_print_warnings(query->stmt->mysql);
  }
  /* mysql_stmt_result_metadata returns NULL if the query doesn't produce
   * a dataset (INSERT, UPDATE, ...) with no error set.
   * So we can differentiate between SELECT, ... here so in result we can
   * accodingly to use to check for failure.
   */
  query->has_dataset = true;
  query->result_metadata = mysql_stmt_result_metadata(query->stmt);
  if (query->result_metadata == NULL) {
    if (mysql_stmt_errno(query->stmt) != 0) {
      query->has_failed = true;
      return false;
    }
    query->has_dataset = false;
  }
  query->exectued = true;

  return true;
}

void bx_database_dump_column(BXDatabaseColumn *column) {
  assert(column != NULL);

  printf("COLUMN %d ", column->column);
  switch (column->type) {
  default:
    break;
  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
    printf("VALUE %le", column->f_value);
    break;
  case MYSQL_TYPE_BIT:
  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_LONGLONG:
  case MYSQL_TYPE_INT24:
    printf("VALUE %ld", column->i_value);
    break;
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_DECIMAL:
    printf("VALUE %s", column->c_value);
    break;
  }

  printf("\n");
}

void bx_database_dump_row(BXDatabaseQuery *query, BXDatabaseRow *row) {
  assert(query != NULL);
  assert(row != NULL);
  printf("### ROW %d\n", row->id);
  for (int i = 0; i < row->column_count; i++) {
    printf("> %s\t", bx_database_get_column_name(query, &row->columns[i]));
    bx_database_dump_column(&row->columns[i]);
  }
}

const char *bx_database_get_column_name(BXDatabaseQuery *query,
                                        BXDatabaseColumn *column) {
  assert(query != NULL);
  assert(column != NULL);

  for (int i = 0; i < query->field_count; i++) {
    if (column->column == query->fields[i].id) {
      return query->fields[i].name;
    }
  }
  return NULL;
}

bool bx_database_results(BXDatabaseQuery *query) {
  BXDatabaseColumn *columns;
  MYSQL_BIND *binds = NULL;
  // MYSQL_RES * result;
  MYSQL_FIELD *fields;

  if (!query->has_dataset) {
    return !query->has_failed;
  }

  if (query->result_metadata == NULL || query->has_failed) {
    bx_log_error("[MYSQL ERROR] Passing a failed dataset result");
    return false;
  }

  int field_count = mysql_num_fields(query->result_metadata);
  if (field_count <= 0) {
    free_query_metadata(query);
    return false;
  }

  binds = calloc(field_count, sizeof(*binds));
  if (binds == NULL) {
    free_query_metadata(query);
    return false;
  }

  fields = mysql_fetch_fields(query->result_metadata);
  if (query->fields == NULL) {
    query->field_count = field_count;
    query->fields = calloc(query->field_count, sizeof(*query->fields));
    if (query->fields != NULL) {
      for (int i = 0; i < field_count; i++) {
        query->fields[i].name = strdup(fields[i].name);
        query->fields[i].id = i;
      }
    }
  }
  int fetch_success = 0;
  int row_count = 0;
  while (1) {
    BXDatabaseRow *row = NULL;
    row = calloc(1, sizeof(*row));
    if (row == NULL) {
      break;
    }
    columns = calloc(field_count, sizeof(*columns));
    if (columns == NULL) {
      free(row);
      break;
    }
    row->id = row_count++;
    row->columns = columns;
    row->column_count = field_count;
    for (int i = 0; i < field_count; i++) {
      columns[i].column = i;
      columns[i].type = fields[i].type;
      binds[i].is_null = (char *)&columns[i].is_null;
      binds[i].length = &columns[i].value_len;
      binds[i].buffer_type = fields[i].type;
      switch (fields[i].type) {
      default:
        break;
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIME:
        break;

      case MYSQL_TYPE_NULL:
        break;

      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_DECIMAL:
        binds[i].buffer = columns[i].c_value;
        binds[i].buffer_length = 0;
        break;

      case MYSQL_TYPE_BIT:
      case MYSQL_TYPE_TINY:
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_LONGLONG:
      case MYSQL_TYPE_INT24:
        binds[i].buffer = (char *)&columns[i].i_value;
        binds[i].buffer_length = sizeof(columns[i].i_value);
        break;
      case MYSQL_TYPE_FLOAT:
      case MYSQL_TYPE_DOUBLE:
        binds[i].buffer = &columns[i].f_value;
        binds[i].buffer_length = sizeof(columns[i].f_value);
        break;
      }
    }

    if (mysql_stmt_bind_result(query->stmt, binds) != 0) {
      free(row);
      free(columns);
      break;
    }

    fetch_success = mysql_stmt_fetch(query->stmt);
    if (fetch_success == 1 || fetch_success == MYSQL_NO_DATA) {
      free(row);
      free(columns);
      break;
    }

    for (int i = 0; i < field_count; i++) {
      switch (fields[i].type) {
      default:
        break;
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
        columns[i].c_value =
            calloc(columns[i].value_len + 1, sizeof(*columns[i].c_value));
        if (columns[i].c_value != NULL) {
          binds[i].buffer = columns[i].c_value;
          binds[i].buffer_length = columns[i].value_len;
          binds[i].length = &columns[i].value_len;
          if (mysql_stmt_fetch_column(query->stmt, &binds[i], columns[i].column,
                                      0) != 0) {
            columns[i].is_null = true;
            columns[i].c_value = NULL;
            columns[i].value_len = 0;
          }
        }
        break;
      }
    }
    if (query->results == NULL) {
      query->results = row;
    } else {
      BXDatabaseRow *current = query->results;
      for (; current->next != NULL; current = current->next)
        ;
      current->next = row;
    }
    memset(binds, 0, sizeof(*binds) * field_count);
  }
  /* row_count is always one step ahead of real count */
  query->row_count = row_count - 1;
  free_query_metadata(query);
  free(binds);
  return true;
}

void bx_database_print_warnings(MYSQL *conn) {
  if (mysql_query(conn, "SHOW WARNINGS")) {
    bx_log_debug("SHOW WARNINGS query failed: %s", mysql_error(conn));
    return;
  }

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result) {
    bx_log_debug("Could not get result set: %s", mysql_error(conn));
    return;
  }

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    bx_log_debug("Level: %s, Code: %s, Message: %s", row[0], row[1], row[2]);
  }

  mysql_free_result(result);
}
