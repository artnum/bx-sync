#ifndef BX_NET_H__
#define BX_NET_H__

#include <bx_mutex.h>
#include <bx_conf.h>
#include <curl/curl.h>
#include <jansson.h>
#include <bx_object.h>

typedef struct s_BXNetRatelimit BXNetRatelimit;
struct s_BXNetRatelimit {
    int max_request;
    int remaining_request;
    int reset_time;
};
typedef struct s_BXNet BXNet;
struct s_BXNet {
    CURL * curl;
    BXMutex mutex;
    char * auth_token;
    size_t auth_token_len;
    char * endpoint;
    size_t endpoint_len;
    BXMutex mutex_limit;
    BXNetRatelimit limits;
};

typedef struct s_BXNetRData BXNetRData;
struct s_BXNetRData {
    char * data;
    size_t data_len;
};

typedef struct s_BXNetURLParams BXNetURLParams;
struct s_BXNetURLParams {
    char * name;
    char * value;
    BXNetURLParams * next;
};

typedef struct s_BXNetRequest BXNetRequest;
struct s_BXNetRequest {
    atomic_bool done;
    uint64_t id;
    char * path;
    json_t * decoded;
    json_t * body;
    BXNetURLParams * params;
    BXNetRData * response;
    BXNetRequest * next;
};

typedef struct s_BXNetRequestList BXNetRequestList;
struct s_BXNetRequestList {
    atomic_bool run;
    uint64_t next_id;
    BXNet * net;
    BXNetRequest * head;
    BXMutex mutex;
};

BXNetRData * bx_fetch(BXNet * net, const char * path, BXNetURLParams * params);
BXNet * bx_net_init(BXConf * conf);
void bx_net_destroy(BXNet ** net);
pthread_t  bx_net_loop(BXNetRequestList * list);

BXNetRequestList * bx_net_request_list_init(BXNet * net);
uint64_t bx_net_request_list_add(BXNetRequestList * list, BXNetRequest * request);
BXNetRequest * bx_net_request_list_remove(BXNetRequestList * list, bool done);
int bx_net_request_list_count(BXNetRequestList * list);
void bx_net_request_list_destroy(BXNetRequestList * list);
bool bx_net_request_add_param(BXNetRequest * request, const char * name, const char * value);
BXNetRequest * bx_net_request_new(
    const char * path,
    json_t * body
);
BXNetRequest * bx_net_request_list_get_finished(
    BXNetRequestList * list,
    uint64_t request_id
);
void bx_net_request_free(BXNetRequest * request);
#endif