# OAuth Refresh Lock Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stop bx-sync and bx-proxy from posting the same Bexio refresh token, and stop a rejected refresh from exiting the process.

**Architecture:** `bx_oauth_decide` is a pure function that chooses use, refresh, or need-login. `bx_oauth_ensure` and the code exchange inside `bx_oauth_login` hold one MariaDB connection, take `GET_LOCK('bx-oauth-refresh', 20)`, re-read the three token rows, and post only when the stored access token still needs replacing. The three rows are committed in one transaction. bx-sync is the source. bx-proxy gets the same `bx_oauth.c` and `bx_oauth.h` by copy into `vendor/bx-sync`.

**Tech Stack:** C, MariaDB `GET_LOCK`, libmysqlclient, liboauth2, the existing bx-sync Makefile and the bx-proxy `make test` harness.

**Spec:** `bx-sync/docs/superpowers/specs/2026-10-13-oauth-refresh-lock-design.md`

## Global Constraints

- Lock name is `bx-oauth-refresh`. Timeout is 20 seconds (`BX_OAUTH_LOCK_TIMEOUT_SEC`). Named locks are server-wide.
- A usable access token is non-empty and `now + 60 < expires_at`. The 60 seconds are `BX_OAUTH_SKEW_SEC`.
- Write attempts are three total (the first try plus two retries).
- A missing or non-positive `expires_in` becomes 300 seconds (`BX_OAUTH_DEFAULT_TTL_SEC`).
- An expiry outside `0 .. INT_MAX` logs `OAuth expiry does not fit an integer configuration value`, fails that write attempt, and does not call `exit`.
- `invalid_grant` during refresh logs the Bexio error and `Refresh token was rejected. Run: bxsync --login <config>`. No `exit`, no `usleep`. The process stays up. Stored rows are left as they are.
- Lock failure logs `OAuth refresh lock not acquired` and returns failure. The process stays up.
- A locked re-read that decides use logs `OAuth access token was refreshed by the other process`.
- A missing refresh token logs `No Bexio refresh token. Run: bxsync --login <config>`.
- `tokens_dirty` means Keycloak has issued tokens that are in memory and not committed. Do not reload over them.
- `--login` holds the lock only around the code exchange and the save. The browser wait stays unlocked. A failed login commit returns `BX_OAUTH_FAILED` and does not keep `tokens_dirty`.
- Public signatures of `bx_oauth_ensure`, `bx_oauth_login`, `bx_oauth_startup`, and `bx_oauth_reload` stay the same. `bx_oauth_decide` is the new public function.
- `bx_conf_put_database` stays unchanged and is no longer called for the three token rows.
- Live `bx-sync/src/bx_net.c` is not modified. Vendored `bx_net.c` keeps calling `bx_oauth_reload` before `bx_oauth_ensure`. No other vendored file is refreshed.
- No schema change. No new configuration key. `init/bxsync.service` is not edited.
- `make test` does not need MariaDB or Bexio. It checks `bx_oauth_decide` only.
- bx-sync and bx-proxy are separate git repositories. Commit each task in the repository whose files it changes.
- Every commit uses the Grok identity below. Do not set `git config`. Do not use `--no-gpg-sign`. Do not commit untracked build products, cores, logs, or `src/hash.c`.

```bash
GIT_AUTHOR_NAME='Grok' GIT_AUTHOR_EMAIL='grok@x.ai' \
GIT_COMMITTER_NAME='Grok' GIT_COMMITTER_EMAIL='grok@x.ai' \
git -c user.signingkey=443B7D62D5B59D50FB5BF7B7BF7714B55F38B753 \
    -c commit.gpgsign=true commit -S -m "…"
```

---

### Task 1: `bx_oauth_decide`

**Files:**
- Create: `bx-sync/test/oauth_decide.c`
- Modify: `bx-sync/src/include/bx_oauth.h`
- Modify: `bx-sync/src/bx_oauth.c`
- Modify: `bx-sync/Makefile`
- Test: `bx-sync/test/oauth_decide.c`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces: `BXOauthAction` with `BX_OAUTH_ACT_USE`, `BX_OAUTH_ACT_REFRESH`, `BX_OAUTH_ACT_NEED_LOGIN`, and

```c
BXOauthAction bx_oauth_decide(bool force, const char *rejected,
                              const char *access, const char *refresh,
                              time_t expires_at, time_t now);
```

An access or refresh string of length 0 is missing. A missing `rejected` means there was no access token on the 401 path. Return use when `force` is false and the access token is usable. Return use when `force` is true, the access token is usable, and its string differs from `rejected` (a missing `rejected` differs). Return need-login when the call would otherwise refresh and the refresh token is missing. Return refresh in every other case where the refresh token is present, including a 401 whose access token is still the rejected string.

- [ ] **Step 1: Write the failing test**

Create `bx-sync/test/oauth_decide.c`:

```c
#include "include/bx_oauth.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void check(int cond, const char *file, int line, const char *what) {
  if (!cond) {
    fprintf(stderr, "FAIL %s:%d %s\n", file, line, what);
    fails++;
  }
}

#define CHECK(cond) check((cond), __FILE__, __LINE__, #cond)

static void expect(bool force, const char *rejected, const char *access,
                   const char *refresh, time_t expires_at, time_t now,
                   BXOauthAction want, const char *what) {
  BXOauthAction got;

  got = bx_oauth_decide(force, rejected, access, refresh, expires_at, now);
  if (got != want) {
    fprintf(stderr, "FAIL %s got %d want %d\n", what, (int)got, (int)want);
    fails++;
  }
}

int main(void) {
  const time_t now = 1000000;

  expect(false, NULL, "access", "refresh", now + 61, now, BX_OAUTH_ACT_USE,
         "valid access");
  expect(false, NULL, "access", NULL, now + 61, now, BX_OAUTH_ACT_USE,
         "valid access without refresh");
  expect(false, NULL, "access", "refresh", now + 60, now, BX_OAUTH_ACT_REFRESH,
         "skew boundary");
  expect(false, NULL, "access", "refresh", now, now, BX_OAUTH_ACT_REFRESH,
         "expired access");
  expect(false, NULL, NULL, "refresh", now, now, BX_OAUTH_ACT_REFRESH,
         "missing access");
  expect(false, NULL, NULL, NULL, now, now, BX_OAUTH_ACT_NEED_LOGIN,
         "nothing stored");
  expect(false, NULL, "access", NULL, now, now, BX_OAUTH_ACT_NEED_LOGIN,
         "expired access and no refresh");
  expect(true, "access", "access", "refresh", now + 61, now,
         BX_OAUTH_ACT_REFRESH, "401 on the same access token");
  expect(true, "old", "access", "refresh", now + 61, now, BX_OAUTH_ACT_USE,
         "401 replaced by the other process");
  expect(true, "old", "access", "refresh", now, now, BX_OAUTH_ACT_REFRESH,
         "replaced token already expired");
  expect(true, "old", "access", NULL, now + 61, now, BX_OAUTH_ACT_USE,
         "replaced token and no refresh");
  expect(true, "access", "access", NULL, now + 61, now, BX_OAUTH_ACT_NEED_LOGIN,
         "401 and no refresh");
  expect(false, NULL, "", "refresh", now + 61, now, BX_OAUTH_ACT_REFRESH,
         "empty access");
  expect(false, NULL, "access", "", now, now, BX_OAUTH_ACT_NEED_LOGIN,
         "empty refresh");
  expect(true, NULL, "access", "refresh", now + 61, now, BX_OAUTH_ACT_USE,
         "force with no rejected token");
  expect(true, "", "access", NULL, now + 61, now, BX_OAUTH_ACT_USE,
         "empty rejected differs");

  if (fails) {
    fprintf(stderr, "%d oauth decide checks failed\n", fails);
    return 1;
  }
  printf("oauth decide ok\n");
  return 0;
}
```

Add this target to `bx-sync/Makefile` before the `clean` target. `.PHONY` must list `test`.

```make
.PHONY: test

TEST_OBJ=$(filter-out build/main.o,$(OBJFILES))

build/test_oauth_decide: test/oauth_decide.c $(TEST_OBJ)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/oauth_decide.c $(TEST_OBJ) -o $@ $(LIBS)

test: build/test_oauth_decide
	./build/test_oauth_decide
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-sync test`

Expected: compile error containing `bx_oauth_decide` undeclared, or `BX_OAUTH_ACT_USE` undeclared. The binary must not print `oauth decide ok`.

- [ ] **Step 3: Add the function**

In `bx-sync/src/include/bx_oauth.h`, add `#include <time.h>` next to the other includes, and add this block after the `BXOauthStatus` enum:

```c
typedef enum e_BXOauthAction {
  BX_OAUTH_ACT_USE = 0,
  BX_OAUTH_ACT_REFRESH,
  BX_OAUTH_ACT_NEED_LOGIN
} BXOauthAction;

/* Usable access token: non-empty and now + BX_OAUTH_SKEW_SEC < expires_at.
 * The skew is 60. A string of length 0 is missing. A missing rejected
 * argument differs from any stored access token. */
BXOauthAction bx_oauth_decide(bool force, const char *rejected,
                              const char *access, const char *refresh,
                              time_t expires_at, time_t now);
```

In `bx-sync/src/bx_oauth.c`, add this above `bx_oauth_ensure`. `BX_OAUTH_SKEW_SEC` is already 60 in that file.

```c
static bool token_present(const char *s) {
  return s != NULL && s[0] != '\0';
}

BXOauthAction bx_oauth_decide(bool force, const char *rejected,
                              const char *access, const char *refresh,
                              time_t expires_at, time_t now) {
  bool access_ok = false;
  bool refresh_ok = false;

  access_ok = token_present(access) &&
              now + (time_t)BX_OAUTH_SKEW_SEC < expires_at;
  refresh_ok = token_present(refresh);
  if (!force && access_ok) {
    return BX_OAUTH_ACT_USE;
  }
  if (force && access_ok &&
      (!token_present(rejected) || strcmp(rejected, access) != 0)) {
    return BX_OAUTH_ACT_USE;
  }
  if (!refresh_ok) {
    return BX_OAUTH_ACT_NEED_LOGIN;
  }
  return BX_OAUTH_ACT_REFRESH;
}
```

Leave `bx_oauth_ensure` unchanged in this task.

- [ ] **Step 4: Run the test to verify it passes**

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-sync test`

Expected: `oauth decide ok` and exit code 0.

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-sync`

Expected: `bxsync` links.

- [ ] **Step 5: Commit**

```bash
cd /home/etienne/Projets/work/bizcuit/bx-sync
git add src/include/bx_oauth.h src/bx_oauth.c test/oauth_decide.c Makefile
GIT_AUTHOR_NAME='Grok' GIT_AUTHOR_EMAIL='grok@x.ai' \
GIT_COMMITTER_NAME='Grok' GIT_COMMITTER_EMAIL='grok@x.ai' \
git -c user.signingkey=443B7D62D5B59D50FB5BF7B7BF7714B55F38B753 \
    -c commit.gpgsign=true commit -S -m "Add bx_oauth_decide for the shared Bexio refresh."
```

---

### Task 2: Hold `GET_LOCK` around refresh and login

**Files:**
- Modify: `bx-sync/src/bx_oauth.c`
- Modify: `bx-sync/README.md`
- Test: `bx-sync/test/oauth_decide.c` (must still pass; no new cases)

**Interfaces:**
- Consumes: `bx_oauth_decide` from Task 1.
- Produces: `bx_oauth_ensure` and `bx_oauth_login` take `GET_LOCK`. `bx_oauth_reload` exists and returns success without reading the database when `tokens_dirty` is set. `post_token` does not call `exit`. The three token rows are written on the lock connection.

The struct gains three fields. The upsert reads those struct fields, so `post_token` installs the new access token, refresh token, and expiry into the struct before the write. A failed commit then still has the only copy of the new refresh token in memory. The conf cache is updated with `bx_conf_set` only after `COMMIT`. A failed `bx_conf_set` after a successful commit is logged and still returns success. Do not set `tokens_dirty` after a commit that succeeded, or the next ensure would post the new refresh token again.

- [ ] **Step 1: Extend the struct and includes**

In `bx-sync/src/bx_oauth.c`, add:

```c
#include <assert.h>
#include <mysql/mysql.h>
```

Add the constants next to `BX_OAUTH_SKEW_SEC`:

```c
#define BX_OAUTH_LOCK_NAME "bx-oauth-refresh"
#define BX_OAUTH_LOCK_TIMEOUT_SEC 20
#define BX_OAUTH_WRITE_ATTEMPTS 3
```

Add these fields to `struct s_BXOAuth`:

```c
  MYSQL *lock_conn;
  bool lock_held;
  bool tokens_dirty;
```

`calloc` in `bx_oauth_init` already zeroes them.

- [ ] **Step 2: Replace `save_tokens` with the locked write**

Delete `save_tokens`. Add the helpers below. They must be above `post_token`.

```c
static void oauth_note(const char *msg) {
  bx_log_info("%s", msg);
}

static bool access_usable(const char *access, time_t expires_at, time_t now) {
  return token_present(access) && now + (time_t)BX_OAUTH_SKEW_SEC < expires_at;
}

static void free_result(MYSQL *conn) {
  MYSQL_RES *result = NULL;

  result = mysql_store_result(conn);
  if (result != NULL) {
    mysql_free_result(result);
  }
}

static MYSQL *oauth_connect(BXOAuth *oa) {
  const char *host = NULL;
  const char *user = NULL;
  const char *password = NULL;
  const char *database = NULL;
  MYSQL *conn = NULL;
  unsigned int timeout = 5;

  host = bx_conf_get_string(oa->conf, "mysql-host");
  user = bx_conf_get_string(oa->conf, "mysql-user");
  password = bx_conf_get_string(oa->conf, "mysql-password");
  database = bx_conf_get_string(oa->conf, "mysql-database");
  if (host == NULL || user == NULL || database == NULL) {
    oauth_fail("mysql-host, mysql-user and mysql-database are required");
    return NULL;
  }
  conn = mysql_init(NULL);
  if (conn == NULL) {
    oauth_fail("mysql_init failed");
    return NULL;
  }
  mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
  if (!mysql_real_connect(conn, host, user, password, database, 0, NULL, 0)) {
    oauth_fail(mysql_error(conn));
    mysql_close(conn);
    return NULL;
  }
  mysql_set_character_set(conn, "utf8mb4");
  return conn;
}

static void oauth_unlock(BXOAuth *oa) {
  if (oa->lock_conn != NULL && oa->lock_held) {
    mysql_query(oa->lock_conn, "SELECT RELEASE_LOCK('" BX_OAUTH_LOCK_NAME "')");
    free_result(oa->lock_conn);
    oa->lock_held = false;
  }
  if (oa->lock_conn != NULL) {
    mysql_close(oa->lock_conn);
    oa->lock_conn = NULL;
  }
}

static bool oauth_lock(BXOAuth *oa) {
  MYSQL_RES *result = NULL;
  MYSQL_ROW row;
  char sql[64];
  bool got = false;

  if (oa->lock_conn != NULL) {
    oauth_unlock(oa);
  }
  oa->lock_conn = oauth_connect(oa);
  if (oa->lock_conn == NULL) {
    return false;
  }
  snprintf(sql, sizeof(sql), "SELECT GET_LOCK('%s', %d)", BX_OAUTH_LOCK_NAME,
           BX_OAUTH_LOCK_TIMEOUT_SEC);
  if (mysql_query(oa->lock_conn, sql) != 0) {
    oauth_unlock(oa);
    return false;
  }
  result = mysql_store_result(oa->lock_conn);
  if (result == NULL) {
    oauth_unlock(oa);
    return false;
  }
  row = mysql_fetch_row(result);
  got = row != NULL && row[0] != NULL && strcmp(row[0], "1") == 0;
  mysql_free_result(result);
  if (!got) {
    oauth_unlock(oa);
    return false;
  }
  oa->lock_held = true;
  return true;
}

static bool oauth_upsert(MYSQL_STMT *stmt, const char *name,
                         const char *type_name, int ival, my_bool ival_null,
                         char *cval, my_bool cval_null) {
  MYSQL_BIND bind[4];
  unsigned long name_len = 0;
  unsigned long type_len = 0;
  unsigned long cval_len = 0;

  name_len = strlen(name);
  type_len = strlen(type_name);
  if (cval == NULL) {
    cval = (char *)"";
    cval_len = 0;
  } else {
    cval_len = strlen(cval);
  }
  memset(bind, 0, sizeof(bind));
  bind[0].buffer_type = MYSQL_TYPE_STRING;
  bind[0].buffer = (char *)name;
  bind[0].buffer_length = name_len;
  bind[0].length = &name_len;
  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer = (char *)type_name;
  bind[1].buffer_length = type_len;
  bind[1].length = &type_len;
  bind[2].buffer_type = MYSQL_TYPE_LONG;
  bind[2].buffer = &ival;
  bind[2].is_null = &ival_null;
  bind[3].buffer_type = MYSQL_TYPE_STRING;
  bind[3].buffer = cval;
  bind[3].buffer_length = cval_len + 1;
  bind[3].length = &cval_len;
  bind[3].is_null = &cval_null;
  if (mysql_stmt_bind_param(stmt, bind) != 0 || mysql_stmt_execute(stmt) != 0) {
    return false;
  }
  return true;
}

static bool oauth_write_once(BXOAuth *oa) {
  static const char *sql =
      "INSERT INTO _configuration (name, type, ival, cval) "
      "VALUES (?, ?, ?, ?) "
      "ON DUPLICATE KEY UPDATE type = VALUES(type), ival = VALUES(ival), "
      "cval = VALUES(cval), bval = NULL, fval = NULL";
  MYSQL_STMT *stmt = NULL;
  int exp = 0;
  bool ok = false;

  assert(oa->lock_conn != NULL);
  assert(oa->lock_held);
  if (oa->access_token == NULL || oa->refresh_token == NULL) {
    return false;
  }
  if (oa->expires_at < 0 || oa->expires_at > (time_t)INT_MAX) {
    oauth_fail("OAuth expiry does not fit an integer configuration value");
    return false;
  }
  exp = (int)oa->expires_at;
  if (mysql_query(oa->lock_conn, "START TRANSACTION") != 0) {
    return false;
  }
  stmt = mysql_stmt_init(oa->lock_conn);
  if (stmt == NULL || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
    mysql_query(oa->lock_conn, "ROLLBACK");
    if (stmt != NULL) {
      mysql_stmt_close(stmt);
    }
    return false;
  }
  /* Refresh token first. The transaction hides that order from other
   * connections until COMMIT. */
  if (oauth_upsert(stmt, "oauth-refresh-token", "string", 0, 1,
                   oa->refresh_token, 0) &&
      oauth_upsert(stmt, "oauth-access-token", "string", 0, 1, oa->access_token,
                   0) &&
      oauth_upsert(stmt, "oauth-expires-at", "integer", exp, 0, NULL, 1) &&
      mysql_query(oa->lock_conn, "COMMIT") == 0) {
    ok = true;
  }
  if (!ok) {
    mysql_query(oa->lock_conn, "ROLLBACK");
  }
  mysql_stmt_close(stmt);
  if (!ok) {
    return false;
  }
  if (!bx_conf_set(oa->conf, "oauth-refresh-token", oa->refresh_token,
                   StringType) ||
      !bx_conf_set(oa->conf, "oauth-access-token", oa->access_token,
                   StringType) ||
      !bx_conf_set(oa->conf, "oauth-expires-at", &exp, IntegerType)) {
    oauth_fail("Could not keep the OAuth tokens in memory");
  }
  return true;
}

static bool oauth_write_tokens(BXOAuth *oa) {
  int attempt = 0;

  for (attempt = 0; attempt < BX_OAUTH_WRITE_ATTEMPTS; attempt++) {
    if (oauth_write_once(oa)) {
      oa->tokens_dirty = false;
      return true;
    }
  }
  oa->tokens_dirty = true;
  oauth_fail("Could not save the OAuth tokens");
  return false;
}
```

Move the Task 1 `token_present` function to just above `access_usable`. The helpers call it, and it is currently defined later in the file.

- [ ] **Step 3: Read the three rows on the lock connection**

Add this helper next to the write helpers.

```c
static bool replace_token(char **slot, const char *value) {
  char *copy = NULL;

  if (token_present(value)) {
    copy = dup_str(value);
    if (copy == NULL) {
      return false;
    }
  }
  if (*slot != NULL) {
    memset(*slot, 0, strlen(*slot));
    free(*slot);
  }
  *slot = copy;
  return true;
}

static bool oauth_read_tokens(BXOAuth *oa) {
  static const char *sql =
      "SELECT name, ival, cval FROM _configuration WHERE name IN "
      "('oauth-access-token', 'oauth-refresh-token', 'oauth-expires-at')";
  MYSQL_RES *result = NULL;
  MYSQL_ROW row;
  char *access = NULL;
  char *refresh = NULL;
  int expiry = 0;
  bool saw_expiry = false;
  bool ok = true;

  assert(oa->lock_conn != NULL);
  assert(oa->lock_held);
  if (mysql_query(oa->lock_conn, sql) != 0) {
    oauth_fail(mysql_error(oa->lock_conn));
    return false;
  }
  result = mysql_store_result(oa->lock_conn);
  if (result == NULL) {
    oauth_fail(mysql_error(oa->lock_conn));
    return false;
  }
  while ((row = mysql_fetch_row(result)) != NULL) {
    if (row[0] == NULL) {
      continue;
    }
    if (strcmp(row[0], "oauth-access-token") == 0) {
      free(access);
      access = row[2] != NULL ? dup_str(row[2]) : NULL;
      if (row[2] != NULL && access == NULL) {
        ok = false;
      }
    } else if (strcmp(row[0], "oauth-refresh-token") == 0) {
      free(refresh);
      refresh = row[2] != NULL ? dup_str(row[2]) : NULL;
      if (row[2] != NULL && refresh == NULL) {
        ok = false;
      }
    } else if (strcmp(row[0], "oauth-expires-at") == 0 && row[1] != NULL) {
      expiry = atoi(row[1]);
      saw_expiry = true;
    }
  }
  mysql_free_result(result);
  if (!ok) {
    free(access);
    free(refresh);
    oauth_fail("Out of memory while reading the OAuth tokens");
    return false;
  }
  if (token_present(access)) {
    bx_conf_set(oa->conf, "oauth-access-token", access, StringType);
  } else {
    bx_conf_release(oa->conf, "oauth-access-token");
  }
  if (token_present(refresh)) {
    bx_conf_set(oa->conf, "oauth-refresh-token", refresh, StringType);
  } else {
    bx_conf_release(oa->conf, "oauth-refresh-token");
  }
  if (saw_expiry) {
    bx_conf_set(oa->conf, "oauth-expires-at", &expiry, IntegerType);
  } else {
    bx_conf_release(oa->conf, "oauth-expires-at");
    expiry = 0;
  }
  if (!replace_token(&oa->access_token, access) ||
      !replace_token(&oa->refresh_token, refresh)) {
    free(access);
    free(refresh);
    oauth_fail("Out of memory while reading the OAuth tokens");
    return false;
  }
  oa->expires_at = (time_t)expiry;
  free(access);
  free(refresh);
  return true;
}
```

- [ ] **Step 4: Stop `post_token` from exiting**

In `post_token`, replace the `invalid_grant` block:

```c
    if (reauth && oa->refreshing) {
      oauth_fail("Refresh token was rejected. Run: bxsync --login <config>");
      usleep(100000);
      exit(EXIT_FAILURE);
    }
```

with:

```c
    if (reauth && oa->refreshing) {
      oauth_fail("Refresh token was rejected. Run: bxsync --login <config>");
      oa->tokens_dirty = false;
    }
```

Put this private result above `post_token`. A transport failure and `invalid_grant` are `POST_FAILED`. They must not be treated as a successful call just because an older unsaved grant is still in memory. `POST_UNSAVED` is only the case where this response installed new tokens and the commit failed.

```c
typedef enum e_PostResult {
  POST_OK = 0,
  POST_FAILED,
  POST_UNSAVED
} PostResult;
```

Change `static bool post_token` to `static PostResult post_token`. Replace every `return false` in that function with `return POST_FAILED`. Replace the `save_tokens` call and the `return true` after it with:

```c
  if (!oauth_write_tokens(oa)) {
    return POST_UNSAVED;
  }
  return POST_OK;
```

The lines that install `oa->access_token`, `oa->refresh_token`, and `oa->expires_at` stay above the write. `oauth_write_tokens` asserts `lock_held`, so both callers must hold the lock before `post_token`.

Replace `refresh_tokens` with:

```c
static PostResult refresh_tokens(BXOAuth *oa) {
  oauth2_nv_list_t *form = NULL;
  PostResult posted = POST_FAILED;

  if (oa->refresh_token == NULL) {
    return POST_FAILED;
  }
  form = oauth2_nv_list_init(oa->log);
  if (form == NULL) {
    return POST_FAILED;
  }
  oauth2_nv_list_add(oa->log, form, "grant_type", "refresh_token");
  oauth2_nv_list_add(oa->log, form, "refresh_token", oa->refresh_token);
  oauth2_nv_list_add(oa->log, form, "client_id", oa->client_id);
  oauth2_nv_list_add(oa->log, form, "scope", oa->scope);
  oa->refreshing = true;
  posted = post_token(oa, form);
  oa->refreshing = false;
  oauth2_nv_list_free(oa->log, form);
  return posted;
}
```

- [ ] **Step 5: Replace `bx_oauth_ensure` and add `bx_oauth_reload`**

Add `bx_oauth_reload` above `bx_oauth_startup`. When `tokens_dirty` is set, skip the database read. The proxy calls this before every ensure. A read here would replace the unsaved refresh token.

```c
bool bx_oauth_reload(BXOAuth *oauth) {
  if (oauth == NULL || oauth->conf == NULL) {
    return false;
  }
  if (oauth->tokens_dirty) {
    return true;
  }
  if (!bx_conf_load_database(oauth->conf)) {
    return false;
  }
  return load_tokens(oauth) == BX_OAUTH_OK;
}
```

Declare it in `bx-sync/src/include/bx_oauth.h` between `bx_oauth_startup` and `bx_oauth_login`:

```c
/* Re-read _configuration, then the tokens, so a concurrent bxsync refresh wins.
 * A process that still holds an uncommitted grant keeps that grant. */
bool bx_oauth_reload(BXOAuth *oauth);
```

Replace `bx_oauth_ensure` with:

```c
bool bx_oauth_ensure(BXOAuth *oauth, bool force) {
  char *rejected = NULL;
  BXOauthAction action = BX_OAUTH_ACT_NEED_LOGIN;
  PostResult posted = POST_FAILED;
  bool ok = false;
  time_t now = 0;

  if (oauth == NULL) {
    return false;
  }
  now = time(NULL);
  if (!oauth->tokens_dirty && !force &&
      access_usable(oauth->access_token, oauth->expires_at, now)) {
    return true;
  }
  rejected = token_present(oauth->access_token) ? dup_str(oauth->access_token)
                                                : NULL;
  if (token_present(oauth->access_token) && rejected == NULL) {
    oauth_fail("Out of memory while refreshing the OAuth token");
    return false;
  }
  if (!oauth_lock(oauth)) {
    oauth_fail("OAuth refresh lock not acquired");
    free(rejected);
    return false;
  }
  if (oauth->tokens_dirty && !force &&
      access_usable(oauth->access_token, oauth->expires_at, time(NULL))) {
    oauth_write_tokens(oauth);
    oauth_unlock(oauth);
    free(rejected);
    return true;
  }
  if (oauth->tokens_dirty) {
    if (!token_present(oauth->refresh_token)) {
      oauth_fail("No Bexio refresh token. Run: bxsync --login <config>");
      oauth_unlock(oauth);
      free(rejected);
      return false;
    }
    posted = refresh_tokens(oauth);
    ok = posted == POST_OK ||
         (posted == POST_UNSAVED &&
          access_usable(oauth->access_token, oauth->expires_at, time(NULL)));
    oauth_unlock(oauth);
    free(rejected);
    return ok;
  }
  if (!oauth_read_tokens(oauth)) {
    oauth_unlock(oauth);
    free(rejected);
    return false;
  }
  action = bx_oauth_decide(force, rejected, oauth->access_token,
                           oauth->refresh_token, oauth->expires_at, time(NULL));
  free(rejected);
  rejected = NULL;
  if (action == BX_OAUTH_ACT_USE) {
    oauth_note("OAuth access token was refreshed by the other process");
    oauth_unlock(oauth);
    return true;
  }
  if (action == BX_OAUTH_ACT_NEED_LOGIN) {
    oauth_fail("No Bexio refresh token. Run: bxsync --login <config>");
    oauth_unlock(oauth);
    return false;
  }
  posted = refresh_tokens(oauth);
  ok = posted == POST_OK ||
       (posted == POST_UNSAVED &&
        access_usable(oauth->access_token, oauth->expires_at, time(NULL)));
  oauth_unlock(oauth);
  return ok;
}
```

`access_usable` and `token_present` must be declared above this function.

In `bx_oauth_destroy`, before freeing the token strings, release a lock the process still holds:

```c
  oauth_unlock(oa);
```

- [ ] **Step 6: Lock only the code exchange**

Replace `exchange_code` with:

```c
static bool exchange_code(BXOAuth *oa, const char *code, const char *verifier) {
  oauth2_nv_list_t *form = NULL;
  bool ok = false;

  form = oauth2_nv_list_init(oa->log);
  if (form == NULL) {
    return false;
  }
  oauth2_nv_list_add(oa->log, form, "grant_type", "authorization_code");
  oauth2_nv_list_add(oa->log, form, "code", code);
  oauth2_nv_list_add(oa->log, form, "redirect_uri", oa->redirect_uri);
  oauth2_nv_list_add(oa->log, form, "client_id", oa->client_id);
  oauth2_nv_list_add(oa->log, form, "code_verifier", verifier);
  if (!oauth_lock(oa)) {
    oauth_fail("OAuth refresh lock not acquired");
    oauth2_nv_list_free(oa->log, form);
    return false;
  }
  ok = post_token(oa, form) == POST_OK;
  oauth_unlock(oa);
  if (!ok) {
    oa->tokens_dirty = false;
  }
  oauth2_nv_list_free(oa->log, form);
  return ok;
}
```

The `poll` loop in `bx_oauth_login` stays above this call and does not take the lock. `main` still returns `EXIT_SUCCESS` only when `bx_oauth_login` returns `BX_OAUTH_OK`. Do not change `src/main.c`.

- [ ] **Step 7: Update the bx-sync README**

In the English configuration section, replace:

```markdown
and `oauth-expires-at` (`integer`). `--login` and each refresh write those
rows.
```

with:

```markdown
and `oauth-expires-at` (`integer`). `--login` and each refresh write those
rows in one transaction. bx-sync and bx-proxy share them. A refresh takes
the MariaDB named lock `bx-oauth-refresh` for up to 20 seconds, re-reads the
rows, and posts only when the stored access token still needs replacing.
The process that finds a token stored by the other one logs
`OAuth access token was refreshed by the other process`.
```

In the English authentication section, replace:

```markdown
`--login` is not a systemd command. If a refresh is rejected,
stop the service, run `--login` again, then start it. `oauth-scope` is
```

with:

```markdown
`--login` is not a systemd command. A rejected refresh is logged
(`Refresh token was rejected. Run: bxsync --login <config>`) and the
process keeps running. Run `--login` while the daemon is up. The daemon
loads the new rows on its next refresh. `oauth-scope` is
```

In the French authentication section, replace:

```markdown
Les jetons sont écrits dans `_configuration`. Ensuite le démon ne fait que les
renouveler. `--login` ne se lance pas via systemd. Si un
renouvellement est refusé, arrêter le service, relancer `--login`, puis
redémarrer. `oauth-scope` est optionnel ; sans cette clé, les scopes de
```

with:

```markdown
Les jetons sont écrits dans `_configuration`. bx-sync et bx-proxy les
partagent. Un renouvellement prend le verrou MariaDB `bx-oauth-refresh`
(20 secondes), relit les lignes, et ne poste un nouveau jeton que si le
jeton d'accès stocké doit encore être remplacé. `--login` ne se lance pas
via systemd. Un renouvellement refusé est journalisé
(`Refresh token was rejected. Run: bxsync --login <config>`) et le
processus continue. Lancer `--login` pendant que le démon tourne. Il
charge les nouvelles lignes à son prochain renouvellement. `oauth-scope`
est optionnel ; sans cette clé, les scopes de
```

- [ ] **Step 8: Verify**

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-sync test`

Expected: `oauth decide ok` and exit code 0.

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-sync`

Expected: `bxsync` links.

Run these from `bx-sync/src` and expect no output:

```bash
grep -n "exit\\|usleep\\|bx_conf_put_database" bx_oauth.c
```

Expected: no matches. `GET_LOCK` and `RELEASE_LOCK` are the lock calls, and `oauth_write_tokens` is the only token write.

Confirm `src/bx_net.c` has no diff:

```bash
git -C /home/etienne/Projets/work/bizcuit/bx-sync diff -- src/bx_net.c src/main.c init/bxsync.service
```

Expected: empty.

- [ ] **Step 9: Commit**

```bash
cd /home/etienne/Projets/work/bizcuit/bx-sync
git add src/bx_oauth.c src/include/bx_oauth.h README.md
GIT_AUTHOR_NAME='Grok' GIT_AUTHOR_EMAIL='grok@x.ai' \
GIT_COMMITTER_NAME='Grok' GIT_COMMITTER_EMAIL='grok@x.ai' \
git -c user.signingkey=443B7D62D5B59D50FB5BF7B7BF7714B55F38B753 \
    -c commit.gpgsign=true commit -S -m "Lock Bexio OAuth refresh with MariaDB GET_LOCK."
```

---

### Task 3: Copy the locked OAuth code into bx-proxy

**Files:**
- Modify: `bx-proxy/vendor/bx-sync/src/bx_oauth.c` (copy of the bx-sync file)
- Modify: `bx-proxy/vendor/bx-sync/src/include/bx_oauth.h` (copy of the bx-sync file)
- Create: `bx-proxy/test/oauth_decide.c` (copy of the bx-sync test)
- Modify: `bx-proxy/Makefile`
- Modify: `bx-proxy/README.md`
- Test: `bx-proxy/test/oauth_decide.c`

**Interfaces:**
- Consumes: the bx-sync `bx_oauth.c`, `bx_oauth.h`, and `test/oauth_decide.c` from Tasks 1 and 2.
- Produces: the proxy test calls the vendored `bx_oauth_decide`. Vendored `bx_net.c` is untouched and still calls `bx_oauth_reload` before `bx_oauth_ensure`.

- [ ] **Step 1: Copy the source and the test**

```bash
cp /home/etienne/Projets/work/bizcuit/bx-sync/src/bx_oauth.c \
   /home/etienne/Projets/work/bizcuit/bx-proxy/vendor/bx-sync/src/bx_oauth.c
cp /home/etienne/Projets/work/bizcuit/bx-sync/src/include/bx_oauth.h \
   /home/etienne/Projets/work/bizcuit/bx-proxy/vendor/bx-sync/src/include/bx_oauth.h
cp /home/etienne/Projets/work/bizcuit/bx-sync/test/oauth_decide.c \
   /home/etienne/Projets/work/bizcuit/bx-proxy/test/oauth_decide.c
diff -q /home/etienne/Projets/work/bizcuit/bx-sync/src/bx_oauth.c \
        /home/etienne/Projets/work/bizcuit/bx-proxy/vendor/bx-sync/src/bx_oauth.c
diff -q /home/etienne/Projets/work/bizcuit/bx-sync/src/include/bx_oauth.h \
        /home/etienne/Projets/work/bizcuit/bx-proxy/vendor/bx-sync/src/include/bx_oauth.h
diff -q /home/etienne/Projets/work/bizcuit/bx-sync/test/oauth_decide.c \
        /home/etienne/Projets/work/bizcuit/bx-proxy/test/oauth_decide.c
```

Expected: all three `diff -q` commands print nothing and exit 0.

Do not copy any other file. `vendor/bx-sync/src/bx_net.c` must still contain:

```c
  if (!bx_oauth_reload(net->oauth) || !bx_oauth_ensure(net->oauth, force)) {
```

- [ ] **Step 2: Teach `make test` to compile the vendored decide function**

In `bx-proxy/Makefile`, add the object list and the test next to `build/test_fresh`. `$(LIBS)` follows the objects so liboauth2, MariaDB, curl, jansson, and pthread resolve the vendored objects.

```make
VENDOR_OBJ=$(patsubst %.c,build/%.o,$(VENDOR_SRC))

build/test_oauth_decide: test/oauth_decide.c $(VENDOR_OBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/oauth_decide.c $(VENDOR_OBJ) -o $@ $(LIBS)
```

Change the `test` target to:

```make
test: build/test_route build/test_json build/test_fresh build/test_oauth_decide
	./build/test_route
	./build/test_json
	./build/test_fresh
	./build/test_oauth_decide
```

- [ ] **Step 3: Run the proxy tests**

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-proxy test`

Expected, in order: the existing route, JSON, and freshness lines, then `oauth decide ok`, exit code 0.

Run: `make -C /home/etienne/Projets/work/bizcuit/bx-proxy`

Expected: `bx-proxy` links.

- [ ] **Step 4: Update the proxy README**

Replace:

```markdown
`bxsync --login` stores the OAuth tokens in `_configuration` (`oauth-access-token`, `oauth-refresh-token`, `oauth-expires-at`). bx-proxy reloads those rows before each Bexio call and refreshes them when they are due. Login is not implemented here.
```

with:

```markdown
`bxsync --login` stores the OAuth tokens in `_configuration` (`oauth-access-token`, `oauth-refresh-token`, `oauth-expires-at`). bx-proxy reloads those rows before each Bexio call. When the access token is due, bx-proxy and bx-sync take the MariaDB named lock `bx-oauth-refresh`, re-read the rows, and only one of them posts the refresh. Login is not implemented here.
```

Replace:

```markdown
`vendor/bx-sync` is a snapshot of bx-sync `5a6dcad`. The proxy does not compile against a `../bx-sync` checkout. Before each Bexio call it re-reads `_configuration`, so a token written by bxsync is picked up. Two processes can still refresh the same token at the same moment.
```

with:

```markdown
`vendor/bx-sync` is a snapshot of bx-sync. `bx_oauth.c` and `bx_oauth.h` match the bx-sync copies that take `bx-oauth-refresh`. The proxy does not compile against a `../bx-sync` checkout. Before each Bexio call it re-reads `_configuration`. The refresh itself takes the lock, so the two processes do not post the same refresh token.
```

- [ ] **Step 5: Commit**

```bash
cd /home/etienne/Projets/work/bizcuit/bx-proxy
git add vendor/bx-sync/src/bx_oauth.c vendor/bx-sync/src/include/bx_oauth.h \
        test/oauth_decide.c Makefile README.md
git diff --cached --stat
GIT_AUTHOR_NAME='Grok' GIT_AUTHOR_EMAIL='grok@x.ai' \
GIT_COMMITTER_NAME='Grok' GIT_COMMITTER_EMAIL='grok@x.ai' \
git -c user.signingkey=443B7D62D5B59D50FB5BF7B7BF7714B55F38B753 \
    -c commit.gpgsign=true commit -S -m "Use the locked Bexio OAuth refresh from bx-sync."
```

`git diff --cached --stat` must list only those five paths. Do not stage any other vendored file.

---

### Task 4: Manual checks

These need one shared MariaDB and a real Bexio grant. `make test` does not cover them. If that database is not configured in this environment, skip the steps and report that the manual checks were not run. Do not invent a grant and do not point either process at a database you were not given.

- [ ] **Step 1: One poster**

Run bx-sync and bx-proxy against the same database. Clear the expiry:

```sql
UPDATE _configuration SET ival = 0 WHERE name = 'oauth-expires-at';
```

The running `log-level` must be at least 8 (`LOG_LEVEL_INFO`), or the reuse line is dropped. Cause one Bexio call from each process. Expected: one process posts the refresh. The other logs `OAuth access token was refreshed by the other process`. Both processes are still running. `_configuration` has one new refresh token, one new access token, and an expiry in the future.

- [ ] **Step 2: Rejected refresh stays up**

Stop both processes. Store a refresh token that Bexio will reject, then start both again.

```sql
UPDATE _configuration SET cval = 'rejected' WHERE name = 'oauth-refresh-token';
```

Expected: the log contains `Refresh token was rejected. Run: bxsync --login <config>`. Neither process has exited. Restore a working grant with `bxsync --login` before leaving the database in this state.
