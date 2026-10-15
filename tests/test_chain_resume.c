#include "../src/include/bx_chain.h"
#include "../src/include/bx_conf.h"
#include "../src/include/bx_utils.h"
#include "../src/include/bxobjects/contact.h"
#include "../src/include/bxobjects/invoice.h"
#include "../src/include/bxobjects/project.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Loads both cursor formats through the real BXConf and bx_chain_load.
 * No MySQL and no network. The race section rewrites the conf while readers
 * call bx_chain_load. */

static int failures;
static int info_logs;
static char last_info[256];
static pthread_mutex_t log_mu = PTHREAD_MUTEX_INITIALIZER;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
      failures++;                                                              \
    }                                                                          \
  } while (0)

void _bx_log_debug(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

void _bx_log_info(char *file, int line, pthread_t thid, const char *fmt, ...) {
  va_list ap;

  (void)file;
  (void)line;
  (void)thid;
  pthread_mutex_lock(&log_mu);
  va_start(ap, fmt);
  vsnprintf(last_info, sizeof(last_info), fmt, ap);
  va_end(ap);
  info_logs++;
  pthread_mutex_unlock(&log_mu);
}

void _bx_log_error(char *file, int line, pthread_t thid, const char *fmt, ...) {
  (void)file;
  (void)line;
  (void)thid;
  (void)fmt;
}

BXillError bx_contact_walk_pass(bXill *app, MYSQL *conn, Cache *cache,
                                bool archived, int64_t start_offset,
                                bx_walk_page_fn on_page, void *page_ctx) {
  (void)app;
  (void)conn;
  (void)cache;
  (void)archived;
  (void)start_offset;
  (void)on_page;
  (void)page_ctx;
  return NoError;
}

BXillError bx_project_walk_item_from(bXill *app, MYSQL *conn, Cache *cache,
                                     int64_t start_offset,
                                     bx_walk_page_fn on_page, void *page_ctx) {
  (void)app;
  (void)conn;
  (void)cache;
  (void)start_offset;
  (void)on_page;
  (void)page_ctx;
  return NoError;
}

BXillError bx_invoice_walk_list(bXill *app, MYSQL *conn, Cache *cache,
                                int64_t start_offset, bx_walk_page_fn on_page,
                                void *page_ctx) {
  (void)app;
  (void)conn;
  (void)cache;
  (void)start_offset;
  (void)on_page;
  (void)page_ctx;
  return NoError;
}

BXillError bx_invoice_fetch_missed(bXill *app, MYSQL *conn, Cache *cache) {
  (void)app;
  (void)conn;
  (void)cache;
  return NoError;
}

static void set_str(BXConf *conf, const char *key, const char *value) {
  bx_conf_set(conf, key, (void *)value, StringType);
}

static void set_int(BXConf *conf, const char *key, int value) {
  bx_conf_set(conf, key, &value, IntegerType);
}

static void seed_legacy(BXConf *conf) {
  set_str(conf, "sync-chain-step", "contact");
  set_int(conf, "sync-chain-offset", 2500);
  set_int(conf, "sync-chain-cycle", 0);
}

static BXChainCursor load_of(BXConf *conf) {
  bXill app;
  BXChainCursor cur;

  memset(&app, 0, sizeof(app));
  memset(&cur, 0, sizeof(cur));
  app.conf = conf;
  bx_chain_load(&app, &cur);
  return cur;
}

static void expect_phase(BXChainCursor cur, BXChainPhase phase, int offset,
                         const char *what) {
  if (cur.phase != phase || cur.offset != offset) {
    fprintf(stderr, "FAIL %s: got %s:%d want %s:%d\n", what,
            bx_chain_phase_name(cur.phase), cur.offset,
            bx_chain_phase_name(phase), offset);
    failures++;
  }
}

static BXConf *gconf;
static atomic_int stop_writers;
static atomic_int bad_reads;
static atomic_int good_reads;
static atomic_int bad_phase;
static atomic_int bad_offset;

static void *writer_main(void *arg) {
  int flip = 0;

  (void)arg;
  while (!atomic_load(&stop_writers)) {
    if (flip) {
      set_str(gconf, "sync-chain", "invoice-list:2500");
      set_str(gconf, "oauth-access-token", "token-a");
    } else {
      set_str(gconf, "sync-chain", "invoice-list:2501");
      set_str(gconf, "oauth-access-token", "token-b-longer");
    }
    flip = !flip;
  }
  return NULL;
}

static void *reader_main(void *arg) {
  (void)arg;
  while (!atomic_load(&stop_writers)) {
    BXChainCursor cur = load_of(gconf);
    if (cur.phase != BX_CHAIN_INVOICE_LIST ||
        (cur.offset != 2500 && cur.offset != 2501)) {
      atomic_store(&bad_phase, (int)cur.phase);
      atomic_store(&bad_offset, cur.offset);
      atomic_fetch_add(&bad_reads, 1);
      atomic_store(&stop_writers, 1);
      break;
    }
    atomic_fetch_add(&good_reads, 1);
  }
  return NULL;
}

/* nwriters threads rewrite sync-chain (and an unrelated key) while two
 * readers load it. Legacy rows stay in the conf, so a NULL window resumes
 * contact-active:2500. */
static int race_reads(int nwriters) {
  pthread_t writers[2];
  pthread_t readers[2];
  int i;

  if (nwriters < 1 || nwriters > 2) {
    return 0;
  }
  gconf = bx_conf_init();
  if (gconf == NULL) {
    fprintf(stderr, "FAIL race: conf init\n");
    return 0;
  }
  set_str(gconf, "sync-chain", "invoice-list:2500");
  seed_legacy(gconf);
  set_str(gconf, "oauth-access-token", "token-a");
  atomic_store(&stop_writers, 0);
  atomic_store(&bad_reads, 0);
  atomic_store(&good_reads, 0);
  atomic_store(&bad_phase, -1);
  atomic_store(&bad_offset, -1);
  for (i = 0; i < nwriters; i++) {
    pthread_create(&writers[i], NULL, writer_main, NULL);
  }
  pthread_create(&readers[0], NULL, reader_main, NULL);
  pthread_create(&readers[1], NULL, reader_main, NULL);
  usleep(200000);
  atomic_store(&stop_writers, 1);
  for (i = 0; i < nwriters; i++) {
    pthread_join(writers[i], NULL);
  }
  pthread_join(readers[0], NULL);
  pthread_join(readers[1], NULL);
  if (atomic_load(&bad_reads) != 0 || atomic_load(&good_reads) < 100) {
    fprintf(stderr,
            "FAIL both formats under rewrite (%d writers): bad=%d good=%d "
            "example %s:%d\n",
            nwriters, atomic_load(&bad_reads), atomic_load(&good_reads),
            bx_chain_phase_name((BXChainPhase)atomic_load(&bad_phase)),
            atomic_load(&bad_offset));
    bx_conf_destroy(&gconf);
    return 0;
  }
  bx_conf_destroy(&gconf);
  return 1;
}

static void test_quiet(void) {
  BXConf *conf;
  BXChainCursor cur;
  int infos;

  bx_chain_decode("invoice-list:2500", "contact", 2500, 0, &cur);
  expect_phase(cur, BX_CHAIN_INVOICE_LIST, 2500, "decode both formats");

  bx_chain_decode("nope", "contact", 2500, 0, &cur);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 0, "decode unreadable");

  bx_chain_decode(NULL, "contact", 2500, 0, &cur);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 2500, "decode legacy only");

  bx_chain_decode("project:40", NULL, 0, 0, &cur);
  expect_phase(cur, BX_CHAIN_PROJECT, 40, "decode new only");

  conf = bx_conf_init();
  CHECK(conf != NULL);
  set_str(conf, "sync-chain", "invoice-list:2500");
  seed_legacy(conf);
  infos = info_logs;
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_INVOICE_LIST, 2500, "conf both formats");
  CHECK(info_logs > infos);
  CHECK(strstr(last_info, "sync-chain-step") != NULL);
  CHECK(strstr(last_info, "ignored") != NULL);
  bx_conf_destroy(&conf);

  conf = bx_conf_init();
  seed_legacy(conf);
  infos = info_logs;
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 2500, "conf legacy only");
  CHECK(info_logs == infos);
  bx_conf_destroy(&conf);

  conf = bx_conf_init();
  set_str(conf, "sync-chain", "invoice-list:2500");
  infos = info_logs;
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_INVOICE_LIST, 2500, "conf new only");
  CHECK(info_logs == infos);
  bx_conf_destroy(&conf);

  conf = bx_conf_init();
  set_str(conf, "sync-chain", "not-a-cursor");
  seed_legacy(conf);
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 0, "conf unreadable string");
  bx_conf_destroy(&conf);

  conf = bx_conf_init();
  set_int(conf, "sync-chain", 2500);
  seed_legacy(conf);
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 0, "conf unreadable type");
  bx_conf_destroy(&conf);

  conf = bx_conf_init();
  cur = load_of(conf);
  expect_phase(cur, BX_CHAIN_CONTACT_ACTIVE, 0, "conf empty");
  bx_conf_destroy(&conf);
}

int main(void) {
  test_quiet();
  if (!race_reads(1)) {
    failures++;
  } else if (!race_reads(2)) {
    failures++;
  }
  if (failures != 0) {
    fprintf(stderr, "%d chain resume checks failed\n", failures);
    return 1;
  }
  printf("chain resume ok\n");
  return 0;
}
