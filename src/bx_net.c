#include "bx_utils.h"
#include <bx_net.h>
#include <bx_mutex.h>
#include <bx_conf.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/urlapi.h>
#include <jansson.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define BX_API_AUTH_HEADER  "Authorization: Bearer "

BXNet * bx_net_init(BXConf * conf)
{
    BXNet * net = NULL;

    const char * t = bx_conf_get_string(conf, "bexio-token");
    if (t == NULL) {
        return NULL;    
    }
    size_t slen = strlen(t);
    char * token = calloc(slen + 1, sizeof(*token));
    if (token == NULL) {
        bx_conf_release(conf, "bexio-token");
        return NULL;
    }
    memcpy(token, t, slen);
    bx_conf_release(conf, "bexio-token");

    const char * e = bx_conf_get_string(conf, "bexio-endpoint");
    if (e == NULL) {
        free(token);
        return NULL;
    }
    size_t elen = strlen(e);
    char * endpoint = calloc(elen + 1, sizeof(*endpoint));
    if (endpoint == NULL) {
        bx_conf_release(conf, "bexio-endpoint");
        free(token);
        return NULL;
    }
    memcpy(endpoint, e, elen);
    bx_conf_release(conf, "bexio-endpoint");

    net = calloc(1, sizeof(*net));
    if (net == NULL) {
        free(token);
        free(endpoint);
        return NULL;
    }
    net->auth_token = token;
    net->auth_token_len = slen;
    net->endpoint = endpoint;
    net->endpoint_len = elen;
    net->curl = curl_easy_init();
    if (!net->curl) {
        free(net);
        return NULL;
    }
    
    bx_mutex_init(&net->mutex);
    bx_mutex_init(&net->mutex_limit);
    return net;
}

size_t _bx_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    BXNetRData * netdata = (BXNetRData *)userdata;
    if (size == 0) { return 0; }
    char * tmp = NULL;
    tmp = realloc(netdata->data, (netdata->data_len + (size * nmemb) + 1));
    if (tmp == NULL) {
        return 0;
    }
    netdata->data = tmp;
    memset(netdata->data + netdata->data_len, 0, (size * nmemb) + 1);
    memcpy(netdata->data + netdata->data_len, ptr, (size * nmemb));
    netdata->data_len += (size * nmemb);
    return (size * nmemb);
}

void bx_net_destroy(BXNet ** net)
{
    assert(net != NULL);
    assert(*net != NULL);
    bx_mutex_lock(&(*net)->mutex);
    if ((*net)->endpoint != NULL) { free((*net)->endpoint); }
    if ((*net)->auth_token != NULL) { free((*net)->auth_token); }
    if ((*net)->curl != NULL) { curl_easy_cleanup((*net)->curl); }
    free(*net);
    *net = NULL;
}


inline static char * _bx_build_url(
    const char * endpoint,
    size_t endpoint_len,
    const char * path,
    BXNetURLParams * head
)
{
    assert(path != NULL);

    size_t url_len = 0;
    if (head != NULL) {
        BXNetURLParams * current = head;
        while (current != NULL) {
            url_len++; // add the '=' betwen name and value
            url_len += strlen(current->name);
            url_len += strlen(current->value);
            current = current->next;
            if (current != NULL) {
                url_len++; // add the '&' in betwen
            }
        }
    }
    url_len += endpoint_len + 1; // add the '/'
    url_len += strlen(path) + 1; // add the '?' or '/'

    char * url = NULL;
    url = calloc(url_len + 1, sizeof(*url)); // add the '\0' at the end
    if (url == NULL) {
        return NULL;
    }
    if (snprintf(url, url_len, "%s/%s", endpoint, path) < 0) {
        free(url);
        return NULL;
    }

    /* remove // in the path */
    int j = endpoint_len - 2;
    bool prev_was_slash = false;
    for(int i = endpoint_len - 2; i < url_len && url[i] != '\0'; i++) {
        if (prev_was_slash && url[i] == '/') { continue; }
        if (prev_was_slash) { prev_was_slash = false; }
        if (url[i] == '/') { prev_was_slash = true; }
        url[j++] = url[i];
    }
    memset(&url[j], 0, url_len - j);

    if (head != NULL) {
        url[j++] = '?';
        while (head != NULL) {
            size_t name_len = strlen(head->name);
            memcpy(&url[j], head->name, name_len);
            j += name_len;
            url[j++] = '=';
            size_t value_len = strlen(head->value);
            memcpy(&url[j], head->value, value_len);
            j += value_len;
            head = head->next;
            if (head != NULL) {
                url[j++] = '&';
            }
        }
    }
    return url;
}

#define RLIMIT_LIMIT    "ratelimit-limit"
#define RLIMIT_REMAIN   "ratelimit-remaining"
#define RLIMIT_RESET    "ratelimit-reset"

static inline size_t bx_header_callback(char * buffer, size_t size, size_t item_count, void * data)
{
    char _b[10];
    char * b = &_b[0];
    BXNet * net = (BXNet *)data;
    
    assert(net != NULL);
    if (buffer == NULL || item_count == 0) { return size * item_count; }
    memset(b, 0, 10);
    if (item_count * size > sizeof(RLIMIT_LIMIT)
        && bx_string_compare(buffer, RLIMIT_LIMIT, sizeof(RLIMIT_LIMIT))
        && (size * item_count) - sizeof(RLIMIT_LIMIT) < 10
    ) {
        memcpy(b, &buffer[sizeof(RLIMIT_LIMIT) + 1], (size * item_count) - sizeof(RLIMIT_LIMIT));
        while ((*b == ' ' || *b == ':') && *b != '\0') { b++; }
        if (*b == 0) { return size * item_count; }
        int v = strtol(b, NULL, 10);
        if (v == 0) { return size * item_count; }
        bx_mutex_lock(&net->mutex_limit);
        net->limits.max_request = v;
        bx_mutex_unlock(&net->mutex_limit);
        return size * item_count;
    }

    if (item_count * size > sizeof(RLIMIT_REMAIN)
        && bx_string_compare(buffer, RLIMIT_REMAIN, sizeof(RLIMIT_REMAIN))
        && (size * item_count) - sizeof(RLIMIT_REMAIN) < 10
    ) {
        memcpy(b, &buffer[sizeof(RLIMIT_REMAIN) + 1], (size * item_count) - sizeof(RLIMIT_REMAIN));
        while ((*b == ' ' || *b == ':') && *b != '\0') { b++; }
        if (*b == 0) { return size * item_count; }
        int v = strtol(b, NULL, 10);
        if (v == 0) { return size * item_count; }
        bx_mutex_lock(&net->mutex_limit);
        net->limits.remaining_request = v;
        bx_mutex_unlock(&net->mutex_limit);
        return size * item_count;
    }

    if (item_count * size > sizeof(RLIMIT_RESET)
        && bx_string_compare(buffer, RLIMIT_RESET, sizeof(RLIMIT_RESET))
        && (size * item_count) - sizeof(RLIMIT_RESET) < 10
    ) {
        memcpy(b, &buffer[sizeof(RLIMIT_RESET) + 1], (size * item_count) - sizeof(RLIMIT_RESET));
        while ((*b == ' ' || *b == ':') && *b != '\0') { b++; }
        if (*b == 0) { return size * item_count; }
        int v = strtol(b, NULL, 10);
        if (v == 0) { return size * item_count; }
        bx_mutex_lock(&net->mutex_limit);
        net->limits.reset_time = v;
        bx_mutex_unlock(&net->mutex_limit);
        return size * item_count;
    }

    return size * item_count;
}

BXNetRData * bx_fetch(BXNet * net, const char * path, BXNetURLParams * params)
{
    assert(net != NULL);
    assert(path != NULL);

    bx_mutex_lock(&net->mutex);    
    char * url = _bx_build_url(net->endpoint, net->endpoint_len, path, params);
    if (url == NULL) {
        bx_mutex_unlock(&net->mutex);
        return NULL;
    }
    bx_mutex_unlock(&net->mutex);
    /* read data structure setup */
    BXNetRData * net_rdata = NULL;
    net_rdata = calloc(1, sizeof(*net_rdata));
    if (net_rdata == NULL) {
        free(url);
        return NULL;
    }
    net_rdata->data_len = 0;
    net_rdata->data = NULL;

    /* auth token setup */
    size_t slen = 0;
    slen = strlen(net->auth_token) + sizeof(BX_API_AUTH_HEADER) + 1;
    char * auth_token = calloc(slen, sizeof(*auth_token));
    if (auth_token == NULL) {
        free(url);
        return NULL;
    }
    if(snprintf(auth_token, slen, "%s%s", BX_API_AUTH_HEADER, net->auth_token) < 0) {
        free(url);
        return NULL;
    }

    /* header setup */
    struct curl_slist * header_list = NULL;
    struct curl_slist * tmp = NULL;
    tmp = curl_slist_append(tmp, auth_token);
    if (tmp == NULL) {
        free(url);
        return NULL;
    }
    header_list = tmp;
    tmp = curl_slist_append(header_list, "Accept: application/json");
    if (tmp == NULL) {
        curl_slist_free_all(header_list);
        free(url);
        return NULL;
    }
    header_list = tmp;

    /* critical section */
    bx_mutex_lock(&net->mutex);
    if (curl_easy_setopt(net->curl, CURLOPT_HEADERFUNCTION, bx_header_callback) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_HEADERDATA, (void *)net) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_WRITEFUNCTION, _bx_write_cb) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_WRITEDATA, (void *)net_rdata) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_HTTPHEADER, header_list) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_URL, url) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    if (curl_easy_setopt(net->curl, CURLOPT_SSL_VERIFYPEER, 0) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    clock_t start = clock();
    if (curl_easy_perform(net->curl) != CURLE_OK) {
        goto failUnlockFreeAndReturn;
    }
    clock_t stop = clock();
    net->request_count++;
    net->average_request_time = ((stop - start) + net->average_request_time * net->request_count) / (net->request_count);
    bx_log_error("[NET TIME] Immediate : %ld us With Average %ld us\n", stop - start, net->average_request_time);
    curl_easy_getinfo(net->curl, CURLINFO_RESPONSE_CODE, &net_rdata->http_code);

    curl_slist_free_all(header_list);
    curl_easy_reset(net->curl);
    bx_mutex_unlock(&net->mutex);


    free(auth_token);
    free(url);

    return net_rdata;

/* fail handling */
failUnlockFreeAndReturn:
    curl_slist_free_all(header_list);
    curl_easy_reset(net->curl);
    bx_mutex_unlock(&net->mutex);
    free(net_rdata);
    free(url);
    return NULL;
}

static inline bool _bx_net_request_list_add(BXNetRequestList * list, BXNetRequest * request)
{
    BXNetRequest * current = NULL;
    
    if (list->head == NULL) {
        list->head = request;
    } else {
        current = list->head;
        while(current->next != NULL) { current = current->next; }
        current->next = request;
    }
    return true;
}

uint64_t bx_net_request_list_add(BXNetRequestList * list, BXNetRequest * request)
{
    assert(list != NULL);
    assert(request != NULL);
    uint64_t retval = 0;
    bx_mutex_lock(&list->mutex);
    if (request->id == 0) { request->id = ++list->next_id; }
    if (_bx_net_request_list_add(list, request)) {
        retval = request->id;
    }
    bx_mutex_unlock(&list->mutex);
    return retval;
}

BXNetRequest * bx_net_request_list_get_finished(
    BXNetRequestList * list,
    uint64_t request_id
)
{
    assert(list != NULL);
    BXNetRequest * retval = NULL;
    BXNetRequest * current = NULL;
    BXNetRequest * previous = NULL;

    bx_mutex_lock(&list->mutex);
    for(current = list->head; current != NULL; current = current->next) {
        if (current->id == request_id) {
            if (atomic_load(&current->done) == false) {
                break;
            }
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            current->next = NULL;
            retval = current;
            break;
        }
        previous = current;
    }
    bx_mutex_unlock(&list->mutex);
    return retval;
}

BXNetRequest * _bx_next_request_list_remove(BXNetRequestList * list, bool done)
{
    BXNetRequest * current = NULL;
    BXNetRequest * previous = NULL;

    for (current = list->head; current != NULL; current = current->next) {
        if (atomic_load(&current->done) == done) {
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            current->next = NULL;
            return current;
        }
    }
    return NULL;
}

BXNetRequest * bx_net_request_list_remove(BXNetRequestList * list, bool done)
{
    assert(list != NULL);
    BXNetRequest * retval = NULL;
    bx_mutex_lock(&list->mutex);
    retval = _bx_next_request_list_remove(list, done);
    bx_mutex_unlock(&list->mutex);
    return retval;
}

BXNetRequestList * bx_net_request_list_init(BXNet * net) 
{
    BXNetRequestList * list = calloc(1, sizeof(*list));
    if (list == NULL) {
        return NULL;
    }
    atomic_store(&list->run, true);
    list->head = NULL;
    list->net = net;
    bx_mutex_init(&list->mutex);
    return list;
}

void bx_net_request_list_destroy(BXNetRequestList * list)
{
    BXNetRequest * current = NULL;
    BXNetRequest * next = NULL;
    bx_mutex_lock(&list->mutex);
    current = list->head;
    while (current != NULL) {
        next = current->next;
        if (current->path != NULL) { free(current->path); }
        if (current->decoded != NULL) { json_decref(current->decoded); }
        if (current->response != NULL) { free(current->response); }
        free(current);
        current = next;
    }
    free(list);
}

int bx_net_request_list_count(BXNetRequestList * list)
{
    assert(list != NULL);
    BXNetRequest * current;
    int i = 0;
    bx_mutex_lock(&list->mutex);
    for(current = list->head; current; current = current->next) {
        i++;
    }
    bx_mutex_unlock(&list->mutex);
    return i;
}

BXNetRequest * bx_net_request_new(
    const char * path,
    json_t * body
)
{
    assert(path != NULL);
    
    BXNetRequest * new = NULL;
    new = calloc(1, sizeof(*new));
    if (new == NULL) {
        return NULL;
    }
    new->decoded = NULL;
    atomic_store(&new->done, false);
    new->next = NULL;
    new->params = NULL;
    new->response = NULL;
    new->path = strdup(path);
    new->body = NULL;

    return new;
}

void bx_net_request_params_free(BXNetURLParams * params)
{
    if (params == NULL) { return; }
    BXNetURLParams * next = NULL;
    while(params != NULL) {
        next = params->next;
        if (params->value != NULL) { free(params->value); }
        if (params->name != NULL) { free(params->name); }
        free(params);
        params = next;
    }
}

static bool _bx_encode_char_percent_encoding (char * url, size_t size, char c)
{
    switch(c) {
        case '~': if (url != NULL && size >= 3) { memcpy(url, "%7E", 3); } return true;
        case '!': if (url != NULL && size >= 3) { memcpy(url, "%21", 3); } return true;
        case '#': if (url != NULL && size >= 3) { memcpy(url, "%23", 3); } return true;
        case '$': if (url != NULL && size >= 3) { memcpy(url, "%24", 3); } return true;
        case '&': if (url != NULL && size >= 3) { memcpy(url, "%26", 3); } return true;
        case '(': if (url != NULL && size >= 3) { memcpy(url, "%28", 3); } return true;
        case ')': if (url != NULL && size >= 3) { memcpy(url, "%29", 3); } return true;
        case '*': if (url != NULL && size >= 3) { memcpy(url, "%2A", 3); } return true;
        case '+': if (url != NULL && size >= 3) { memcpy(url, "%2B", 3); } return true;
        case ',': if (url != NULL && size >= 3) { memcpy(url, "%2C", 3); } return true; 	
        case '/': if (url != NULL && size >= 3) { memcpy(url, "%2F", 3); } return true;
        case ':': if (url != NULL && size >= 3) { memcpy(url, "%3A", 3); } return true;
        case ';': if (url != NULL && size >= 3) { memcpy(url, "%3B", 3); } return true;
        case '=': if (url != NULL && size >= 3) { memcpy(url, "%3D", 3); } return true;
        case '?': if (url != NULL && size >= 3) { memcpy(url, "%3F", 3); } return true;
        case '@': if (url != NULL && size >= 3) { memcpy(url, "%40", 3); } return true;
        case '[': if (url != NULL && size >= 3) { memcpy(url, "%5B", 3); } return true;
        case ']': if (url != NULL && size >= 3) { memcpy(url, "%5D", 3); } return true;
        case '"': if (url != NULL && size >= 3) { memcpy(url, "%22", 3); } return true;
        case '%': if (url != NULL && size >= 3) { memcpy(url, "%25", 3); } return true;
        case '-': if (url != NULL && size >= 3) { memcpy(url, "%2D", 3); } return true;
        case '.': if (url != NULL && size >= 3) { memcpy(url, "%2E", 3); } return true;
        case '<': if (url != NULL && size >= 3) { memcpy(url, "%3C", 3); } return true;
        case '>': if (url != NULL && size >= 3) { memcpy(url, "%3E", 3); } return true;
        case '^': if (url != NULL && size >= 3) { memcpy(url, "%5E", 3); } return true; 
        case '`': if (url != NULL && size >= 3) { memcpy(url, "%60", 3); } return true;
        case '{': if (url != NULL && size >= 3) { memcpy(url, "%7B", 3); } return true;
        case '|': if (url != NULL && size >= 3) { memcpy(url, "%7C", 3); } return true;
        case '}': if (url != NULL && size >= 3) { memcpy(url, "%7D", 3); } return true;
        case '_': if (url != NULL && size >= 3) { memcpy(url, "%5F", 3); } return true;
        case '\'': if (url != NULL && size >= 3) { memcpy(url, "%27", 3); } return true;
        case '\\': if (url != NULL && size >= 3) { memcpy(url, "%5C", 3); } return true;
    }
    if (c & 0x80) {
        if (url != NULL && size >= 3) {
            char buffer[4];
            snprintf(buffer, 4, "%%%02X", (int)c & 0xFF);
            memcpy(url, buffer, 3);
        }
        return true;
    }
    if (url != NULL && size >= 1) { *url = c; }
    return false;
}

static char * _bx_percent_encoding (const char * value)
{
    size_t final_len = 0;
    for(int i = 0; value[i] != '\0'; i++) {
        if (_bx_encode_char_percent_encoding(NULL, 0, value[i])) {
            final_len += 3;
        } else {
            final_len++;
        }
        
    }
    char * o = calloc(final_len + 1, sizeof(*o));
    if (o == NULL) {
        return NULL;
    }
    char * tmp = o;
    size_t r_size = final_len;
    for(int i = 0; value[i] != '\0'; i++) {
        if (_bx_encode_char_percent_encoding(tmp, r_size, value[i])) {
            tmp += 3;
            r_size -= 3;
        } else {
            tmp++;
            r_size--;
        }
    }
    return o;
}

bool bx_net_request_add_param(BXNetRequest * request, const char * name, const char * value)
{
    assert(name != NULL);
    assert(value != NULL);
    assert(request != NULL);

    BXNetURLParams * param = NULL;

    char * e_name = NULL;
    char * e_value = NULL;

    param = calloc(1, sizeof(*param));
    if (param == NULL) { return false; }

    e_name = _bx_percent_encoding(name);
    if (e_name == NULL) {
        free(param);
        return false;
    }
    e_value = _bx_percent_encoding(value);
    if (e_value == NULL) {
        free(e_name);
        free(param);
    }
    param->name = e_name;
    param->value = e_value;
    param->next = NULL;
    if (request->params == NULL) {
        request->params = param;
    } else {
        BXNetURLParams * current = request->params;
        while(current->next != NULL) { current = current->next; }
        current->next = param;
    }
    return true;
}

void bx_net_request_free(BXNetRequest * request)
{
    if(request == NULL) { return; }
    if (request->decoded != NULL) { json_decref(request->decoded); }
    if (request->body != NULL) { json_decref(request->body); }
    if (request->path != NULL) { free(request->path); }
    if (request->params != NULL) { bx_net_request_params_free(request->params); }
    if (request->response != NULL) {
        if (request->response->data != NULL) { free(request->response->data); }
        free(request->response);
    }
    free(request);
}

#define DEFAULT_RATELIMIT_US   60000 /* 1000 requests per minute */

static void * _bx_net_loop_worker(void * l)
{
    BXNetRequestList * list = (BXNetRequestList *)l;
    BXNet * net;

    bx_mutex_lock(&list->mutex);
    net = list->net;
    bx_mutex_unlock(&list->mutex);

    int us_sleep = DEFAULT_RATELIMIT_US;
    uint64_t request_count = 0;
    const float max_request_share = 1;
    while(atomic_load(&list->run)) {
        BXNetRequest * request = NULL;

        request = bx_net_request_list_remove(list, false);
        if (request != NULL) {
            /* request is locked in search loop */
            // do request
            request->response = bx_fetch(net, request->path, request->params);
            /* readd in list so it can be processed */
            bx_net_request_list_add(list, request);
            atomic_store(&request->done, true);
            bx_mutex_lock(&net->mutex_limit);

            /*
             * Cumulative average of time slice. Trying to use as much bandwith
             * as allowed without using too much. As, with each request, we get
             * remaining requests for a given time, we can speed up or reduce
             * our request timing. So if it's already in heavy use, the request
             * rate will drop and go back up when bandwith is available.
             * It might not please people at bexio, as this is designed to run
             * 24/7, but we pay for this bandwith we use it.
             */
            float us_sleep_1 = (
                ((float)net->limits.reset_time * 1000000)
                / (max_request_share * (float)net->limits.remaining_request)
            );

            float average_us_sleep = (us_sleep_1
                + (us_sleep * (net->request_count - 1))
            ) / net->request_count;
            us_sleep = (int)average_us_sleep;
            request_count++;
            if (us_sleep <= 0 || us_sleep > DEFAULT_RATELIMIT_US * 100) { 
                us_sleep = DEFAULT_RATELIMIT_US;
            }
            bx_log_error("US_SLEEP %d, LIMIT %d, REMAINING %d, RESET %d\n", us_sleep, net->limits.max_request, net->limits.remaining_request, net->limits.reset_time);
            bx_mutex_unlock(&net->mutex_limit);
        }

        /* don't load server too much */
        usleep(us_sleep);
    }
    return 0;
}

pthread_t  bx_net_loop(BXNetRequestList * list)
{
    pthread_t thread;
    pthread_create(&thread, NULL, _bx_net_loop_worker, list);
    return thread;
}