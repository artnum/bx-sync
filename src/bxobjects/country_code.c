#include <bx_mutex.h>
#include <bxobjects/country_code.h>
#include <bx_database.h>
#include <bx_object.h>
#include <bx_object_value.h>
#include <bx_utils.h>
#include <curses.h>
#include <jansson.h>

Country * COUNTRY_LIST = NULL;
BXMutex MTX_COUNTRY_LIST;

#define GET_CONTACT_GROUP_PATH    "2.0/country"
bool bx_country_code_load(bXill * app)
{
    bx_log_debug("BX Country Code load");
    BXNetRequest * request = bx_do_request(
        app->queue,
        NULL,
        GET_CONTACT_GROUP_PATH
    );
    if (request == NULL
        || request->response == NULL
        || request->response->http_code != 200
    ) {
        if (request == NULL || request->response == NULL) { return false; }
        if (request->response->http_code == 404) {
            return false;
        }
        return false;
    }
    
    json_t * jroot = request->decoded;
    size_t array_length = json_array_size(jroot);
    bx_mutex_lock(&MTX_COUNTRY_LIST);
    COUNTRY_LIST = calloc(array_length + 1, sizeof(*COUNTRY_LIST));
    if (COUNTRY_LIST == NULL ) {
        bx_mutex_unlock(&MTX_COUNTRY_LIST);
        bx_net_request_free(request);
        return false;
    }
    COUNTRY_LIST[array_length].bx_id = -1; /* last one at -1 so we have a stop point*/

    for (int i = 0; i < array_length; i++) {
        json_t * country = json_array_get(jroot, i);
        BXInteger id = bx_object_get_json_int(country, "id", NULL);
        /* Official documentation says iso3166_alpha2 but in reality it's
         * iso_3166_alpha2 when reading. When creating it's iso3166_alpha2.
         * I wrote a bug report in 2023, they said they will fix it, but as 
         * december 2024, it is not fixed even if the updated documentation at 
         * least 10 times since my report.
         * Bexio shouldn't be used for anything except trying to crash your 
         * company because this application will blow up at one point.
         */
        BXString code = bx_object_get_json_string(country, "iso_3166_alpha2", NULL);
        COUNTRY_LIST[i].bx_id = id.value;
        memcpy(COUNTRY_LIST[i].code, code.value, 2);
        bx_object_free_value(&code);
    }
    bx_mutex_unlock(&MTX_COUNTRY_LIST);

    return true;
}

const char * bx_country_list_get_code(int id) 
{
    bx_mutex_lock(&MTX_COUNTRY_LIST);
    for(int i = 0; COUNTRY_LIST[i].bx_id != -1; i++) {
        if (COUNTRY_LIST[i].bx_id == id) {
            bx_mutex_unlock(&MTX_COUNTRY_LIST);
            return COUNTRY_LIST[i].code;
        }
    }
    bx_mutex_unlock(&MTX_COUNTRY_LIST);
    return NULL;
}

void bx_country_list_free(void)
{
    bx_mutex_lock(&MTX_COUNTRY_LIST);
    free(COUNTRY_LIST);
    COUNTRY_LIST = NULL;
    bx_mutex_unlock(&MTX_COUNTRY_LIST);
}

void bx_country_list_dump(void)
{
    bx_mutex_lock(&MTX_COUNTRY_LIST);
    for(int i = 0; COUNTRY_LIST[i].bx_id != -1; i++) {
        bx_log_debug("COUNTRY CODE %s", COUNTRY_LIST[i].code);
    }
    bx_mutex_unlock(&MTX_COUNTRY_LIST);
}