#ifndef BX_OAUTH_H__
#define BX_OAUTH_H__

#include "bx_conf.h"
#include <stdbool.h>
#include <time.h>

typedef struct s_BXOAuth BXOAuth;

typedef enum e_BXOauthStatus {
  BX_OAUTH_OK = 0,
  BX_OAUTH_NEED_LOGIN,
  BX_OAUTH_FAILED
} BXOauthStatus;

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

BXOAuth *bx_oauth_init(BXConf *conf);
void bx_oauth_destroy(BXOAuth **oauth);

/* Load tokens from _configuration and refresh when the access token is due. */
BXOauthStatus bx_oauth_startup(BXOAuth *oauth);

/* Interactive authorization-code login. Prints the URL and waits. */
BXOauthStatus bx_oauth_login(BXOAuth *oauth);

/* Refresh when the access token expires within 60s, or always if force. */
bool bx_oauth_ensure(BXOAuth *oauth, bool force);
const char *bx_oauth_access_token(const BXOAuth *oauth);

#endif /* BX_OAUTH_H__ */
