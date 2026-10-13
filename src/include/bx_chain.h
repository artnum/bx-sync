#ifndef BX_CHAIN_H__
#define BX_CHAIN_H__

#include "bx_ids_cache.h"
#include "bx_walk.h"
#include "bxill.h"
#include <stdbool.h>

/* Runtime progress in _configuration. Not settings: a row overrides the JSON
 * file, so these names must not be used as config keys. */
#define BX_CHAIN_KEY_STEP "sync-chain-step"
#define BX_CHAIN_KEY_OFFSET "sync-chain-offset"
#define BX_CHAIN_KEY_CYCLE "sync-chain-cycle"

typedef enum {
  BX_CHAIN_CONTACT = 0,
  BX_CHAIN_PROJECT = 1,
  BX_CHAIN_INVOICE = 2
} BXChainStep;

/* offset is the next list offset to request (0 = first page of this step).
 * pass is the contact archive pass (0 active, 1 archived). On invoice, pass 1
 * means the list finished and missed ids are not loaded yet. */
typedef struct {
  BXChainStep step;
  int offset;
  int pass;
} BXChainCursor;

typedef struct {
  BXillError error;
  bool allow_prune;
} BXChainResult;

typedef bool (*bx_chain_load_fn)(void *ctx, BXChainCursor *out);
typedef bool (*bx_chain_save_fn)(void *ctx, MYSQL *conn,
                                 const BXChainCursor *in);

typedef BXillError (*bx_chain_contact_pass_fn)(bXill *app, MYSQL *conn,
                                               Cache *cache, bool archived,
                                               int64_t start_offset,
                                               bx_walk_page_fn on_page,
                                               void *page_ctx);
typedef BXillError (*bx_chain_list_fn)(bXill *app, MYSQL *conn, Cache *cache,
                                       int64_t start_offset,
                                       bx_walk_page_fn on_page, void *page_ctx);
typedef BXillError (*bx_chain_missed_fn)(bXill *app, MYSQL *conn, Cache *cache);

void bx_chain_set_store(bx_chain_load_fn load, bx_chain_save_fn save, void *ctx);
void bx_chain_set_walks(bx_chain_contact_pass_fn contact_pass,
                        bx_chain_list_fn project, bx_chain_list_fn invoice,
                        bx_chain_missed_fn invoice_missed);
/* Bind the real contact, project and invoice walkers. */
void bx_chain_bind_default(void);
/* Load progress from _configuration. No new table. */
void bx_chain_use_database(bXill *app);
/* Load the cursor and arm the one-shot startup skip. */
void bx_chain_init(void);
void bx_chain_read(BXChainCursor *out);

BXChainResult bx_chain_sync_contact(bXill *app, MYSQL *conn, Cache *cache);
BXChainResult bx_chain_sync_project(bXill *app, MYSQL *conn, Cache *cache);
BXChainResult bx_chain_sync_invoice(bXill *app, MYSQL *conn, Cache *cache);

const char *bx_chain_step_name(BXChainStep step);
bool bx_chain_parse_step(const char *name, BXChainStep *step);

#endif /* BX_CHAIN_H__ */
