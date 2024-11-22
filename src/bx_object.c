#include <bx_object.h>
#include <bx_html_entities.h>
#include <bx_object_value.h>

#include <jansson.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

BXBool bx_object_get_json_bool(json_t * object, const char * key, XXH3_state_t * state)
{
    json_t * value = NULL;
    BXBool retval = { .type = BX_OBJECT_TYPE_BOOL, .isset = false, .value = false };

    value = json_object_get(object, key);
    if (value == NULL) {
        return retval;
    }

    retval.isset = true;
    if (json_is_string(value)) {
        if (strcasecmp("true", json_string_value(value)) == 0) {
            retval.value = true;
        } else if (strcasecmp("yes", json_string_value(value)) == 0) {
            retval.value = true;
        } else if (strcasecmp("on", json_string_value(value)) == 0) {
            retval.value = true;
        } else {
            retval.value = false;
        }
    } else if (json_is_boolean(value)) {
        if (json_is_false(value)) { retval.value = false; }
        else { retval.value = true; }
    } else if (json_is_integer(value)) {
        if (json_integer_value(value)) { retval.value = true; }
        else { retval.value = false; }
    }
    unsigned char hv = retval.value ? 0xFF : 0x00;
    XXH3_64bits_update(state, &hv, sizeof(hv));
    return retval;
}

BXInteger bx_object_get_json_int(json_t * object, const char * key, XXH3_state_t * state)
{
    json_t * value = NULL;
    BXInteger i = { .type = BX_OBJECT_TYPE_INTEGER, .isset = false, .value = 0 };
    value = json_object_get(object, key);

    if (value == NULL) { return i; }
    
    i.isset = true;
    if (json_is_string(value)) {
        i.value = strtol(json_string_value(value), NULL, 10);
    } else if (json_is_integer(value)) {
        i.value = json_integer_value(value);
    }
    XXH3_64bits_update(state, &i.value, sizeof(i.value));
    
    return i;
}

BXFloat bx_object_get_json_double(json_t * object, const char * key, XXH3_state_t * state)
{
    json_t * value = NULL;
    BXFloat d = { .type = BX_OBJECT_TYPE_FLOAT, .isset = false, .value = 0};

    value = json_object_get(object, key);
    if (value == NULL) { return d; }
    
    d.isset = true;
    if (json_is_string(value)) {
        d.value = strtod(json_string_value(value), NULL);
    } else if (json_is_real(value)) {
        d.value = json_real_value(value);
    }
    XXH3_64bits_update(state, &d.value, sizeof(d.value));
    return d;
}

BXString bx_object_get_json_string(json_t * object, const char * key, XXH3_state_t * state)
{
    json_t * value = NULL;
    BXString str = { .type = BX_OBJECT_TYPE_STRING, .isset = false, .value = NULL, .value_len = 0 };
    value = json_object_get(object, key);
    if (value == NULL) { return str; }
    
    if (json_is_string(value)) {
        str.value_len = json_string_length(value);
        str.value = calloc(str.value_len + 1, sizeof(*str.value));
        if (str.value == NULL) { return str; }
        memcpy(str.value, json_string_value(value), str.value_len);
    } else if (json_is_integer(value)) {
        str.value_len = snprintf(NULL, 0, "%lld", json_integer_value(value)) - 1;
        str.value = calloc(str.value_len + 1, sizeof(*str.value));
        if (str.value == NULL) { return str; }
        snprintf(str.value, str.value_len + 1, "%lld", json_integer_value(value));
    } else if (json_is_real(value)) {
        str.value_len = snprintf(NULL, 0, "%e", json_real_value(value)) - 1;
        str.value = calloc(str.value_len + 1, sizeof(*str.value));
        if (str.value == NULL) { return str; }
        snprintf(str.value, str.value_len + 1, "%e", json_real_value(value));
    }

    if (str.value != NULL) {    
        str.isset = true;
        bx_html_entities_replaces(&str);
        XXH3_64bits_update(state, str.value, str.value_len);
    }

    return str;
}

inline static void bx_object_free_string(BXString string)
{
    if (string.value != NULL) {
        free(string.value);
    }
    string.value_len = 0;
}

void bx_object_free_value(void * value)
{
    if (value == NULL) { return; }
    if (*(uint8_t *)value == BX_OBJECT_TYPE_STRING) {
        bx_object_free_string(*(BXString *)value);
    }
}

