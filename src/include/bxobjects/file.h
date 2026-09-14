#ifndef BX_OBJECT_FILE_H__
#define BX_OBJECT_FILE_H__

#include "../bx_ids_cache.h"
#include "../bxill.h"

BXillError bx_file_walk_items(bXill *app, MYSQL *conn, Cache *cache);

#endif /* BX_OBJECT_FILE_H__ */
