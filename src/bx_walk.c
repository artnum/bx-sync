#include "include/bx_walk.h"
#include "include/bx_net.h"
#include "include/bx_utils.h"
#include <string.h>
#include <threads.h>

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

BXillError bx_walk_pages(bXill *app, MYSQL *conn, const char *path_fmt,
                         BXGeneric *extra, const struct timespec *page_sleep,
                         bx_walk_sync_fn sync, void *ctx) {
  if (app == NULL || conn == NULL || path_fmt == NULL || sync == NULL) {
    return ErrorGeneric;
  }

  int use_page = strstr(path_fmt, "page=$") != NULL;
  BXInteger offset = {.type = BX_OBJECT_TYPE_INTEGER,
                      .isset = true,
                      .value = use_page ? 1 : 0};
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
    for (size_t i = 0; i < arr_len; i++) {
      BXillError e = sync(app, conn, json_array_get(arr, i), ctx);
      if (e != NoError) {
        bx_net_request_free(request);
        return e;
      }
    }
    bx_net_request_free(request);
    if (!paged) {
      break;
    }
    if (page_sleep != NULL) {
      thrd_sleep(page_sleep, NULL);
    }
    if (use_page) {
      offset.value += 1;
    } else {
      offset.value += limit.value;
    }
  } while (arr_len > 0);
  return NoError;
}
