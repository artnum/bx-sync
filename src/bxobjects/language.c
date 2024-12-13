#include "bx_database.h"
#include "bxill.h"
#include <bx_net.h>
#include <bx_object.h>
#include <bx_object_value.h>
#include <bxobjects/language.h>
#include <bx_utils.h>
#include <jansson.h>

#define GET_LANGUAGE_PATH   "2.0/language"

static BXObjectLanguage * decode_object(json_t * object, BXObjectLanguage * language)
{
    bool is_langage_freeable = false;
    if (language == NULL) {
        is_langage_freeable = true;
        language = calloc(1, sizeof(*language));
        if (language == NULL) {
            return NULL;
        }
    }
    XXH3_state_t * hash_state = XXH3_createState();
    if (hash_state == NULL) {
        if (is_langage_freeable) { free(language); }
        return NULL;
    }
    XXH3_64bits_reset(hash_state);
    language->type = BXTypeLanguage;

    language->id = bx_object_get_json_uint(object, "id", hash_state);
    language->date_format_id = bx_object_get_json_uint(object, "date_format_id", hash_state);

    language->name = bx_object_get_json_string(object, "name", hash_state);
    language->decimal_point = bx_object_get_json_string(object, "decimal_point", hash_state);
    language->thousands_separator = bx_object_get_json_string(object, "thousands_separator", hash_state);
    language->iso_639_1 = bx_object_get_json_string(object, "iso_639_1", hash_state);
    language->date_format = bx_object_get_json_string(object, "date_format", hash_state);
    
    language->checksum = XXH3_64bits_digest(hash_state);
    XXH3_freeState(hash_state);

    return language;
}

static void free_content(BXObjectLanguage * language)
{
    bx_object_free_value(&language->name);
    bx_object_free_value(&language->date_format);
    bx_object_free_value(&language->thousands_separator);
    bx_object_free_value(&language->decimal_point);
    bx_object_free_value(&language->iso_639_1);
}

void bx_language_free(BXObjectLanguage * language)
{
    free_content(language);
    free(language);
}

ObjectState bx_language_check_database(bXill * app, BXObjectLanguage * language)
{
    BXDatabaseQuery * query = bx_database_new_query(
        app->mysql,
        "SELECT _checksum FROM language WHERE id = :id"
    );
    if (query == NULL) {
        return Error;
    }
    bx_database_add_param_uint64(query, ":id", &language->id.value);
    bx_database_execute(query);
    bx_database_results(query);

    if (query->results == NULL || query->results->column_count == 0) {
        bx_database_free_query(query);
        return NeedCreate;
    }

    if (query->results->columns[0].i_value != language->checksum) {
        bx_database_free_query(query);
        return NeedUpdate;
    }
    
    bx_database_free_query(query);
    return NeedNothing;
}

#define QUERY_UPDATE    "UPDATE language SET name = :name, decimal_point =: decimal_point," \
    "thousands_separator = :thousands_separator, date_format_id = :date_format_id," \
    "date_format = :date_format, iso_639_1 = :iso_639_1," \
    "_checksum = :_checksum, _last_updated = :_last_updated" \
    " WHERE id = :id"
#define QUERY_INSERT    "INSERT INTO language (id, name, decimal_point, thousands_separator,"\
    "date_format_id, date_format, iso_639_1, _checksum, _last_updated)" \
    " VALUES (:id, :name, :decimal_point, :thousands_separator, :date_format_id," \
    ":date_format, :iso_639_1," \
    ":_checksum, :_last_updated)"

static bool bind_params(BXDatabaseQuery * query, BXObjectLanguage * language)
{
    uint64_t now = 0;
    now = time(NULL);
    if (
        !bx_database_add_param_uint64(query, ":id", &language->id.value)
        || !bx_database_add_param_uint64(query, ":date_format_id", &language->date_format_id.value)
        || !bx_database_add_param_char(query, ":name", language->name.value, language->name.value_len)
        || !bx_database_add_param_char(query, ":decimal_point", language->decimal_point.value, language->decimal_point.value_len)
        || !bx_database_add_param_char(query, ":date_format", language->date_format.value, language->date_format.value_len)
        || !bx_database_add_param_char(query, ":thousands_separator", language->thousands_separator.value, language->thousands_separator.value_len)
        || !bx_database_add_param_char(query, ":iso_639_1", language->iso_639_1.value, language->iso_639_1.value_len)
        || !bx_database_add_param_uint64(query, ":_checksum", &language->checksum)
        || !bx_database_add_param_uint64(query, ":_last_updated", &now)
    ) {
        return false;
    }

    return true;
}

static bool execute_request(bXill * app, BXObjectLanguage * language, const char * request)
{
    BXDatabaseQuery * query = NULL;

    query = bx_database_new_query(app->mysql, request);
    if (query == NULL) {
        return false;
    }
    bool success = true;
    if (
        !bind_params(query, language)
        || !bx_database_execute(query)
        || !bx_database_results(query)
    ) {
        success = false;
    }
    bx_database_free_query(query);
    return success;
}

bool bx_language_update_db(bXill * app, BXObjectLanguage * language)
{
    return execute_request(app, language, QUERY_UPDATE);
}

bool bx_language_insert_db(bXill * app, BXObjectLanguage * language)
{
    return execute_request(app, language, QUERY_INSERT);
}

bool bx_language_delete_db(bXill * app, BXObjectLanguage * language)
{
    return false;
}

bool bx_language_load (bXill * app)
{
    bx_log_debug("BX Language load");
    BXNetRequest * request = bx_do_request(
        app->queue,
        NULL,
        GET_LANGUAGE_PATH
    );

    if (request == NULL
        || request->response == NULL
        || request->response->http_code != 200    
    ) {
        return false;
    }

    json_t * jroot = request->decoded;
    size_t array_length = json_array_size(jroot);
    for (size_t i = 0; i < array_length; i++) {
        BXObjectLanguage language;
        if (decode_object(json_array_get(jroot, i), &language) != NULL) {
            switch(bx_language_check_database(app, &language)) {
                default:
                case NeedNothing: 
                    break;
                case Error:
                    bx_log_error("Error checking for language %d", language.id.value);
                    break;
                case NeedCreate:
                    if (!bx_language_insert_db(app, &language)) {
                        bx_log_error("Failed insert language %d", language.id.value);
                    }
                    break;
                case NeedUpdate:
                    if (!bx_language_update_db(app, &language)) {
                        bx_log_error("Failed insert language %d", language.id.value);
                    }
                    break;
            }
            free_content(&language);
        }
    }
    return true;
}