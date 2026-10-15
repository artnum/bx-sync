#ifndef BX_CHAIN_H__
#define BX_CHAIN_H__

#include "bx_ids_cache.h"
#include "bxill.h"
#include <stdbool.h>

/* Runtime progress in _configuration. Not a setting: a row overrides the JSON
 * file, so this name must not be used as a config key. */
#define BX_CHAIN_KEY "sync-chain"
/* Previous three-key cursor. Read only when BX_CHAIN_KEY is absent. */
#define BX_CHAIN_KEY_STEP "sync-chain-step"
#define BX_CHAIN_KEY_OFFSET "sync-chain-offset"
#define BX_CHAIN_KEY_CYCLE "sync-chain-cycle"

typedef enum {
  BX_CHAIN_CONTACT_ACTIVE = 0,
  BX_CHAIN_CONTACT_ARCHIVED = 1,
  BX_CHAIN_PROJECT = 2,
  BX_CHAIN_INVOICE_LIST = 3,
  BX_CHAIN_INVOICE_MISSED = 4
} BXChainPhase;

/* offset is the next list offset (0 = first page). Missed-ids keep it at 0. */
typedef struct {
  BXChainPhase phase;
  int offset;
} BXChainCursor;

/* One call of bx_chain_sync runs the current phase. pause is set when that
 * call finishes a resource (archived contacts, projects, or invoices). */
typedef struct {
  BXillError error;
  bool prune_contact;
  bool prune_project;
  bool prune_invoice;
  bool bump_contact;
  bool bump_project;
  bool bump_invoice;
  bool pause;
} BXChainResult;

bool bx_chain_load(bXill *app, BXChainCursor *out);
bool bx_chain_save(bXill *app, MYSQL *conn, const BXChainCursor *in);

void bx_chain_init(bXill *app);
void bx_chain_read(BXChainCursor *out);
BXChainResult bx_chain_sync(bXill *app, MYSQL *conn, Cache *contacts,
                            Cache *projects, Cache *invoices);

const char *bx_chain_phase_name(BXChainPhase phase);
/* sync_chain wins when it parses. legacy_step NULL skips the old rows. */
void bx_chain_decode(const char *sync_chain, const char *legacy_step,
                     int legacy_offset, int legacy_pass, BXChainCursor *out);

#endif /* BX_CHAIN_H__ */
