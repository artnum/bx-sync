# OAuth refresh lock

Date: 2026-10-13

bx-sync and bx-proxy share one Bexio OAuth grant. The refresh token, access token, and expiry live in `_configuration` (`oauth-refresh-token`, `oauth-access-token`, `oauth-expires-at`). Keycloak rotates the refresh token on every successful refresh and rejects the previous one. A second post of that previous token returns `invalid_grant` and can revoke the whole chain.

Today the two processes can both post. bx-proxy reloads `_configuration` before each Bexio call. bx-sync keeps the tokens in memory after startup. `save_tokens` writes the three rows as three autocommit statements on three connections. On `invalid_grant`, `post_token` logs the error, sleeps, and `exit`s. `bxsync.service` has `Restart=on-failure`, so that exit comes back and posts the dead refresh token again.

Inside one process, `bx_fetch` holds `net->mutex` for the whole request, including the refresh. The hole is between processes: the sync daemon, the proxy, and `bxsync --login`.

## Locked refresh

The lock is MariaDB `GET_LOCK`. The name is `bx-oauth-refresh`. The timeout is 20 seconds (`BX_OAUTH_LOCK_TIMEOUT_SEC`). Named locks are server-wide, which matches one Bexio account per MariaDB server.

One MariaDB connection is opened for the critical section and stored on the `BXOAuth` struct. It stays open, idle, while the token HTTP call runs. The three token rows are read and written on that connection. `GET_LOCK` is taken once by the outer holder. `post_token` does not take the lock. An assert fires if a token write runs while the lock is not held. `RELEASE_LOCK` and `mysql_close` run on every exit from the section. A crash or a killed process drops the connection, and MariaDB releases the lock.

`GET_LOCK` returns 1 when the lock is acquired. A timeout (0) or an error (NULL) is logged as `OAuth refresh lock not acquired`. The call returns failure. The process stays up.

The fetch thread may block for the lock timeout plus the token HTTP call while it holds `net->mutex`. Each process has one fetch thread, so that wait does not deadlock inside the process.

### `bx_oauth_ensure`

`tokens_dirty` means Keycloak has issued tokens that are in memory and not yet committed. It skips the fast path.

1. Fast path, lock-free. `tokens_dirty` is clear, `force` is false, and the in-memory access token is usable (rule below). Return success.
2. Copy the current access token. That string is the one a 401 retry just presented to Bexio. The copy survives the reload that frees the struct field.
3. Take the lock. On failure, log and return failure.
4. If `tokens_dirty` is set, do not reload. A reload would replace the only copy of the new refresh token with the old row. When `force` is false and the in-memory access token is usable, retry that write on this connection. Three attempts total (the first try plus two retries). On commit, clear `tokens_dirty`, unlock, and return success. If all three attempts fail, leave `tokens_dirty` set, unlock, and return success. The next ensure tries the write again. When `force` is true, or the in-memory access token is not usable, post a refresh from the in-memory refresh token (step 7), still without reloading. That token is the grant Keycloak last issued to this process. If it is missing, log `No Bexio refresh token. Run: bxsync --login <config>`, unlock, and return failure.
5. When `tokens_dirty` is clear, re-read the three rows on the lock connection. Store them with `bx_conf_set` and in the `BXOAuth` struct. Do not call `bx_conf_load_database` here. That helper opens another connection and loads every key.
6. Call `bx_oauth_decide`. Then:
   - **use.** Log `OAuth access token was refreshed by the other process`. Unlock and return success.
   - **need-login.** Log `No Bexio refresh token. Run: bxsync --login <config>`. Unlock and return failure.
   - **refresh.** Post the refresh grant.
7. On a successful post, write the three rows in one transaction on the lock connection, refresh token first, then the access token, then the expiry. A missing or non-positive `expires_in` still becomes the existing default of 300 seconds, so the new access token is usable. Commit, update memory with `bx_conf_set`, refresh the struct fields, clear `tokens_dirty`, unlock, and return success. The write uses the same three-attempt rule. If all three attempts fail, leave the new tokens in memory, set `tokens_dirty`, unlock, and return success.
8. On `invalid_grant`, log the Bexio error and `Refresh token was rejected. Run: bxsync --login <config>`. Leave the stored rows as they are. Unlock and return failure. No `exit`, no sleep.
9. On a transport or HTTP failure before tokens are issued, leave memory and the database as they were. Unlock and return failure.

A usable access token is non-empty and `now + 60 < expires_at`. The 60 seconds are `BX_OAUTH_SKEW_SEC`.

The transaction is three upserts of the existing `INSERT ... ON DUPLICATE KEY UPDATE` statement, with `bval` and `fval` cleared, inside `START TRANSACTION` / `COMMIT`. Other connections see either the previous triple or the new triple. The expiry must fit in a configuration integer. A value outside `0 .. INT_MAX` logs and fails that write attempt. It does not exit. After three such failures the step 7 rule applies: keep the new tokens in memory, set `tokens_dirty`, and return success.

`bx_conf_put_database` stays unchanged and is no longer used for these three rows. It opens its own connection and commits one row at a time.

### Decide

```c
typedef enum e_BXOauthAction {
  BX_OAUTH_ACT_USE = 0,
  BX_OAUTH_ACT_REFRESH,
  BX_OAUTH_ACT_NEED_LOGIN
} BXOauthAction;

BXOauthAction bx_oauth_decide(bool force, const char *rejected,
                              const char *access, const char *refresh,
                              time_t expires_at, time_t now);
```

An access or refresh string with length 0 is treated as missing. `rejected` missing means there was no access token on the 401 path.

- Return use when `force` is false and the access token is usable.
- Return use when `force` is true, the access token is usable, and its string differs from `rejected`.
- Return need-login when the call would otherwise refresh and the refresh token is missing.
- Return refresh in every other case where the refresh token is present. That includes a 401 whose reloaded access token is still the rejected string, even when the clock says it is usable.

`tokens_dirty` is not an argument. The usable-token rule is `now + 60 < expires_at`, so an expiry equal to `now + 60` refreshes.

### `bxsync --login`

The browser wait is unlocked. The command can sit for up to five minutes without blocking the daemon.

Once the authorization code is in hand, login takes `bx-oauth-refresh`, exchanges the code, and commits the three rows on that connection. The write is retried up to three attempts. If the commit still fails, login logs the database error and returns `BX_OAUTH_FAILED`. The new refresh token then exists only in that short-lived process, so the operator runs `--login` again. Login does not keep a `tokens_dirty` flag after it returns.

A lock timeout makes `--login` return failure. The process exit code stays the current one: `EXIT_SUCCESS` only when `bx_oauth_login` returns `BX_OAUTH_OK`. That exit is the CLI result, not the daemon's `invalid_grant` path.

If the daemon is refreshing while login commits, it is blocked on the lock. When it acquires the lock it re-reads and uses the login tokens when they are usable.

### Callers after a failed ensure

No caller changes.

- The sync treats a `NULL` response as a failed request and continues its loop.
- The proxy already maps an upstream failure to HTTP 200 with `X-Bx-Proxy: stale` when it has a row, and to 502 when it does not.

## Where the code lives

bx-sync is the source. bx-proxy compiles `vendor/bx-sync` and does not compile against `../bx-sync`. After the edit, `src/bx_oauth.c` and `src/include/bx_oauth.h` are copied over the vendor copies. Those two files are the same text in both trees.

That copy adds `bx_oauth_reload` to live bx-sync. The sync does not call it on the hot path. The proxy's vendored `bx_net.c` keeps calling `bx_oauth_reload` before `bx_oauth_ensure`. Live `bx_net.c` is unchanged. No other vendored file is refreshed.

Public signatures of `bx_oauth_ensure`, `bx_oauth_login`, `bx_oauth_startup`, and `bx_oauth_reload` stay the same. `bx_oauth_decide` is new and public so the tests can call it.

No schema change. No new configuration key.

## Docs

Update the refresh paragraphs in `bx-sync/README.md` (English and French) and `bx-proxy/README.md`.

- Both processes may refresh. They take `bx-oauth-refresh` first, re-read, and post only when the stored access token still needs replacing.
- A rejected refresh is logged. The process keeps running.
- Recovery is `bxsync --login` while the daemon is up. The daemon loads the new rows on its next ensure.
- Remove the proxy sentence that says the two processes can still refresh the same token at the same moment.

The systemd unit is unchanged. Stopping the exit removes the restart loop. The operator does not have to stop the unit before `--login`.

## Tests

`make test` does not need MariaDB or Bexio. It checks `bx_oauth_decide` only.

The cases live in `bx-sync/test/oauth_decide.c` and `bx-proxy/test/oauth_decide.c`. The sync test compiles live `src/bx_oauth.c`. The proxy test compiles `vendor/bx-sync/src/bx_oauth.c`. Each tree's `make test` runs its own file. bx-sync gains a `test` target. The proxy target is added next to the existing route, JSON, and freshness tests.

`now` is 1000000. The boundary is 60 seconds.

| Call | Result |
| --- | --- |
| No force, access set, expiry `now + 61`, refresh set | use |
| No force, access set, expiry `now + 61`, refresh missing | use |
| No force, access set, expiry `now + 60`, refresh set | refresh |
| No force, access set, expiry `now`, refresh set | refresh |
| No force, access missing, refresh set | refresh |
| No force, access missing, refresh missing | need-login |
| No force, access expired, refresh missing | need-login |
| Force, rejected equals reloaded, expiry `now + 61`, refresh set | refresh |
| Force, rejected differs, expiry `now + 61`, refresh set | use |
| Force, rejected differs, expiry `now`, refresh set | refresh |
| Force, rejected differs, expiry `now + 61`, refresh missing | use |
| Force, rejected equals reloaded, refresh missing | need-login |
| Empty access string | same as missing |
| Empty refresh string | same as missing |

The lock, the transaction, and the token HTTP call are not part of `make test`. Two manual checks cover them, against one shared database with the sync and the proxy both running:

1. Clear `oauth-expires-at` so the next call on each side needs a refresh. One process posts the refresh. The other logs `OAuth access token was refreshed by the other process`. Both processes are still running afterward.
2. Break the refresh token on purpose. The log contains `Refresh token was rejected. Run: bxsync --login <config>`. Neither process has exited.

## Residual window

If the process dies after Keycloak has rotated the refresh token and before the commit, the new refresh token dies with it. The next refresh logs `invalid_grant` and stays up. `--login` is the recovery. The lock does not close this window.

## Out of scope

- Re-vendoring bx-sync beyond `bx_oauth.c` and `bx_oauth.h`.
- Changing how the sync loop or the proxy reports a failed Bexio call.
- A live Keycloak or MariaDB test in `make test`.
- Editing `bxsync.service`.
