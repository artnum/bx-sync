#include "include/bx_walk.h"
#include "include/bx_net.h"
#include "include/bx_utils.h"
#include <string.h>
#include <threads.h>

static void log_item_failure(const json_t *item, BXillError err) {
  json_t *id = item != NULL ? json_object_get(item, "id") : NULL;
  if (id != NULL && json_is_integer(id)) {
    bx_log_error("Walk item %lld failed (%d), continuing page",
                 (long long)json_integer_value(id), (int)err);
    return;
  }
  bx_log_error("Walk item failed (%d), continuing page", (int)err);
}

static json_t *as_array(json_t *root) {
  if (json_is_array(root)) {
    return root;
  }
  if (json_is_object(root)) {
    json_t *data = json_object_get(root, "data");
    if (json_is_array(data)) {
      return data;
    }
  }
  return NULL;
}

BXillError bx_walk_pages_from(bXill *app, MYSQL *conn, const char *path_fmt,
                              BXGeneric *extra, const struct timespec *page_sleep,
                              bx_walk_sync_fn sync, void *ctx,
                              int64_t start_offset, bx_walk_page_fn on_page,
                              void *page_ctx) {
  if (app == NULL || conn == NULL || path_fmt == NULL || sync == NULL) {
    return ErrorGeneric;
  }

  int use_page = strstr(path_fmt, "page=$") != NULL;
  int64_t start = start_offset < 0 ? 0 : start_offset;
  BXInteger offset = {.type = BX_OBJECT_TYPE_INTEGER,
                      .isset = true,
                      .value = use_page ? (start > 0 ? start : 1) : start};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BXILL_LIST_LIMIT};
  int paged = (strstr(path_fmt, "$") != NULL);
  size_t arr_len = 0;

  do {
    BXNetRequest *request = NULL;
    if (!paged) {
      request = bx_do_request(app->queue, NULL, (char *)path_fmt);
    } else if (extra != NULL) {
      request = bx_do_request(app->queue, NULL, (char *)path_fmt, &limit,
                              &offset, extra);
    } else {
      request =
          bx_do_request(app->queue, NULL, (char *)path_fmt, &limit, &offset);
    }
    if (request == NULL) {
      return ErrorNet;
    }
    json_t *arr = as_array(request->decoded);
    if (arr == NULL) {
      bx_net_request_free(request);
      return ErrorJSON;
    }
    arr_len = json_array_size(arr);
    size_t ok_count = 0;
    size_t fail_count = 0;
    BXillError first_error = NoError;
    for (size_t i = 0; i < arr_len; i++) {
      json_t *item = json_array_get(arr, i);
      BXillError e = sync(app, conn, item, ctx);
      if (e == NoError) {
        ok_count++;
        continue;
      }
      /* The connection is dead. The rest of this page was not processed. */
      if (e == ErrorSQLReconnect) {
        bx_net_request_free(request);
        return e;
      }
      if (first_error == NoError) {
        first_error = e;
      }
      fail_count++;
      log_item_failure(item, e);
    }
    if (arr_len > 0 && ok_count == 0 && fail_count > 0) {
      bx_net_request_free(request);
      return first_error;
    }
    int64_t next = offset.value;
    if (paged) {
      if (use_page) {
        next += 1;
      } else {
        next += limit.value;
      }
    }
    if (arr_len > 0 && on_page != NULL) {
      BXillError pe = on_page(page_ctx, next);
      if (pe != NoError) {
        bx_net_request_free(request);
        return pe;
      }
    }
    bx_net_request_free(request);
    if (!paged) {
      break;
    }
    if (page_sleep != NULL) {
      thrd_sleep(page_sleep, NULL);
    }
    offset.value = next;
  } while (arr_len > 0);
  return NoError;
}

BXillError bx_walk_pages(bXill *app, MYSQL *conn, const char *path_fmt,
                         BXGeneric *extra, const struct timespec *page_sleep,
                         bx_walk_sync_fn sync, void *ctx) {
  return bx_walk_pages_from(app, conn, path_fmt, extra, page_sleep, sync, ctx, 0,
                            NULL, NULL);
}
