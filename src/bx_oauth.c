#include "include/bx_oauth.h"
#include "include/bx_utils.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <jansson.h>
#include <limits.h>
#include <netinet/in.h>
#include <oauth2/http.h>
#include <oauth2/mem.h>
#include <oauth2/oauth2.h>
#include <openssl/evp.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BX_OAUTH_AUTHORIZE_URL                                                 \
  "https://auth.bexio.com/realms/bexio/protocol/openid-connect/auth"
#define BX_OAUTH_TOKEN_URL                                                     \
  "https://auth.bexio.com/realms/bexio/protocol/openid-connect/token"
#define BX_OAUTH_REDIRECT_DEFAULT "http://127.0.0.1:8765/callback"
#define BX_OAUTH_DEFAULT_SCOPE                                                 \
  "openid offline_access company_profile contact_show kb_invoice_show "       \
  "kb_offer_show kb_order_show kb_delivery_show kb_bill_show "                 \
  "kb_expense_show kb_article_order_show accounting project_show "             \
  "article_show note_show task_show file bank_account_show"
#define BX_OAUTH_SKEW_SEC 60
#define BX_OAUTH_DEFAULT_TTL_SEC 300
#define BX_OAUTH_LOGIN_TIMEOUT_MS (5 * 60 * 1000)

struct s_BXOAuth {
  oauth2_log_t *log;
  oauth2_cfg_endpoint_t *token_ep;
  char *client_id;
  char *redirect_uri;
  char *scope;
  BXConf *conf;
  char *access_token;
  char *refresh_token;
  time_t expires_at;
  bool refreshing;
};

static char *dup_str(const char *s) {
  if (s == NULL) {
    return NULL;
  }
  size_t n = strlen(s);
  char *d = malloc(n + 1);
  if (d == NULL) {
    return NULL;
  }
  memcpy(d, s, n + 1);
  return d;
}

static char *conf_string(BXConf *conf, const char *key) {
  const char *s = bx_conf_get_string(conf, key);
  if (s == NULL || s[0] == '\0') {
    return NULL;
  }
  return dup_str(s);
}

static char *own_oauth2(char *s) {
  char *d = NULL;
  if (s == NULL) {
    return NULL;
  }
  d = dup_str(s);
  oauth2_mem_free(s);
  return d;
}

static void oauth_fail(const char *msg) {
  bx_log_error("%s", msg);
  fprintf(stderr, "%s\n", msg);
}

static bool save_tokens(BXOAuth *oa) {
  int exp = 0;

  if (oa->conf == NULL || oa->access_token == NULL || oa->refresh_token == NULL) {
    return false;
  }
  if (oa->expires_at < 0 || oa->expires_at > (time_t)INT_MAX) {
    oauth_fail("OAuth expiry does not fit an integer configuration value");
    return false;
  }
  exp = (int)oa->expires_at;
  /* Refresh token first: Keycloak invalidates the previous one on success. */
  if (!bx_conf_put_database(oa->conf, "oauth-refresh-token", StringType,
                            oa->refresh_token)) {
    oauth_fail("Could not save the OAuth refresh token");
    return false;
  }
  if (!bx_conf_put_database(oa->conf, "oauth-access-token", StringType,
                            oa->access_token)) {
    oauth_fail("Could not save the OAuth access token");
    return false;
  }
  if (!bx_conf_put_database(oa->conf, "oauth-expires-at", IntegerType, &exp)) {
    oauth_fail("Could not save the OAuth expiry");
    return false;
  }
  return true;
}

static BXOauthStatus load_tokens(BXOAuth *oa) {
  const char *access = NULL;
  const char *refresh = NULL;

  if (oa->conf == NULL) {
    return BX_OAUTH_FAILED;
  }
  access = bx_conf_get_string(oa->conf, "oauth-access-token");
  refresh = bx_conf_get_string(oa->conf, "oauth-refresh-token");
  oa->expires_at = (time_t)bx_conf_get_int(oa->conf, "oauth-expires-at");
  if (access != NULL && access[0] != '\0') {
    free(oa->access_token);
    oa->access_token = dup_str(access);
    if (oa->access_token == NULL) {
      oauth_fail("Out of memory while reading the OAuth access token");
      return BX_OAUTH_FAILED;
    }
  }
  if (refresh != NULL && refresh[0] != '\0') {
    free(oa->refresh_token);
    oa->refresh_token = dup_str(refresh);
    if (oa->refresh_token == NULL) {
      oauth_fail("Out of memory while reading the OAuth refresh token");
      return BX_OAUTH_FAILED;
    }
  }
  if (oa->refresh_token == NULL || oa->refresh_token[0] == '\0') {
    return BX_OAUTH_NEED_LOGIN;
  }
  return BX_OAUTH_OK;
}

static bool post_token(BXOAuth *oa, oauth2_nv_list_t *form) {
  oauth2_http_call_ctx_t *ctx = NULL;
  char *response = NULL;
  char *err = NULL;
  char *desc = NULL;
  char *access = NULL;
  char *refresh = NULL;
  json_t *json = NULL;
  json_int_t expires_in = 0;
  oauth2_http_status_code_t status = 0;
  bool called = false;
  bool reauth = false;
  char msg[512];

  ctx = oauth2_http_call_ctx_init(oa->log);
  if (ctx == NULL) {
    return false;
  }
  oauth2_http_call_ctx_ssl_verify_set(oa->log, ctx, true);
  if (!oauth2_http_ctx_auth_add(oa->log, ctx,
                                oauth2_cfg_endpoint_get_auth(oa->token_ep),
                                form)) {
    oauth2_http_call_ctx_free(oa->log, ctx);
    oauth_fail("Could not set Bexio client authentication");
    return false;
  }
  called = oauth2_http_post_form(oa->log, BX_OAUTH_TOKEN_URL, form, ctx,
                                 &response, &status);
  oauth2_http_call_ctx_free(oa->log, ctx);
  if (!called || response == NULL) {
    oauth_fail("Bexio token request failed");
    if (response != NULL) {
      oauth2_mem_free(response);
    }
    return false;
  }
  if (!oauth2_json_decode_object(oa->log, response, &json)) {
    oauth2_mem_free(response);
    snprintf(msg, sizeof(msg),
             "Bexio token response was not a JSON object (HTTP %u)",
             (unsigned)status);
    oauth_fail(msg);
    return false;
  }
  oauth2_mem_free(response);

  oauth2_json_string_get(oa->log, json, "error", &err, NULL);
  if (err != NULL) {
    oauth2_json_string_get(oa->log, json, "error_description", &desc, NULL);
    reauth = strcmp(err, "invalid_grant") == 0;
    snprintf(msg, sizeof(msg), "Bexio token error: %s%s%s", err,
             desc != NULL ? ": " : "", desc != NULL ? desc : "");
    oauth_fail(msg);
    oauth2_mem_free(err);
    if (desc != NULL) {
      oauth2_mem_free(desc);
    }
    json_decref(json);
    if (reauth && oa->refreshing) {
      oauth_fail("Refresh token was rejected. Run: bxsync --login <config>");
      usleep(100000);
      exit(EXIT_FAILURE);
    }
    return false;
  }

  oauth2_json_string_get(oa->log, json, "access_token", &access, NULL);
  oauth2_json_string_get(oa->log, json, "refresh_token", &refresh, NULL);
  oauth2_json_number_get(oa->log, json, "expires_in", &expires_in, 0);
  json_decref(json);
  if (access == NULL) {
    if (refresh != NULL) {
      oauth2_mem_free(refresh);
    }
    snprintf(msg, sizeof(msg),
             "Bexio token response had no access_token (HTTP %u)",
             (unsigned)status);
    oauth_fail(msg);
    return false;
  }

  free(oa->access_token);
  oa->access_token = own_oauth2(access);
  if (refresh != NULL) {
    free(oa->refresh_token);
    oa->refresh_token = own_oauth2(refresh);
  }
  if (oa->access_token == NULL || oa->refresh_token == NULL) {
    oauth_fail("Out of memory while storing Bexio tokens");
    return false;
  }
  if (expires_in <= 0) {
    expires_in = BX_OAUTH_DEFAULT_TTL_SEC;
  }
  oa->expires_at = time(NULL) + (time_t)expires_in;
  if (!save_tokens(oa)) {
    return false;
  }
  return true;
}

static bool refresh_tokens(BXOAuth *oa) {
  oauth2_nv_list_t *form = NULL;
  bool ok = false;

  if (oa->refresh_token == NULL) {
    return false;
  }
  form = oauth2_nv_list_init(oa->log);
  if (form == NULL) {
    return false;
  }
  oauth2_nv_list_add(oa->log, form, "grant_type", "refresh_token");
  oauth2_nv_list_add(oa->log, form, "refresh_token", oa->refresh_token);
  oauth2_nv_list_add(oa->log, form, "client_id", oa->client_id);
  oauth2_nv_list_add(oa->log, form, "scope", oa->scope);
  oa->refreshing = true;
  ok = post_token(oa, form);
  oa->refreshing = false;
  oauth2_nv_list_free(oa->log, form);
  return ok;
}

static bool redirect_port(const char *uri, int *port) {
  const char *rest = NULL;
  char *end = NULL;
  long value = 0;

  if (strncmp(uri, "http://127.0.0.1:", 17) == 0) {
    rest = uri + 17;
  } else if (strncmp(uri, "http://localhost:", 17) == 0) {
    rest = uri + 17;
  } else {
    return false;
  }
  value = strtol(rest, &end, 10);
  if (end == rest || value < 1 || value > 65535) {
    return false;
  }
  if (*end != '/' && *end != '\0') {
    return false;
  }
  *port = (int)value;
  return true;
}

/* PKCE S256 is base64url(SHA-256(verifier)) without '=' padding. */
static char *pkce_challenge(oauth2_log_t *log, const char *verifier) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;
  char *b64 = NULL;
  char *challenge = NULL;
  size_t n = 0;
  char *eq = NULL;

  if (EVP_Digest(verifier, strlen(verifier), hash, &hash_len, EVP_sha256(),
                 NULL) != 1) {
    return NULL;
  }
  n = oauth2_base64url_encode(log, hash, hash_len, &b64);
  if (b64 == NULL || n == 0) {
    return NULL;
  }
  challenge = malloc(n + 1);
  if (challenge == NULL) {
    oauth2_mem_free(b64);
    return NULL;
  }
  memcpy(challenge, b64, n);
  challenge[n] = '\0';
  oauth2_mem_free(b64);
  eq = strchr(challenge, '=');
  if (eq != NULL) {
    *eq = '\0';
  }
  return challenge;
}

static char *query_value(oauth2_log_t *log, const char *src, const char *name) {
  const char *q = NULL;
  const char *p = NULL;
  size_t nlen = 0;
  char *raw = NULL;
  char *decoded = NULL;
  char *owned = NULL;

  if (src == NULL || name == NULL) {
    return NULL;
  }
  q = strchr(src, '?');
  q = q != NULL ? q + 1 : src;
  nlen = strlen(name);
  p = q;
  while (*p != '\0') {
    if ((p == q || p[-1] == '&' || p[-1] == '?') &&
        strncmp(p, name, nlen) == 0 && p[nlen] == '=') {
      const char *start = p + nlen + 1;
      const char *end = start;
      while (*end != '\0' && *end != '&' && *end != ' ' && *end != '\r' &&
             *end != '\n' && *end != '#') {
        end++;
      }
      raw = malloc((size_t)(end - start) + 1);
      if (raw == NULL) {
        return NULL;
      }
      memcpy(raw, start, (size_t)(end - start));
      raw[end - start] = '\0';
      decoded = oauth2_url_decode(log, raw);
      free(raw);
      owned = own_oauth2(decoded);
      return owned;
    }
    p++;
  }
  return NULL;
}

static int listen_local(int port) {
  int fd = -1;
  int one = 1;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
  }
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
      listen(fd, 1) != 0) {
    close(fd);
    return -1;
  }
  return fd;
}

static void reply_ok(int fd) {
  static const char body[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<!DOCTYPE html><html><body><p>bxsync got the browser callback. "
      "Return to the terminal.</p></body></html>\n";
  size_t off = 0;
  size_t len = sizeof(body) - 1;
  while (off < len) {
    ssize_t n = write(fd, body + off, len - off);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    off += (size_t)n;
  }
}

static bool accept_code(oauth2_log_t *log, int lfd, const char *expect_state,
                        char **code) {
  int cfd = -1;
  char req[4096];
  size_t n = 0;
  char *got_state = NULL;
  char *got_code = NULL;
  struct timeval tv;

  *code = NULL;
  cfd = accept(lfd, NULL, NULL);
  if (cfd < 0) {
    return false;
  }
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  while (n + 1 < sizeof(req)) {
    char c = 0;
    ssize_t r = read(cfd, &c, 1);
    if (r <= 0) {
      break;
    }
    req[n++] = c;
    if (c == '\n') {
      break;
    }
  }
  req[n] = '\0';
  got_code = query_value(log, req, "code");
  got_state = query_value(log, req, "state");
  reply_ok(cfd);
  if (got_code != NULL && got_state != NULL &&
      strcmp(got_state, expect_state) == 0) {
    *code = got_code;
    got_code = NULL;
  }
  free(got_code);
  free(got_state);
  close(cfd);
  return *code != NULL;
}

static bool code_from_line(oauth2_log_t *log, char *line, const char *expect,
                           char **code) {
  char *got_state = NULL;
  char *got_code = NULL;
  bool ok = false;

  *code = NULL;
  while (*line != '\0' && isspace((unsigned char)*line)) {
    line++;
  }
  got_code = query_value(log, line, "code");
  got_state = query_value(log, line, "state");
  if (got_code != NULL && got_state != NULL && strcmp(got_state, expect) == 0) {
    *code = got_code;
    got_code = NULL;
    ok = true;
  } else if (got_state != NULL && strcmp(got_state, expect) != 0) {
    oauth_fail("OAuth state did not match this login. Paste the URL from this attempt.");
  } else if (strchr(line, '?') != NULL || strstr(line, "code=") != NULL) {
    oauth_fail("Could not read code and state from that URL.");
  }
  free(got_code);
  free(got_state);
  return ok;
}

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
  ok = post_token(oa, form);
  oauth2_nv_list_free(oa->log, form);
  return ok;
}

static void print_login_help(const char *url, int port) {
  printf("\nOpen this URL in a browser on your laptop:\n\n%s\n\n", url);
  printf("This server has no desktop. Forward the callback port from the\n");
  printf("laptop, then open the URL there:\n\n");
  printf("  ssh -L %d:127.0.0.1:%d user@this-server\n\n", port, port);
  printf("The browser will call http://127.0.0.1:%d/... on the laptop,\n",
         port);
  printf("and SSH delivers that to this process.\n\n");
  printf("If the browser cannot connect, copy the full address-bar URL\n");
  printf("(it contains ?code=...) and paste it here, then press Enter.\n\n");
  printf("Waiting up to 5 minutes.\n");
  fflush(stdout);
}

BXOAuth *bx_oauth_init(BXConf *conf) {
  BXOAuth *oa = NULL;
  oauth2_nv_list_t *opts = NULL;
  char *err = NULL;
  char *secret = NULL;

  oa = calloc(1, sizeof(*oa));
  if (oa == NULL) {
    return NULL;
  }
  oa->conf = conf;
  oa->log = oauth2_init(OAUTH2_LOG_ERROR, NULL);
  if (oa->log == NULL) {
    free(oa);
    return NULL;
  }
  oa->client_id = conf_string(conf, "oauth-client-id");
  secret = conf_string(conf, "oauth-client-secret");
  oa->redirect_uri = conf_string(conf, "oauth-redirect-uri");
  oa->scope = conf_string(conf, "oauth-scope");
  if (oa->redirect_uri == NULL) {
    oa->redirect_uri = dup_str(BX_OAUTH_REDIRECT_DEFAULT);
  }
  if (oa->scope == NULL) {
    oa->scope = dup_str(BX_OAUTH_DEFAULT_SCOPE);
  }
  if (oa->client_id == NULL || secret == NULL || oa->redirect_uri == NULL ||
      oa->scope == NULL) {
    oauth_fail(
        "oauth-client-id and oauth-client-secret are required in the config");
    free(secret);
    bx_oauth_destroy(&oa);
    return NULL;
  }

  opts = oauth2_nv_list_init(oa->log);
  oa->token_ep = oauth2_cfg_endpoint_init(oa->log);
  if (opts == NULL || oa->token_ep == NULL) {
    if (opts != NULL) {
      oauth2_nv_list_free(oa->log, opts);
    }
    free(secret);
    bx_oauth_destroy(&oa);
    return NULL;
  }
  oauth2_nv_list_add(oa->log, opts, "auth", "client_secret_basic");
  oauth2_nv_list_add(oa->log, opts, "client_id", oa->client_id);
  oauth2_nv_list_add(oa->log, opts, "client_secret", secret);
  err = oauth2_cfg_set_endpoint(oa->log, oa->token_ep, BX_OAUTH_TOKEN_URL, opts,
                                NULL);
  oauth2_nv_list_free(oa->log, opts);
  free(secret);
  if (err != NULL) {
    oauth_fail(err);
    oauth2_mem_free(err);
    bx_oauth_destroy(&oa);
    return NULL;
  }
  return oa;
}

void bx_oauth_destroy(BXOAuth **oauth) {
  BXOAuth *oa = NULL;
  if (oauth == NULL || *oauth == NULL) {
    return;
  }
  oa = *oauth;
  if (oa->token_ep != NULL && oa->log != NULL) {
    oauth2_cfg_endpoint_free(oa->log, oa->token_ep);
  }
  free(oa->client_id);
  free(oa->redirect_uri);
  free(oa->scope);
  if (oa->access_token != NULL) {
    memset(oa->access_token, 0, strlen(oa->access_token));
    free(oa->access_token);
  }
  if (oa->refresh_token != NULL) {
    memset(oa->refresh_token, 0, strlen(oa->refresh_token));
    free(oa->refresh_token);
  }
  if (oa->log != NULL) {
    oauth2_shutdown(oa->log);
  }
  free(oa);
  *oauth = NULL;
}

BXOauthStatus bx_oauth_startup(BXOAuth *oauth) {
  BXOauthStatus st = BX_OAUTH_FAILED;
  char msg[512];

  if (oauth == NULL) {
    return BX_OAUTH_FAILED;
  }
  st = load_tokens(oauth);
  if (st == BX_OAUTH_NEED_LOGIN) {
    snprintf(msg, sizeof(msg),
             "No Bexio refresh token in _configuration. Run: bxsync --login "
             "<config>");
    oauth_fail(msg);
    return st;
  }
  if (st != BX_OAUTH_OK) {
    return st;
  }
  if (!bx_oauth_ensure(oauth, false)) {
    return BX_OAUTH_FAILED;
  }
  return BX_OAUTH_OK;
}

bool bx_oauth_ensure(BXOAuth *oauth, bool force) {
  if (oauth == NULL) {
    return false;
  }
  if (!force && oauth->access_token != NULL &&
      time(NULL) + BX_OAUTH_SKEW_SEC < oauth->expires_at) {
    return true;
  }
  if (oauth->refresh_token == NULL) {
    oauth_fail("No Bexio refresh token. Run: bxsync --login <config>");
    return false;
  }
  return refresh_tokens(oauth);
}

const char *bx_oauth_access_token(const BXOAuth *oauth) {
  if (oauth == NULL) {
    return NULL;
  }
  return oauth->access_token;
}

BXOauthStatus bx_oauth_login(BXOAuth *oa) {
  char *verifier = NULL;
  char *state = NULL;
  char *challenge = NULL;
  char *url = NULL;
  char *code = NULL;
  oauth2_nv_list_t *params = NULL;
  int port = 0;
  int lfd = -1;
  struct pollfd fds[2];
  struct timespec start;
  struct timespec now;
  bool got = false;
  BXOauthStatus st = BX_OAUTH_FAILED;

  if (oa == NULL) {
    return BX_OAUTH_FAILED;
  }
  if (!redirect_port(oa->redirect_uri, &port)) {
    oauth_fail("oauth-redirect-uri must be http://127.0.0.1:<port>/... or "
               "http://localhost:<port>/...");
    return BX_OAUTH_FAILED;
  }
  verifier = oauth2_rand_str(oa->log, 64);
  state = oauth2_rand_str(oa->log, 32);
  if (verifier == NULL || state == NULL) {
    oauth_fail("Could not generate OAuth state");
    goto done;
  }
  challenge = pkce_challenge(oa->log, verifier);
  if (challenge == NULL) {
    oauth_fail("Could not build the PKCE challenge");
    goto done;
  }
  params = oauth2_nv_list_init(oa->log);
  if (params == NULL) {
    goto done;
  }
  oauth2_nv_list_add(oa->log, params, "response_type", "code");
  oauth2_nv_list_add(oa->log, params, "client_id", oa->client_id);
  oauth2_nv_list_add(oa->log, params, "redirect_uri", oa->redirect_uri);
  oauth2_nv_list_add(oa->log, params, "scope", oa->scope);
  oauth2_nv_list_add(oa->log, params, "state", state);
  oauth2_nv_list_add(oa->log, params, "code_challenge", challenge);
  oauth2_nv_list_add(oa->log, params, "code_challenge_method", "S256");
  url = oauth2_http_url_query_encode(oa->log, BX_OAUTH_AUTHORIZE_URL, params);
  oauth2_nv_list_free(oa->log, params);
  params = NULL;
  if (url == NULL) {
    oauth_fail("Could not build the Bexio authorize URL");
    goto done;
  }
  print_login_help(url, port);

  lfd = listen_local(port);
  if (lfd < 0) {
    fprintf(stderr,
            "Could not listen on 127.0.0.1:%d (%s). Paste the callback URL "
            "instead.\n",
            port, strerror(errno));
  }
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (!got) {
    int left = 0;
    int pr = 0;
    int nfds = 0;
    clock_gettime(CLOCK_MONOTONIC, &now);
    left = BX_OAUTH_LOGIN_TIMEOUT_MS -
           (int)((now.tv_sec - start.tv_sec) * 1000 +
                 (now.tv_nsec - start.tv_nsec) / 1000000);
    if (left <= 0) {
      oauth_fail("Login timed out after 5 minutes");
      break;
    }
    memset(fds, 0, sizeof(fds));
    if (lfd >= 0) {
      fds[nfds].fd = lfd;
      fds[nfds].events = POLLIN;
      nfds++;
    }
    fds[nfds].fd = STDIN_FILENO;
    fds[nfds].events = POLLIN;
    nfds++;
    pr = poll(fds, (nfds_t)nfds, left);
    if (pr < 0) {
      if (errno == EINTR) {
        continue;
      }
      oauth_fail("poll failed while waiting for the Bexio callback");
      break;
    }
    if (pr == 0) {
      oauth_fail("Login timed out after 5 minutes");
      break;
    }
    if (lfd >= 0 && (fds[0].revents & POLLIN) != 0) {
      if (accept_code(oa->log, lfd, state, &code)) {
        got = true;
        break;
      }
      fprintf(stderr, "Callback did not contain a matching code. Still "
                      "waiting.\n");
    }
    if ((fds[nfds - 1].revents & POLLIN) != 0) {
      char line[8192];
      if (fgets(line, sizeof(line), stdin) == NULL) {
        fds[nfds - 1].fd = -1;
      } else if (code_from_line(oa->log, line, state, &code)) {
        got = true;
      }
    }
  }
  if (got && exchange_code(oa, code, verifier)) {
    printf("Saved Bexio tokens to _configuration\n");
    st = BX_OAUTH_OK;
  }
done:
  if (code != NULL) {
    memset(code, 0, strlen(code));
    free(code);
  }
  free(challenge);
  if (url != NULL) {
    oauth2_mem_free(url);
  }
  if (verifier != NULL) {
    oauth2_mem_free(verifier);
  }
  if (state != NULL) {
    oauth2_mem_free(state);
  }
  if (lfd >= 0) {
    close(lfd);
  }
  return st;
}
