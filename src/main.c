#include "include/bx_conf.h"
#include "include/bx_ids_cache.h"
#include "include/bx_mutex.h"
#include "include/bx_net.h"
#include "include/bx_utils.h"
#include "include/bxill.h"
#include "include/bxobjects/contact.h"
#include "include/bxobjects/contact_sector.h"
#include "include/bxobjects/country_code.h"
#include "include/bxobjects/invoice.h"
#include "include/bxobjects/language.h"
#include "include/bxobjects/project.h"
#include "include/bxobjects/taxes.h"

#include <jansson.h>
#include <mariadb/ma_list.h>
#include <mariadb/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#define MAX_COMMAND_LEN 100
extern BXMutex io_mutex;
extern BXMutex MTX_COUNTRY_LIST;
const struct timespec THREAD_SLEEP_TIME = {.tv_nsec = 10000000, .tv_sec = 0};

MYSQL *thread_setup_mysql(bXill *app) {
  MYSQL *conn = NULL;
  conn = mysql_init(NULL);
  mysql_thread_init();
  mysql_real_connect(conn, bx_conf_get_string(app->conf, "mysql-host"),
                     bx_conf_get_string(app->conf, "mysql-user"),
                     bx_conf_get_string(app->conf, "mysql-password"),
                     bx_conf_get_string(app->conf, "mysql-database"), 0, NULL,
                     0);
  bx_conf_release(app->conf, "mysql-host");
  bx_conf_release(app->conf, "mysql-user");
  bx_conf_release(app->conf, "mysql-password");
  bx_conf_release(app->conf, "mysql-database");
  mysql_set_character_set(conn, "utf8mb4");
  return conn;
}

void thread_teardown_mysql(MYSQL *conn) {
  mysql_close(conn);
  mysql_thread_end();
}

void *random_item_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;

  conn = thread_setup_mysql(app);
  bx_log_debug("Random items thread data thread %lx", pthread_self());
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    bx_taxes_walk_item(app, conn);
    usleep(500);
  }
  thread_teardown_mysql(conn);
  return 0;
}

void *contact_sector_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;

  conn = thread_setup_mysql(app);
  bx_log_debug("Contact Sector data thread %lx", pthread_self());
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    bx_contact_sector_walk_items(app, conn);
    thrd_yield();
  }
  thread_teardown_mysql(conn);
  return 0;
}

#define CACHE_FILE_CONTACT "contact.bin"
void *contact_thread(void *arg) {
  bXill *app = (bXill *)arg;

  /* mysql init */
  MYSQL *conn = NULL;
  conn = thread_setup_mysql(app);

  /* filename for cache */
  char *filename = bx_utils_cache_filename(app, CACHE_FILE_CONTACT);
  if (!filename) {
    bx_log_debug("Failed allocation of cache filename %s", CACHE_FILE_CONTACT);
    return 0;
  }

  /* init cache */
  Cache *my_cache;
  my_cache = cache_create();
  if (my_cache == NULL) {
    bx_log_error("Cache init failed");
    return 0;
  }
  if (!cache_load(my_cache, filename)) {
    /* loading failed, empty just in case */
    cache_empty(my_cache);
  }

  /* validate cache with database */
  bx_contact_sync_cache_with_db(conn, my_cache);

  /* remove items that are in file but not in database */
  cache_invalidate(my_cache, 1);
  cache_prune(my_cache);
  int cache_checkpoint = bx_utils_cache_checkpoint(app);

  /* load language */
  bx_language_load(app, conn);

  bx_log_debug("Contact data thread %lx", pthread_self());
  time_t start = time(NULL);
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    bx_contact_walk_items(app, conn, my_cache);
    bx_contact_prune_items(app, conn, my_cache);
    time_t current = time(NULL);
    if (current - start > cache_checkpoint) {
      cache_stats(my_cache, "contact");
      cache_store(my_cache, filename);
      start = current;
    }
    cache_next_version(my_cache);

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  thread_teardown_mysql(conn);
  cache_store(my_cache, filename);
  free(filename);
  cache_destroy(my_cache);
  return 0;
}

/**
 * Thread to synchronize pr_project endpoint
 */
void *project_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  bx_log_debug("Project data thread %ld", pthread_self());
  time_t start;
  time(&start);

  Cache *my_cache;
  my_cache = cache_create();
  if (my_cache == NULL) {
    bx_log_error("Cache init failed");
    return 0;
  }

  conn = thread_setup_mysql(app);

  bx_project_sync_cache_with_db(conn, my_cache);

  while (atomic_load(&app->queue->run)) {
    bx_project_walk_item(app, conn, my_cache);
    my_cache->version++;
    thrd_yield();
    if (time(NULL) - start > 5) {
      time(&start);
      cache_stats(my_cache, "projects");
    }
  }
  cache_destroy(my_cache);
  thread_teardown_mysql(conn);
  return 0;
}

#define CACHE_FILE_INVOICE "invoice.bin"
void *invoice_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  /* mysql setup */
  conn = thread_setup_mysql(app);

  /* filename for cache */
  char *filename = bx_utils_cache_filename(app, CACHE_FILE_INVOICE);
  if (!filename) {
    bx_log_debug("Failed allocation of cache filename %s", CACHE_FILE_INVOICE);
    return 0;
  }

  /* init cache */
  Cache *my_cache;
  my_cache = cache_create();
  if (my_cache == NULL) {
    bx_log_error("Cache init failed");
  }
  if (!cache_load(my_cache, filename)) {
    cache_empty(my_cache);
  }
  bx_contact_sync_cache_with_db(conn, my_cache);
  cache_invalidate(my_cache, 1);
  cache_prune(my_cache);
  int cache_checkpoint = bx_utils_cache_checkpoint(app);

  bx_log_debug("Invoice data thread %ld", pthread_self());
  time_t start = time(NULL);
  while (atomic_load(&app->queue->run)) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    bx_invoice_walk_items(app, conn, my_cache);
    bx_invoice_prune_items(app, conn, my_cache);
    time_t current = time(NULL);
    if (current - start > cache_checkpoint) {
      cache_stats(my_cache, "invoice");
      cache_store(my_cache, filename);
    }
    cache_next_version(my_cache);

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  cache_store(my_cache, filename);
  cache_destroy(my_cache);
  thread_teardown_mysql(conn);
  return 0;
}

/* ENTRY POINT */
int main(int argc, char **argv) {
  BXConf *conf = NULL;
  bXill app;

  bx_mutex_init(&MTX_COUNTRY_LIST);
  enum e_ThreadList {
    PROJECT_THREAD,
    CONTACT_THREAD,
    INVOICE_THREAD,
    CONTACT_SECTOR_THREAD,
    RANDOM_ITEM_THREAD,

    MAX__THREAD_LIST
  };
  pthread_t threads[MAX__THREAD_LIST];

  mysql_library_init(argc, argv, NULL);

  conf = bx_conf_init();
  if (!bx_conf_loadfile(conf, "conf.json")) {
    bx_conf_destroy(&conf);
    return -1;
  }
  bx_log_init(bx_conf_get_string(conf, "log-file"),
              bx_conf_get_int(conf, "log-level"));
  bx_conf_release(conf, "log-file");
  bx_utils_init();
  app.conf = conf;
  atomic_store(&app.logthread, true);
  pthread_t log_thread;
  pthread_create(&log_thread, NULL, bx_log_out_thread, (void *)&app);

  BXNet *net = bx_net_init(conf);
  if (net == NULL) {
    bx_log_error("Net configuration failed");
    exit(0);
  }
  app.net = net;
  /* START NET THREAD, REQUEST CAN BE DONE AFTER THAT */
  BXNetRequestList *queue = bx_net_request_list_init(net);
  app.queue = queue;
  pthread_t request_thread = bx_net_loop(queue);
  /* REQUEST AVAILABLE, LOAD SOME STUFF HERE*/

  assert(bx_country_code_load(&app) != false);

  /* RUN CODE TO UPDATE DATABASE */
  pthread_create(&threads[CONTACT_THREAD], NULL, contact_thread, (void *)&app);

  pthread_create(&threads[PROJECT_THREAD], NULL, project_thread, (void *)&app);
  pthread_create(&threads[INVOICE_THREAD], NULL, invoice_thread, (void *)&app);
  pthread_create(&threads[CONTACT_SECTOR_THREAD], NULL, contact_sector_thread,
                 (void *)&app);
  pthread_create(&threads[RANDOM_ITEM_THREAD], NULL, random_item_thread,
                 (void *)&app);

  bool exit = false;

  do {
    getc(stdin);
    exit = true;
  } while (!exit);

  atomic_store(&queue->run, 0);
  for (int i = 0; i < MAX__THREAD_LIST; i++) {
    bx_log_debug("Waiting for data thread %d", i);
    pthread_join(threads[i], NULL);
  }
  bx_log_debug("Waiting for request thread");
  pthread_join(request_thread, NULL);
  bx_country_list_free();
  bx_net_request_list_destroy(queue);
  bx_net_destroy(&net);
  bx_conf_destroy(&conf);
  mysql_library_end();

  atomic_store(&app.logthread, false);
  pthread_join(log_thread, NULL);
  bx_log_end();
  return 0;
}
