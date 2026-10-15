#include "include/bx_chain.h"
#include "include/bx_conf.h"
#include "include/bx_utils.h"

#include <stdio.h>

bool bx_chain_load(bXill *app, BXChainCursor *out) {
  char stored_buf[64];
  char step_buf[64];
  const char *stored = NULL;
  const char *step = NULL;
  int offset = 0;
  int pass = 0;
  int sync_rc = 0;
  int step_rc = 0;
  bool legacy_rows = false;

  if (out == NULL) {
    return false;
  }
  if (app != NULL && app->conf != NULL) {
    sync_rc = bx_conf_read_string(app->conf, BX_CHAIN_KEY, stored_buf,
                                  sizeof(stored_buf));
    if (sync_rc == 1) {
      stored = stored_buf;
    } else if (sync_rc < 0) {
      /* Present, but not a string we can read. Do not look absent. */
      stored = "";
    }
    step_rc = bx_conf_read_string(app->conf, BX_CHAIN_KEY_STEP, step_buf,
                                  sizeof(step_buf));
    if (step_rc == 1) {
      step = step_buf;
    }
    offset = bx_conf_get_int(app->conf, BX_CHAIN_KEY_OFFSET);
    pass = bx_conf_get_int(app->conf, BX_CHAIN_KEY_CYCLE);
    legacy_rows = step_rc != 0 ||
                  bx_conf_has(app->conf, BX_CHAIN_KEY_OFFSET) ||
                  bx_conf_has(app->conf, BX_CHAIN_KEY_CYCLE);
    if (sync_rc != 0 && legacy_rows) {
      bx_log_info("Legacy sync-chain-step, sync-chain-offset and "
                  "sync-chain-cycle are ignored because sync-chain is set");
    }
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
