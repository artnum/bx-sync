#include "include/bx_conf.h"
#include "include/bx_database.h"
#include "include/bx_ids_cache.h"
#include "include/bx_mutex.h"
#include "include/bx_net.h"
#include "include/bx_prune.h"
#include "include/bx_utils.h"
#include "include/bxill.h"
#include "include/bxobjects/contact.h"
#include "include/bxobjects/contact_sector.h"
#include "include/bxobjects/country_code.h"
#include "include/bxobjects/invoice.h"
#include "include/bxobjects/language.h"
#include "include/bxobjects/project.h"
#include "include/bxobjects/taxes.h"

#include <fcntl.h>
#include <jansson.h>
#include <mariadb/errmsg.h>
#include <mariadb/ma_list.h>
#include <mariadb/mysql.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#define MAX_COMMAND_LEN 100
#define MS_TO_NS 100000
extern BXMutex io_mutex;
extern BXMutex MTX_COUNTRY_LIST;
const struct timespec THREAD_SLEEP_TIME = {
    .tv_nsec = BXILL_THREAD_SLEEP_MS * MS_TO_NS, .tv_sec = 0};
const struct timespec THREAD_ERROR_SLEEP_TIME = {
    .tv_nsec = 3 * BXILL_THREAD_SLEEP_MS * MS_TO_NS, .tv_sec = 0};

void thread_blocks_signals() {
  sigset_t set;
  sigaddset(&set, SIGALRM);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
}

MYSQL *thread_setup_mysql(bXill *app) {
  MYSQL *conn = NULL;
  conn = mysql_init(NULL);
  if (conn == NULL) {
    return NULL;
  }
  mysql_thread_init();
  if (!mysql_real_connect(conn, bx_conf_get_string(app->conf, "mysql-host"),
                          bx_conf_get_string(app->conf, "mysql-user"),
                          bx_conf_get_string(app->conf, "mysql-password"),
                          bx_conf_get_string(app->conf, "mysql-database"), 0,
                          NULL, 0)) {
    return NULL;
  }
  bx_log_debug("New MYSQL Thread : %lu", mysql_thread_id(conn));
  mysql_set_character_set(conn, "utf8mb4");
  return conn;
}

void thread_teardown_mysql(MYSQL *conn) {
  mysql_close(conn);
  mysql_thread_end();
}
MYSQL *thread_reconnect(MYSQL *conn, bXill *app) {
  bx_log_info("Reconnect MYSQL previous thread : %lu", mysql_thread_id(conn));
  thread_teardown_mysql(conn);
  return thread_setup_mysql(app);
}

bool thread_handle_error(BXillError error, bXill *app, MYSQL **conn) {
  if (error == NoError) {
    return true;
  }
  int ping = mysql_ping(*conn);
  if (ping == 0) {
    return true;
  }
  switch (mysql_errno(*conn)) {
  case CR_SERVER_GONE_ERROR:
    *conn = thread_reconnect(*conn, app);
    if (*conn == NULL) {
      bx_log_error("Reconnect to MySQL failed");
      return false;
    }
    break;
  default:
    bx_log_error("Unknown error, sleeping a bit");
    thrd_sleep(&THREAD_ERROR_SLEEP_TIME, NULL);
    return false;
  }
  return true;
}

void *random_item_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  int error_counter = 0;
  BXillError RetVal = NoError;
  thread_blocks_signals();
  conn = thread_setup_mysql(app);
  bx_log_debug("Random items thread data thread %lx", pthread_self());
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    BXillError e = bx_taxes_walk_item(app, conn);
    if (thread_handle_error(e, app, &conn)) {
      error_counter = 0;
    } else {
      if (error_counter++ > BXILL_THREAD_EXIT_MAX_COUNT) {
        bx_log_error("Too much error, exiting");
        RetVal = EXIT_FAILURE;
        break;
      }
    }

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }

  thread_teardown_mysql(conn);
  return (void *)(intptr_t)RetVal;
}

void *contact_sector_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  int error_counter = 0;
  int RetVal = EXIT_SUCCESS;

  thread_blocks_signals();
  conn = thread_setup_mysql(app);
  if (!conn) {
    thread_teardown_mysql(conn);
    return (void *)EXIT_FAILURE;
  }
  bx_log_debug("Contact Sector data thread %lx", pthread_self());
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    BXillError e = bx_contact_sector_walk_items(app, conn);
    if (thread_handle_error(e, app, &conn)) {
      error_counter = 0;
    } else {
      if (error_counter++ > BXILL_THREAD_EXIT_MAX_COUNT) {
        bx_log_error("Too much error, exiting");
        RetVal = EXIT_FAILURE;
        break;
      }
    }

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  thread_teardown_mysql(conn);
  return (void *)(intptr_t)RetVal;
}

#define CACHE_FILE_CONTACT "contact.bin"
void *contact_thread(void *arg) {
  bXill *app = (bXill *)arg;
  int RetVal = EXIT_SUCCESS;
  int error_counter = 0;

  thread_blocks_signals();
  /* mysql init */
  MYSQL *conn = NULL;
  conn = thread_setup_mysql(app);
  if (!conn) {
    thread_teardown_mysql(conn);
    bx_log_error("Cannot set up MYSQL");
    return (void *)EXIT_FAILURE;
  }
  /* filename for cache */
  char *filename = bx_utils_cache_filename(app, CACHE_FILE_CONTACT);
  if (!filename) {
    thread_teardown_mysql(conn);
    bx_log_debug("Failed allocation of cache filename %s", CACHE_FILE_CONTACT);
    return (void *)EXIT_FAILURE;
  }

  /* init cache */
  Cache *my_cache;
  my_cache = cache_create();
  if (my_cache == NULL) {
    bx_log_error("Cache init failed");
    thread_teardown_mysql(conn);
    free(filename);
    return (void *)EXIT_FAILURE;
  }
  if (!cache_load(my_cache, filename)) {
    /* loading failed, empty just in case */
    cache_empty(my_cache);
  }
  /* validate cache with database */
  PruningParameters prune_one_source_of_truth = {
      .cache = my_cache,
      .query =
          bx_database_new_query(conn, "SELECT id, _checksum FROM contact")};
  /* sync */
  if (bx_prune_from_db(app, &prune_one_source_of_truth) == ErrorSQLReconnect) {
    conn = thread_reconnect(conn, app);
    if (!conn) {
      bx_database_free_query(prune_one_source_of_truth.query);
      thread_teardown_mysql(conn);
      bx_log_error("Cannot set up MYSQL");
      return (void *)EXIT_FAILURE;
    }
  }
  bx_database_free_query(prune_one_source_of_truth.query);

  int cache_checkpoint = bx_utils_cache_checkpoint(app);

  /* load language */
  bx_language_load(app, conn);

  PruningParameters contact_prune = {
      .query =
          bx_database_new_query(conn, "DELETE FROM contact WHERE id = :id"),
      .cache = my_cache};
  bx_log_debug("Contact data thread %lx", pthread_self());
  time_t start = time(NULL);
  while (atomic_load(&(app->queue->run))) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    BXillError e = NoError;
    (void)((e = bx_contact_walk_items(app, conn, my_cache)) == NoError &&
           (e = bx_prune_items(app, &contact_prune)) == NoError);
    if (thread_handle_error(e, app, &conn)) {
      error_counter = 0;
    } else {
      if (error_counter++ > BXILL_THREAD_EXIT_MAX_COUNT) {
        bx_log_error("Too much error, exiting");
        RetVal = EXIT_FAILURE;
        break;
      }
    }

    time_t current = time(NULL);
    if (current - start > cache_checkpoint) {
      cache_stats(my_cache, "contact");
      cache_store(my_cache, filename);
      start = current;
    }
    cache_next_version(my_cache);

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  bx_database_free_query(contact_prune.query);
  thread_teardown_mysql(conn);
  cache_store(my_cache, filename);
  free(filename);
  cache_destroy(my_cache);
  return (void *)(intptr_t)RetVal;
}

/**
 * Thread to synchronize pr_project endpoint
 */
#define CACHE_FILE_PROJECT "project.bin"
void *project_thread(void *arg) {
  bXill *app = (bXill *)arg;
  int RetVal = EXIT_SUCCESS;
  int error_counter = 0;
  thread_blocks_signals();
  /* mysql setup */
  MYSQL *conn = NULL;
  conn = thread_setup_mysql(app);

  /* cache filename */
  char *filename = bx_utils_cache_filename(app, CACHE_FILE_PROJECT);
  if (!filename) {
    bx_log_error("Failed allocation of cache filename %s", CACHE_FILE_PROJECT);
    return 0;
  }

  /* init cache */
  Cache *my_cache = cache_create();
  if (!my_cache) {
    bx_log_error("Cache init failed");
    free(filename);
    return 0;
  }
  if (!cache_load(my_cache, filename)) {
    cache_empty(my_cache);
  }

  PruningParameters prune_one_source_of_truth = {
      .cache = my_cache,
      .query =
          bx_database_new_query(conn, "SELECT id, _checksum FROM pr_project")};
  /* sync */
  bx_prune_from_db(app, &prune_one_source_of_truth);
  bx_database_free_query(prune_one_source_of_truth.query);

  int cache_checkpoint = bx_utils_cache_checkpoint(app);

  PruningParameters project_prune = {
      .query =
          bx_database_new_query(conn, "DELETE FROM contact WHERE id = :id"),
      .cache = my_cache};

  bx_log_debug("Project data thread %ld", pthread_self());
  time_t start = time(NULL);
  while (atomic_load(&app->queue->run)) {
    BXillError e = NoError;
    (void)((e = bx_project_walk_item(app, conn, my_cache)) == NoError &&
           (e = bx_prune_items(app, &project_prune)) == NoError);
    if (thread_handle_error(e, app, &conn)) {
      error_counter = 0;
    } else {
      if (error_counter++ > BXILL_THREAD_EXIT_MAX_COUNT) {
        bx_log_error("Too much error, exiting");
        RetVal = EXIT_FAILURE;
        break;
      }
    }

    time_t current = time(NULL);
    if (current - start > cache_checkpoint) {
      cache_stats(my_cache, "projects");
      cache_store(my_cache, filename);
      start = current;
    }
    cache_next_version(my_cache);

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  bx_database_free_query(project_prune.query);
  thread_teardown_mysql(conn);
  cache_store(my_cache, filename);
  free(filename);
  cache_destroy(my_cache);
  return (void *)(intptr_t)RetVal;
}

#define CACHE_FILE_INVOICE "invoice.bin"
void *invoice_thread(void *arg) {
  bXill *app = (bXill *)arg;
  MYSQL *conn = NULL;
  BXillError RetVal = NoError;
  int error_counter = 0;
  thread_blocks_signals();
  /* mysql setup */
  conn = thread_setup_mysql(app);

  /* filename for cache */
  char *filename = bx_utils_cache_filename(app, CACHE_FILE_INVOICE);
  if (!filename) {
    bx_log_error("Failed allocation of cache filename %s", CACHE_FILE_INVOICE);
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

  PruningParameters prune_one_source_of_truth = {
      .cache = my_cache,
      .query =
          bx_database_new_query(conn, "SELECT id, _checksum FROM invoice")};
  /* sync */
  bx_prune_from_db(app, &prune_one_source_of_truth);
  bx_database_free_query(prune_one_source_of_truth.query);

  int cache_checkpoint = bx_utils_cache_checkpoint(app);
  PruningParameters invoice_prune = {
      .query =
          bx_database_new_query(conn, "DELETE FROM invoice WHERE id = :id"),
      .cache = my_cache};

  bx_log_debug("Invoice data thread %ld", pthread_self());
  time_t start = time(NULL);
  while (atomic_load(&app->queue->run)) {
    while (atomic_load(&app->queue->standby)) {
      sleep(BXILL_STANDBY_SECONDS);
    }
    BXillError e = NoError;
    (void)((e = bx_invoice_walk_items(app, conn, my_cache)) == NoError &&
           (e = bx_prune_items(app, &invoice_prune)) == NoError);

    if (thread_handle_error(e, app, &conn)) {
      error_counter = 0;
    } else {
      if (error_counter++ > BXILL_THREAD_EXIT_MAX_COUNT) {
        bx_log_error("Too much error, exiting");
        RetVal = EXIT_FAILURE;
        break;
      }
    }

    time_t current = time(NULL);
    if (current - start > cache_checkpoint) {
      cache_stats(my_cache, "invoice");
      cache_store(my_cache, filename);
    }
    cache_next_version(my_cache);

    thrd_sleep(&THREAD_SLEEP_TIME, NULL);
  }
  bx_database_free_query(invoice_prune.query);
  cache_store(my_cache, filename);
  free(filename);
  cache_destroy(my_cache);
  thread_teardown_mysql(conn);
  return (void *)(intptr_t)RetVal;
}

/* signal handler */
bool kill_signal = 0;
void signal_hander(int sig) {
  switch (sig) {
  case SIGINT:
  case SIGTERM:
    kill_signal = true;
    bx_log_info("Kill signal received");
    break;
  case SIGHUP:
    bx_log_reopen();
    break;
  default:
    break;
  }
}

/* deamonizer */
int deamon(int fork_once) {
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    bx_log_write("Cannot fork");
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  if (setsid() < 0) {
    bx_log_write("Cannot set session id");
    exit(EXIT_FAILURE);
  }

  /* double forking if not configured otherwise */
  if (fork_once == 0) {
    pid = fork();
    if (pid < 0) {
      bx_log_write("Cannot fork again");
      exit(EXIT_FAILURE);
    }
    if (pid > 0) {
      exit(EXIT_SUCCESS);
    }
  }
  if (chdir("/") < 0) {
    bx_log_write("Cannot change directory");
  }
  umask(0);

  /* doing close open for stdin, stdout, stderr */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  int fd = open("/dev/null", O_RDWR);
  if (fd < 0) {
    bx_log_write("Cannot open /dev/null");
    exit(EXIT_FAILURE);
  }
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDERR_FILENO);
  dup2(fd, STDOUT_FILENO);
  if (fd > 2) {
    close(fd);
  }

  return 0;
}

#define PID_FILE "/run/bxsync.pid"
int write_pid(const char *pid_file) {
  char pidcontent[32];
  int fd = -1;
  if (pid_file == NULL) {
    fd = open(PID_FILE, O_WRONLY | O_CREAT, 0644);
  } else {
    fd = open(pid_file, O_WRONLY | O_CREAT, 0644);
  }
  if (fd < 0) {
    bx_log_write("Log file could not be opened");
    return -1;
  }

  if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    bx_log_write("Log file could not be locked");
    close(fd);
    return -1;
  }

  memset(pidcontent, 0, 32);
  snprintf(pidcontent, 32, "%d\n", getpid());
  size_t len = strlen(pidcontent);
  if (write(fd, pidcontent, len) < len) {
    bx_log_write("Log file could not be written");
    close(fd);
    return -1;
  }
  fsync(fd);
  return fd;
}

/* ENTRY POINT */
int main(int argc, char **argv) {
  BXConf *conf = NULL;
  bXill app;

  if (argc < 2) {
    printf("Missing config file as first argument");
    exit(EXIT_FAILURE);
  }

  conf = bx_conf_init();
  if (!bx_conf_loadfile(conf, argv[1])) {
    bx_conf_destroy(&conf);
    exit(EXIT_FAILURE);
  }

  bx_log_init(&app, bx_conf_get_string(conf, "log-file"),
              bx_conf_get_int(conf, "log-level"));

  int pid_file_fd = -1;
  if (bx_conf_get_int(conf, "no-deamon") == 0) {
    if (deamon(bx_conf_get_int(conf, "fork-once")) != 0) {
      bx_log_write("Error going deamon");
      bx_conf_destroy(&conf);
      bx_log_close();
      exit(EXIT_FAILURE);
    }
    pid_file_fd = write_pid(bx_conf_get_string(conf, "pid-file"));
    if (pid_file_fd < 0) {
      bx_log_write("Error writing pid");
      bx_conf_destroy(&conf);
      bx_log_close();
      exit(EXIT_FAILURE);
    }

    setuid(bx_conf_get_int(conf, "uid"));
    setgid(bx_conf_get_int(conf, "gid"));
  }

  bx_mutex_init(&MTX_COUNTRY_LIST);
  pthread_t threads[MAX__THREAD_LIST];

  struct sigaction sa = {.sa_handler = signal_hander, .sa_flags = 0};
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    printf("Failed setup signal handler\n");
    return 1;
  }
  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    printf("Failed setup signal handler\n");
    return 1;
  }
  if (sigaction(SIGHUP, &sa, NULL) == -1) {
    printf("Failed setup signal handler\n");
    return 1;
  }

  mysql_library_init(argc, argv, NULL);

  bx_conf_release(conf, "log-file");
  bx_utils_init();
  app.conf = conf;
  atomic_store(&app.logthread, true);
  index_init(&app.indexes);
  index_new(&app.indexes, "User");
  index_new(&app.indexes, "Contact");
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
  pthread_create(&threads[RANDOM_ITEM_THREAD], NULL, random_item_thread,
                 (void *)&app);
  while (kill_signal == 0) {
    pause();
  }
  atomic_store(&queue->run, 0);
  sleep(5);

  pthread_cond_signal(&queue->in_cond);
  bx_log_debug("Waiting for request thread");
  pthread_join(request_thread, NULL);

  pthread_cond_broadcast(&queue->out_cond);
  for (int i = 0; i < MAX__THREAD_LIST; i++) {
    bx_log_debug("Waiting for data thread %d", i);
    pthread_join(threads[i], NULL);
  }
  bx_country_list_free();
  bx_net_request_list_destroy(queue);
  bx_net_destroy(&net);
  atomic_store(&app.logthread, 0);
  pthread_join(log_thread, NULL);
  bx_conf_destroy(&conf);
  mysql_library_end();

  bx_log_end();
  if (pid_file_fd != -1) {
    close(pid_file_fd);
  }
  return 0;
}
