#include "include/bx_chain.h"
#include "include/bx_conf.h"
#include "include/bx_utils.h"

#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

/* One statement so the three keys move together. Names are fixed literals. */
static bool write_progress(MYSQL *conn, const BXChainCursor *cur) {
  char sql[512];
  const char *step = bx_chain_step_name(cur->step);
  int n = 0;

  if (conn == NULL || cur == NULL) {
    return false;
  }
  n = snprintf(sql, sizeof(sql),
               "INSERT INTO _configuration (name, type, ival, cval) VALUES "
               "('%s','string',NULL,'%s'),"
               "('%s','integer',%d,NULL),"
               "('%s','integer',%d,NULL) "
               "ON DUPLICATE KEY UPDATE type = VALUES(type), "
               "ival = VALUES(ival), cval = VALUES(cval), "
               "bval = NULL, fval = NULL",
               BX_CHAIN_KEY_STEP, step, BX_CHAIN_KEY_OFFSET, cur->offset,
               BX_CHAIN_KEY_CYCLE, cur->pass);
  if (n < 0 || (size_t)n >= sizeof(sql)) {
    return false;
  }
  if (mysql_query(conn, sql) != 0) {
    bx_log_error("Sync chain state was not stored: %s", mysql_error(conn));
    return false;
  }
  return true;
}

static bool db_load(void *ctx, BXChainCursor *out) {
  bXill *app = ctx;
  const char *step = NULL;

  out->step = BX_CHAIN_CONTACT;
  out->offset = 0;
  out->pass = 0;
  if (app == NULL || app->conf == NULL) {
    return true;
  }
  step = bx_conf_get_string(app->conf, BX_CHAIN_KEY_STEP);
  if (step == NULL || !bx_chain_parse_step(step, &out->step)) {
    out->step = BX_CHAIN_CONTACT;
    return true;
  }
  out->offset = bx_conf_get_int(app->conf, BX_CHAIN_KEY_OFFSET);
  out->pass = bx_conf_get_int(app->conf, BX_CHAIN_KEY_CYCLE);
  return true;
}

static bool db_save(void *ctx, MYSQL *conn, const BXChainCursor *in) {
  bXill *app = ctx;
  int offset = 0;
  int pass = 0;
  const char *step = NULL;

  if (!write_progress(conn, in)) {
    return false;
  }
  if (app == NULL || app->conf == NULL) {
    return true;
  }
  step = bx_chain_step_name(in->step);
  offset = in->offset;
  pass = in->pass;
  bx_conf_set(app->conf, BX_CHAIN_KEY_STEP, (void *)step, StringType);
  bx_conf_set(app->conf, BX_CHAIN_KEY_OFFSET, &offset, IntegerType);
  bx_conf_set(app->conf, BX_CHAIN_KEY_CYCLE, &pass, IntegerType);
  return true;
}

void bx_chain_use_database(bXill *app) {
  bx_chain_set_store(db_load, db_save, app);
}
