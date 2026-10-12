#ifndef BX_OAUTH_H__
#define BX_OAUTH_H__

#include "bx_conf.h"
#include <stdbool.h>

typedef struct s_BXOAuth BXOAuth;

typedef enum e_BXOauthStatus {
  BX_OAUTH_OK = 0,
  BX_OAUTH_NEED_LOGIN,
  BX_OAUTH_FAILED
} BXOauthStatus;

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
