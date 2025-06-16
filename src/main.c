#include "include/bx_conf.h"
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

#include <jansson.h>
#include <mariadb/ma_list.h>
#include <mariadb/mysql.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <threads.h>
#include <unistd.h>

#define MAX_COMMAND_LEN 100
extern BXMutex io_mutex;
extern BXMutex MTX_COUNTRY_LIST;

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

void *contact_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;

  conn = thread_setup_mysql(app);
  bx_log_debug("Contact data thread %lx", pthread_self());
  while (atomic_load(&(app->queue->run))) {
    bx_language_load(app, conn);
    bx_contact_sector_walk_items(app, conn);
    bx_contact_walk_items(app, conn);
    thrd_yield();
  }
  thread_teardown_mysql(conn);
  return 0;
}

void *project_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  bx_log_debug("Project data thread %ld", pthread_self());
  conn = thread_setup_mysql(app);
  while (atomic_load(&app->queue->run)) {
    bx_project_walk_item(app, conn);
    thrd_yield();
  }
  thread_teardown_mysql(conn);
  return 0;
}

void *invoice_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  bx_log_debug("Invoice data thread %ld", pthread_self());
  conn = thread_setup_mysql(app);
  while (atomic_load(&app->queue->run)) {
    bx_invoice_walk_items(app, conn);
    thrd_yield();
  }
  thread_teardown_mysql(conn);
  return 0;
}

int main(int argc, char **argv) {
  BXConf *conf = NULL;
  bXill app;

  bx_mutex_init(&MTX_COUNTRY_LIST);
  enum e_ThreadList {
    CONTACT_THREAD,
    PROJECT_THREAD,
    INVOICE_THREAD,
    MAX__THREAD_LIST
  };
  pthread_t threads[MAX__THREAD_LIST];

  mysql_library_init(argc, argv, NULL);
  bx_log_init();
  bx_utils_init();

  conf = bx_conf_init();
  if (!bx_conf_loadfile(conf, "conf.json")) {
    bx_conf_destroy(&conf);
    return -1;
  }
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
