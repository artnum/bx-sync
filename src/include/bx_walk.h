#ifndef BX_WALK_H__
#define BX_WALK_H__

#include "bx_object_value.h"
#include "bxill.h"
#include <jansson.h>
#include <mysql/mysql.h>
#include <time.h>

typedef BXillError (*bx_walk_sync_fn)(bXill *app, MYSQL *conn, json_t *item,
                                      void *ctx);

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

#endif /* BX_WALK_H__ */
