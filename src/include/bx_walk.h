#ifndef BX_WALK_H__
#define BX_WALK_H__

#include "bx_object_value.h"
#include "bxill.h"
#include <jansson.h>
#include <mysql/mysql.h>
#include <stdint.h>
#include <time.h>

typedef BXillError (*bx_walk_sync_fn)(bXill *app, MYSQL *conn, json_t *item,
                                      void *ctx);

/* Called after every row of a non-empty page synced. next_offset is the
 * offset (or page number) the next request will use. */
typedef BXillError (*bx_walk_page_fn)(void *page_ctx, int64_t next_offset);

/**
 * Fetch path_fmt in pages and call sync for each item.
 *
 * path_fmt may contain $ placeholders. If it has none, a single request is
 * made. Pagination is offset (default) or page when the path contains
 * "page=$". extra, if non-NULL, is a third $ after limit and offset/page.
 * page_sleep, if non-NULL, is waited after each page.
 */
BXillError bx_walk_pages(bXill *app, MYSQL *conn, const char *path_fmt,
                         BXGeneric *extra, const struct timespec *page_sleep,
                         bx_walk_sync_fn sync, void *ctx);

/* Same walk, starting at start_offset (0 matches bx_walk_pages). on_page, if
 * non-NULL, runs only after a non-empty page synced without error, and receives
 * the next offset. */
BXillError bx_walk_pages_from(bXill *app, MYSQL *conn, const char *path_fmt,
                              BXGeneric *extra, const struct timespec *page_sleep,
                              bx_walk_sync_fn sync, void *ctx,
                              int64_t start_offset, bx_walk_page_fn on_page,
                              void *page_ctx);

#endif /* BX_WALK_H__ */
