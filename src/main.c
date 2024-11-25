#include "bx_object_value.h"
#include "bxobjects/contact_group.h"
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

int main(int argc, char ** argv) 
{
    BXConf * conf = NULL;
    MYSQL * mysql = NULL;
    bXill app;

    mysql_library_init(argc, argv, NULL);
    mysql = mysql_init(mysql);
    app.mysql = mysql;

    bx_utils_init();

    conf = bx_conf_init();
    if (!bx_conf_loadfile(conf, "conf.json")) {
        bx_conf_destroy(&conf);
        return -1;
    }
    bx_conf_dump(conf);

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
        fprintf(stderr, "Net configuration failed");
        exit(0);
    }
    app.net = net;
    BXNetRequestList * queue = bx_net_request_list_init(net);
    app.queue = queue;
    pthread_t request_thread = bx_net_loop(queue);
    
    BXInteger item = {.isset = 1, .value = 2, .type = BX_OBJECT_TYPE_INTEGER };
    int round = 5;
    do {
        /* walk sector first */
        bx_contact_sector_walk_items(&app);
        bx_contact_walk_items(&app);
    } while(round-- > 0);


    /*
    BXNetRequest * request = NULL;
    request = bx_net_request_new(BXTypeContact, "2.0", "contact/2", NULL);
    uint64_t c2 = bx_net_request_list_add(queue, request);
    printf("REQUEST ID %ld", c2);
    while(atomic_load(&request->done) == false) { thrd_yield(); }
    request = bx_net_request_list_get_finished(queue, c2);
    if (request != NULL) {
        if (request->response != NULL) {
            if (request->response->data != NULL) {
                json_t * jroot = bx_decode_net(request);
                free(request->response->data);
                free(request->response);
                request->response = NULL;
                if (jroot != NULL) {
                    request->decoded = jroot;
                    const BXObjectFunctions * decoder = bx_decode_select_decoder(request->decoder);
                    void * object = NULL;
                    if (decoder->decode_function == NULL) {
                        fprintf(stderr, "No decoder found for '%d'\n", request->decoder);

                    } else {
                        if (json_is_array(request->decoded)) {
                            for (size_t i = 0; i < json_array_size(jroot); i++) {
                                object = decoder->decode_function(json_array_get(jroot, i));
                                decoder = bx_decode_select_decoder(*(enum e_BXObjectType *)object);
                                if (decoder->free_function != NULL) { decoder->free_function(object); }
                            }
                        } else {
                            object = decoder->decode_function(jroot);
                            decoder = bx_decode_select_decoder(*(enum e_BXObjectType *)object);
                            bx_object_contact_store(mysql, object);
                            if (decoder->free_function != NULL) { decoder->free_function(object); }
                        }
                    }
                } else {
                    fprintf(stderr, "Failed json decode\n");
                }
                
            }
        }
        bx_net_request_free(request);
        request = NULL;
    }
    */
    
    atomic_store(&queue->run, 0);
    pthread_join(request_thread, NULL);
    bx_net_request_list_destroy(queue);
    bx_net_destroy(&net);
    bx_conf_destroy(&conf);
    mysql_close(mysql);
    mysql_library_end();
    return 0;
}