#include "../include/bxobjects/file.h"
#include "../include/bx_conf.h"
#include "../include/bx_database.h"
#include "../include/bx_net.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bx_walk.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <xxhash.h>

#define QUERY_INSERT                                                           \
  "INSERT INTO bx_file (id, uuid, name, size_in_bytes, extension, mime_type, " \
  "uploader_email, user_id, is_archived, source_id, source_type, "             \
  "is_referenced, created_at, _checksum, _last_updated, _deleted) VALUES "     \
  "(:id, :uuid, :name, :size_in_bytes, :extension, :mime_type, "               \
  ":uploader_email, :user_id, :is_archived, :source_id, :source_type, "        \
  ":is_referenced, :created_at, :_checksum, :_last_updated, :_deleted)"
#define QUERY_UPDATE                                                           \
  "UPDATE bx_file SET uuid = :uuid, name = :name, "                            \
  "size_in_bytes = :size_in_bytes, extension = :extension, "                   \
  "mime_type = :mime_type, uploader_email = :uploader_email, "                 \
  "user_id = :user_id, is_archived = :is_archived, source_id = :source_id, "    \
  "source_type = :source_type, is_referenced = :is_referenced, "               \
  "created_at = :created_at, _checksum = :_checksum, "                         \
  "_last_updated = :_last_updated, _deleted = :_deleted WHERE id = :id"

#define WALK_FILES_PATH "3.0/files?limit=$&offset=$&archived_state=$"
#define DOWNLOAD_FILE_PATH "3.0/files/$/download"

typedef struct {
  BXUInteger id;
  BXString uuid;
  BXString name;
  BXUInteger size_in_bytes;
  BXString extension;
  BXString mime_type;
  BXString uploader_email;
  BXUInteger user_id;
  BXBool is_archived;
  BXUInteger source_id;
  BXString source_type;
  BXBool is_referenced;
  BXString created_at;
  uint64_t checksum;
} BXFileMeta;

typedef struct {
  Cache *cache;
  const char *cache_dir;
} file_walk_ctx;

static void file_meta_free(BXFileMeta *f) {
  bx_object_free_value(&f->uuid);
  bx_object_free_value(&f->name);
  bx_object_free_value(&f->extension);
  bx_object_free_value(&f->mime_type);
  bx_object_free_value(&f->uploader_email);
  bx_object_free_value(&f->source_type);
  bx_object_free_value(&f->created_at);
}

static int uuid_to_hex(const char *uuid, char hex[33]) {
  size_t n = 0;
  for (const char *p = uuid; *p != '\0' && n < 32; p++) {
    if (*p == '-') {
      continue;
    }
    char c = *p;
    if (c >= 'A' && c <= 'F') {
      c = (char)(c - 'A' + 'a');
    }
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return 0;
    }
    hex[n++] = c;
  }
  hex[n] = '\0';
  return n == 32;
}

static int mkdir_p(const char *dir) {
  char tmp[4096];
  size_t len = strlen(dir);
  if (len == 0 || len >= sizeof(tmp)) {
    return -1;
  }
  memcpy(tmp, dir, len + 1);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }
  for (char *p = tmp + 1; *p != '\0'; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
      }
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
    return -1;
  }
  return 0;
}

static int file_dest_path(const char *cache_dir, const char *uuid_hex,
                          char *out, size_t out_len) {
  if (snprintf(out, out_len, "%s/%c%c/%c%c/%s.bin", cache_dir, uuid_hex[0],
               uuid_hex[1], uuid_hex[2], uuid_hex[3], uuid_hex) >=
      (int)out_len) {
    return -1;
  }
  return 0;
}

static int local_size_matches(const char *path, uint64_t size) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return (uint64_t)st.st_size == size;
}

static BXillError download_file(bXill *app, const BXFileMeta *f,
                                const char *cache_dir) {
  if (cache_dir == NULL || !f->uuid.isset || f->uuid.value == NULL) {
    return NoError;
  }
  char hex[33];
  if (!uuid_to_hex(f->uuid.value, hex)) {
    bx_log_error("File %lu: invalid uuid", (unsigned long)f->id.value);
    return NoError;
  }
  char dest[4096];
  if (file_dest_path(cache_dir, hex, dest, sizeof(dest)) != 0) {
    return ErrorGeneric;
  }
  if (f->size_in_bytes.isset && local_size_matches(dest, f->size_in_bytes.value)) {
    return NoError;
  }
  char dir[4096];
  if (snprintf(dir, sizeof(dir), "%s/%c%c/%c%c", cache_dir, hex[0], hex[1],
               hex[2], hex[3]) >= (int)sizeof(dir)) {
    return ErrorGeneric;
  }
  if (mkdir_p(dir) != 0) {
    bx_log_error("Cannot create %s: %s", dir, strerror(errno));
    return ErrorGeneric;
  }

  BXNetRequest *req =
      bx_do_request_raw(app->queue, NULL, DOWNLOAD_FILE_PATH, &f->id);
  if (req == NULL || req->response == NULL || req->response->http_code != 200 ||
      req->response->data == NULL) {
    bx_log_error("Download failed for file %lu", (unsigned long)f->id.value);
    bx_net_request_free(req);
    return NoError;
  }
  size_t body_len = req->response->data_len;

  char tmp[4096];
  if (snprintf(tmp, sizeof(tmp), "%s.part", dest) >= (int)sizeof(tmp)) {
    bx_net_request_free(req);
    return ErrorGeneric;
  }
  FILE *fp = fopen(tmp, "wb");
  if (fp == NULL) {
    bx_log_error("Cannot write %s: %s", tmp, strerror(errno));
    bx_net_request_free(req);
    return ErrorGeneric;
  }
  size_t n = fwrite(req->response->data, 1, body_len, fp);
  int ferr = ferror(fp);
  fclose(fp);
  bx_net_request_free(req);
  if (ferr || n != body_len) {
    unlink(tmp);
    return ErrorGeneric;
  }
  if (rename(tmp, dest) != 0) {
    bx_log_error("Cannot rename %s: %s", tmp, strerror(errno));
    unlink(tmp);
    return ErrorGeneric;
  }
  return NoError;
}

static BXillError persist_file(MYSQL *conn, BXFileMeta *f) {
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT _checksum FROM bx_file WHERE id = :id");
  if (query == NULL) {
    return ErrorGeneric;
  }
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&f->id);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  int need_insert =
      (query->results == NULL || query->results->column_count == 0);
  if (!need_insert &&
      (uint64_t)query->results->columns[0].i_value == f->checksum) {
    bx_database_free_query(query);
    return NoError;
  }
  bx_database_free_query(query);

  query = bx_database_new_query(conn, need_insert ? QUERY_INSERT : QUERY_UPDATE);
  if (query == NULL) {
    return ErrorGeneric;
  }
  uint64_t now = (uint64_t)time(NULL);
  uint64_t not_deleted = 0;
  if (!bxd_bind(f, id) || !bxd_bind(f, uuid) || !bxd_bind(f, name) ||
      !bxd_bind(f, size_in_bytes) || !bxd_bind(f, extension) ||
      !bxd_bind(f, mime_type) || !bxd_bind(f, uploader_email) ||
      !bxd_bind(f, user_id) || !bxd_bind(f, is_archived) ||
      !bxd_bind(f, source_id) || !bxd_bind(f, source_type) ||
      !bxd_bind(f, is_referenced) || !bxd_bind(f, created_at) ||
      !bx_database_add_param_uint64(query, ":_checksum", &f->checksum) ||
      !bx_database_add_param_uint64(query, ":_last_updated", &now) ||
      !bx_database_add_param_uint64(query, ":_deleted", &not_deleted)) {
    bx_database_free_query(query);
    return ErrorGeneric;
  }
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  bx_database_free_query(query);
  return NoError;
}

static BXillError file_page_item(bXill *app, MYSQL *conn, json_t *item,
                                 void *ctx) {
  file_walk_ctx *w = ctx;
  XXH3_state_t *hash = XXH3_createState();
  if (hash == NULL) {
    return ErrorGeneric;
  }
  XXH3_64bits_reset(hash);
  BXFileMeta f = {0};
  f.id = bx_object_get_json_uint(item, "id", hash);
  f.uuid = bx_object_get_json_string(item, "uuid", hash);
  f.name = bx_object_get_json_string(item, "name", hash);
  f.size_in_bytes = bx_object_get_json_uint(item, "size_in_bytes", hash);
  f.extension = bx_object_get_json_string(item, "extension", hash);
  f.mime_type = bx_object_get_json_string(item, "mime_type", hash);
  f.uploader_email = bx_object_get_json_string(item, "uploader_email", hash);
  f.user_id = bx_object_get_json_uint(item, "user_id", hash);
  f.is_archived = bx_object_get_json_bool(item, "is_archived", hash);
  f.source_id = bx_object_get_json_uint(item, "source_id", hash);
  f.source_type = bx_object_get_json_string(item, "source_type", hash);
  f.is_referenced = bx_object_get_json_bool(item, "is_referenced", hash);
  f.created_at = bx_object_get_json_string(item, "created_at", hash);
  f.checksum = XXH3_64bits_digest(hash);
  XXH3_freeState(hash);

  if (!f.id.isset) {
    file_meta_free(&f);
    return NoError;
  }

  CacheState st = cache_check_item(w->cache, f.id.value, f.checksum);
  BXillError e = NoError;
  if (st != CacheOk) {
    e = persist_file(conn, &f);
    if (e == NoError) {
      cache_set_item(w->cache, f.id.value, f.checksum);
    }
  }
  if (e != ErrorSQLReconnect) {
    BXillError d = download_file(app, &f, w->cache_dir);
    if (e == NoError) {
      e = d;
    }
  }
  uint64_t fid = f.id.value;
  file_meta_free(&f);
  if (e == ErrorSQLReconnect) {
    return e;
  }
  if (e != NoError) {
    bx_log_error("File %lu sync failed: %d", (unsigned long)fid, (int)e);
    return NoError;
  }
  return NoError;
}

BXillError bx_file_walk_items(bXill *app, MYSQL *conn, Cache *cache) {
  bx_log_debug("BX Walk File Items");
  const char *cdir = bx_conf_get_string(app->conf, "cache-directory");
  char *cache_dir = NULL;
  if (cdir != NULL && cdir[0] != '\0') {
    cache_dir = strdup(cdir);
  } else {
    bx_log_error("cache-directory is not set; file blobs will not be stored");
  }
  bx_conf_release(app->conf, "cache-directory");

  BXString archived = {.type = BX_OBJECT_TYPE_STRING,
                       .isset = true,
                       .value = "all",
                       .value_len = 3};
  file_walk_ctx ctx = {.cache = cache, .cache_dir = cache_dir};
  BXillError e = bx_walk_pages(app, conn, WALK_FILES_PATH,
                               (BXGeneric *)&archived, NULL, file_page_item,
                               &ctx);
  free(cache_dir);
  return e;
}
