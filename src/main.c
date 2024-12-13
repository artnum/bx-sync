#include "bx_mutex.h"
#include "bx_object_value.h"
#include "bxobjects/contact_group.h"
#include "bxobjects/country_code.h"
#include "bxobjects/language.h"
#include <bxobjects/contact.h>
#include <bx_conf.h>
#include <bx_net.h>
#include <bx_decode.h>
#include <bx_object.h>
#include <bxobjects/invoice.h>
#include <bxobjects/contact_sector.h>
#include <bx_utils.h>
#include <bxill.h>

#include <jansson.h>
#include <mariadb/ma_list.h>
#include <mariadb/mysql.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/ioctl.h>

#define MAX_COMMAND_LEN 100
extern BXMutex io_mutex;
extern BXMutex MTX_COUNTRY_LIST;

void * contact_thread(void * arg)
{
    bXill * app = (bXill *)arg;
    bx_log_debug("Contact data thread %lx", pthread_self());
    while(atomic_load(&(app->queue->run))) {
        bx_language_load(app);
        bx_contact_sector_walk_items(app);
        bx_contact_walk_items(app);
    }
    return 0;
}

int main(int argc, char ** argv) 
{
    BXConf * conf = NULL;
    MYSQL * mysql = NULL;
    bXill app;

    bx_mutex_init(&MTX_COUNTRY_LIST);

    enum e_ThreadList {
        CONTACT_THREAD,
        MAX__THREAD_LIST
    };
    pthread_t threads[MAX__THREAD_LIST];

    mysql_library_init(argc, argv, NULL);
    mysql = mysql_init(mysql);
    app.mysql = mysql;

    bx_log_init();
    bx_utils_init();

    conf = bx_conf_init();
    if (!bx_conf_loadfile(conf, "conf.json")) {
        bx_conf_destroy(&conf);
        return -1;
    }

    mysql_real_connect(
        mysql,
        bx_conf_get_string(conf, "mysql-host"),
        bx_conf_get_string(conf, "mysql-user"),
        bx_conf_get_string(conf, "mysql-password"),
        bx_conf_get_string(conf, "mysql-database"),
        0,
        NULL,
        0
    );
    bx_conf_release(conf, "mysql-host");
    bx_conf_release(conf, "mysql-user");
    bx_conf_release(conf, "mysql-password");
    bx_conf_release(conf, "mysql-database");
    mysql_set_character_set(mysql, "utf8mb4");
    BXNet * net = bx_net_init(conf);
    if (net == NULL) {
        bx_log_error("Net configuration failed");
        exit(0);
    }
    app.net = net;
    /* START NET THREAD, REQUEST CAN BE DONE AFTER THAT */
    BXNetRequestList * queue = bx_net_request_list_init(net);
    app.queue = queue;
    pthread_t request_thread = bx_net_loop(queue);

    /* REQUEST AVAILABLE, LOAD SOME STUFF HERE*/

    assert(bx_country_code_load(&app) != false);

    /* RUN CODE TO UPDATE DATABASE */
    pthread_create(
        &threads[CONTACT_THREAD],
        NULL,
        contact_thread,
        (void *)&app
    );

    bool exit = false;

    do {
        getc(stdin);
        exit = true;
    } while(!exit);

    atomic_store(&queue->run, 0);
    for (int i = 0; i < MAX__THREAD_LIST; i++) {
        bx_log_debug("Waiting for data thread %d", i);
        pthread_join(threads[i], NULL);
    }
    bx_log_debug("Waiting for request thread");
    pthread_join(request_thread, NULL);
    bx_net_request_list_destroy(queue);
    bx_net_destroy(&net);
    bx_conf_destroy(&conf);
    mysql_close(mysql);
    mysql_library_end();

    assert(bx_mutex_lock(&io_mutex) != false);
    bx_log_end();
    return 0;
}