#ifndef BX_UPSERT_H__
#define BX_UPSERT_H__

#include "bx_object_value.h"
#include "bxill.h"
#include <jansson.h>
#include <mysql/mysql.h>

typedef enum {
  BX_F_UINT = 1,
  BX_F_INT,
  BX_F_STR,
  BX_F_FLOAT,
  BX_F_BOOL,
  BX_F_UUID
} BXFieldKind;

typedef struct {
  const char *key;
  const char *sql;
  BXFieldKind kind;
} BXJsonField;

#define BX_FIELD(k, ftype)                                                     \
  { .key = (k), .sql = NULL, .kind = (ftype) }
#define BX_FIELD_SQL(k, s, ftype)                                              \
  { .key = (k), .sql = (s), .kind = (ftype) }

BXillError bx_json_upsert(MYSQL *conn, const char *table, json_t *obj,
                          const BXJsonField *fields, size_t nfields,
                          const char *parent_sql, uint64_t parent_id);

BXillError bx_walk_list(bXill *app, MYSQL *conn, const char *path_fmt,
                        BXillError (*sync)(MYSQL *conn, json_t *item));

BXillError bx_walk_list_app(bXill *app, MYSQL *conn, const char *path_fmt,
                            BXillError (*sync)(bXill *app, MYSQL *conn,
                                               json_t *item));

#endif /* BX_UPSERT_H__ */
