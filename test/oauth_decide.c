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
