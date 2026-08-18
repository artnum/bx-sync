#ifndef BX_NAMED_LIST_H__
#define BX_NAMED_LIST_H__

#include "bxill.h"
#include <mysql/mysql.h>

/**
 * Walk a Bexio list of {id, name} objects into a table with
 * (id, name, _checksum, _last_updated, _deleted).
 * path_fmt must contain two $ placeholders: limit then offset.
 */
BXillError bx_named_list_walk(bXill *app, MYSQL *conn, const char *path_fmt,
                              const char *table);

BXillError bx_unit_walk_items(bXill *app, MYSQL *conn);
BXillError bx_salutation_walk_items(bXill *app, MYSQL *conn);
BXillError bx_title_walk_items(bXill *app, MYSQL *conn);
BXillError bx_payment_type_walk_items(bXill *app, MYSQL *conn);

#endif /* BX_NAMED_LIST_H__ */
