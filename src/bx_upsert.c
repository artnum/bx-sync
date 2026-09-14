#include "include/bx_upsert.h"
#include "include/bx_database.h"
#include "include/bx_object.h"
#include "include/bx_utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BX_UPSERT_MAX_FIELDS 48

static int table_allowed(const char *table) {
  static const char *ok[] = {
      "country",
      "account_group",
      "client_service",
      "communication_kind",
      "pr_project_state",
      "pr_project_type",
      "timesheet_status",
      "todo_status",
      "todo_priority",
      "stock",
      "stock_place",
      "bank_account",
      "company_profile",
      "calendar_year",
      "business_year",
      "vat_period",
      "contact_relation",
      "additional_address",
      "article",
      "note",
      "task",
      "timesheet",
      "kb_quote",
      "kb_order",
      "kb_delivery",
      "invoice_payment",
      "invoice_reminder",
      "kb_comment",
      "project_milestone",
      "project_package",
      "purchase_order",
      "purchase_bill",
      "expense",
      "manual_entry",
      "manual_entry_line",
      NULL};
  for (int i = 0; ok[i]; i++) {
    if (strcmp(table, ok[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static const char *fname(const BXJsonField *f) {
  return f->sql ? f->sql : f->key;
}

typedef struct {
  BXUInteger u;
  BXInteger i;
  BXString s;
  BXFloat fl;
  BXBool b;
  BXUuid q;
} BXSlot;

static void free_slots(BXSlot *slots, const BXJsonField *fields, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (fields[i].kind == BX_F_STR) {
      bx_object_free_value(&slots[i].s);
    }
  }
}

static const BXGeneric *slot_generic(BXSlot *sl, BXFieldKind kind) {
  switch (kind) {
  case BX_F_UINT:
    return (BXGeneric *)&sl->u;
  case BX_F_INT:
    return (BXGeneric *)&sl->i;
  case BX_F_STR:
    return (BXGeneric *)&sl->s;
  case BX_F_FLOAT:
    return (BXGeneric *)&sl->fl;
  case BX_F_BOOL:
    return (BXGeneric *)&sl->b;
  case BX_F_UUID:
    return (BXGeneric *)&sl->q;
  }
  return NULL;
}

BXillError bx_json_upsert(MYSQL *conn, const char *table, json_t *obj,
                          const BXJsonField *fields, size_t nfields,
                          const char *parent_sql, uint64_t parent_id) {
  if (conn == NULL || obj == NULL || fields == NULL || nfields == 0 ||
      nfields > BX_UPSERT_MAX_FIELDS || !table_allowed(table)) {
    return ErrorGeneric;
  }

  XXH3_state_t *hash = XXH3_createState();
  if (hash == NULL) {
    return ErrorGeneric;
  }
  XXH3_64bits_reset(hash);
  if (parent_sql) {
    XXH3_64bits_update(hash, &parent_id, sizeof(parent_id));
  }

  BXSlot slots[BX_UPSERT_MAX_FIELDS];
  memset(slots, 0, sizeof(slots));
  for (size_t i = 0; i < nfields; i++) {
    switch (fields[i].kind) {
    case BX_F_UINT:
      slots[i].u = bx_object_get_json_uint(obj, fields[i].key, hash);
      break;
    case BX_F_INT:
      slots[i].i = bx_object_get_json_int(obj, fields[i].key, hash);
      break;
    case BX_F_STR:
      slots[i].s = bx_object_get_json_string(obj, fields[i].key, hash);
      break;
    case BX_F_FLOAT:
      slots[i].fl = bx_object_get_json_double(obj, fields[i].key, hash);
      break;
    case BX_F_BOOL:
      slots[i].b = bx_object_get_json_bool(obj, fields[i].key, hash);
      break;
    case BX_F_UUID:
      slots[i].q = bx_object_get_json_uuid(obj, fields[i].key, hash);
      break;
    }
  }
  uint64_t checksum = XXH3_64bits_digest(hash);
  XXH3_freeState(hash);

  int pk_ok = 0;
  switch (fields[0].kind) {
  case BX_F_UINT:
    pk_ok = slots[0].u.isset;
    break;
  case BX_F_INT:
    pk_ok = slots[0].i.isset;
    break;
  case BX_F_STR:
    pk_ok = slots[0].s.isset && slots[0].s.value != NULL;
    break;
  case BX_F_UUID:
    pk_ok = slots[0].q.isset;
    break;
  case BX_F_FLOAT:
  case BX_F_BOOL:
    break;
  }
  if (!pk_ok) {
    free_slots(slots, fields, nfields);
    return ErrorGeneric;
  }

  char select_sql[256];
  if (parent_sql) {
    snprintf(select_sql, sizeof(select_sql),
             "SELECT _checksum FROM %s WHERE %s = :id AND %s = :_parent", table,
             fname(&fields[0]), parent_sql);
  } else {
    snprintf(select_sql, sizeof(select_sql),
             "SELECT _checksum FROM %s WHERE %s = :id", table,
             fname(&fields[0]));
  }

  BXDatabaseQuery *query = bx_database_new_query(conn, select_sql);
  if (query == NULL) {
    free_slots(slots, fields, nfields);
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id",
                         slot_generic(&slots[0], fields[0].kind));
  if (parent_sql) {
    bx_database_add_param_uint64(query, ":_parent", &parent_id);
  }
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    free_slots(slots, fields, nfields);
    return e;
  }
  int need_insert =
      (query->results == NULL || query->results->column_count == 0);
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == checksum) {
    bx_database_free_query(query);
    free_slots(slots, fields, nfields);
    return NoError;
  }
  bx_database_free_query(query);

  char cols[1024];
  char vals[1024];
  char sets[1024];
  cols[0] = vals[0] = sets[0] = 0;
  size_t cl = 0, vl = 0, sl = 0;
  for (size_t i = 0; i < nfields; i++) {
    const char *n = fname(&fields[i]);
    cl += (size_t)snprintf(cols + cl, sizeof(cols) - cl, "%s%s", i ? ", " : "",
                           n);
    vl += (size_t)snprintf(vals + vl, sizeof(vals) - vl, "%s:%s", i ? ", " : "",
                           n);
    if (i > 0) {
      sl += (size_t)snprintf(sets + sl, sizeof(sets) - sl, "%s%s = :%s",
                             sl ? ", " : "", n, n);
    }
  }
  if (parent_sql) {
    cl += (size_t)snprintf(cols + cl, sizeof(cols) - cl, ", %s", parent_sql);
    vl += (size_t)snprintf(vals + vl, sizeof(vals) - vl, ", :_parent");
  }
  cl += (size_t)snprintf(cols + cl, sizeof(cols) - cl,
                         ", _checksum, _last_updated, _deleted");
  vl += (size_t)snprintf(vals + vl, sizeof(vals) - vl,
                         ", :_checksum, :_last_updated, :_deleted");
  sl += (size_t)snprintf(sets + sl, sizeof(sets) - sl,
                         "%s_checksum = :_checksum, _last_updated = "
                         ":_last_updated, _deleted = :_deleted",
                         sl ? ", " : "");

  char sql[2800];
  if (need_insert) {
    snprintf(sql, sizeof(sql), "INSERT INTO %s (%s) VALUES (%s)", table, cols,
             vals);
  } else if (parent_sql) {
    snprintf(sql, sizeof(sql), "UPDATE %s SET %s WHERE %s = :id AND %s = :_parent",
             table, sets, fname(&fields[0]), parent_sql);
  } else {
    snprintf(sql, sizeof(sql), "UPDATE %s SET %s WHERE %s = :id", table, sets,
             fname(&fields[0]));
  }

  query = bx_database_new_query(conn, sql);
  if (query == NULL) {
    free_slots(slots, fields, nfields);
    return ErrorGeneric;
  }
  for (size_t i = 0; i < nfields; i++) {
    char pname[64];
    snprintf(pname, sizeof(pname), ":%s", fname(&fields[i]));
    bx_database_add_bxtype(query, pname,
                           slot_generic(&slots[i], fields[i].kind));
  }
  uint64_t now = (uint64_t)time(NULL);
  uint64_t not_deleted = 0;
  if (parent_sql) {
    bx_database_add_param_uint64(query, ":_parent", &parent_id);
  }
  bx_database_add_param_uint64(query, ":_checksum", &checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);
  bx_database_add_param_uint64(query, ":_deleted", &not_deleted);

  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    free_slots(slots, fields, nfields);
    return e;
  }
  bx_database_free_query(query);
  free_slots(slots, fields, nfields);
  return NoError;
}
