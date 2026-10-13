#include "include/bx_chain.h"
#include "include/bxobjects/contact.h"
#include "include/bxobjects/invoice.h"
#include "include/bxobjects/project.h"

static BXillError bind_contact(bXill *app, MYSQL *conn, Cache *cache,
                               bool archived, int64_t start_offset,
                               bx_walk_page_fn on_page, void *page_ctx) {
  return bx_contact_walk_pass(app, conn, cache, archived, start_offset, on_page,
                              page_ctx);
}

static BXillError bind_project(bXill *app, MYSQL *conn, Cache *cache,
                               int64_t start_offset, bx_walk_page_fn on_page,
                               void *page_ctx) {
  return bx_project_walk_item_from(app, conn, cache, start_offset, on_page,
                                   page_ctx);
}

static BXillError bind_invoice(bXill *app, MYSQL *conn, Cache *cache,
                               int64_t start_offset, bx_walk_page_fn on_page,
                               void *page_ctx) {
  return bx_invoice_walk_list(app, conn, cache, start_offset, on_page, page_ctx);
}

static BXillError bind_missed(bXill *app, MYSQL *conn, Cache *cache) {
  return bx_invoice_fetch_missed(app, conn, cache);
}

void bx_chain_bind_default(void) {
  bx_chain_set_walks(bind_contact, bind_project, bind_invoice, bind_missed);
}
