#include "include/bx_chain.h"
#include "include/bx_conf.h"

#include <stdio.h>

bool bx_chain_load(bXill *app, BXChainCursor *out) {
  const char *stored = NULL;
  const char *step = NULL;
  int offset = 0;
  int pass = 0;

  if (out == NULL) {
    return false;
  }
  if (app != NULL && app->conf != NULL) {
    stored = bx_conf_get_string(app->conf, BX_CHAIN_KEY);
    step = bx_conf_get_string(app->conf, BX_CHAIN_KEY_STEP);
    offset = bx_conf_get_int(app->conf, BX_CHAIN_KEY_OFFSET);
    pass = bx_conf_get_int(app->conf, BX_CHAIN_KEY_CYCLE);
  }
  bx_chain_decode(stored, step, offset, pass, out);
  return true;
}

bool bx_chain_save(bXill *app, MYSQL *conn, const BXChainCursor *in) {
  char value[64];
  int n = 0;

  if (app == NULL || app->conf == NULL || conn == NULL || in == NULL) {
    return false;
  }
  n = snprintf(value, sizeof(value), "%s:%d", bx_chain_phase_name(in->phase),
               in->offset);
  if (n < 0 || (size_t)n >= sizeof(value)) {
    return false;
  }
  return bx_conf_put_database_conn(app->conf, conn, BX_CHAIN_KEY, StringType,
                                   value);
}
