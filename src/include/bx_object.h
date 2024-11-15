#ifndef BX_OBJECT_H__
#define BX_OBJECT_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <jansson.h>
#include <xxhash.h>

#include <bx_object_value.h>

enum e_BXObjectType {
    BXTypeNone = 0,
    BXTypeInvoice = 1,
    BXTypeContact,
    BXTypeInvoiceTax,
    BXTypeInvoicePositionCE,
    BXTypeInvoicePositionAE,
    BXTypeInvoicePositionDE,
    BXTypeInvoicePositionSE,
    BXTypeInvoicePositionPE,
    BXTypeInvoicePositionTE
};

BXBool bx_object_get_json_bool(json_t * object, const char * key, XXH3_state_t * state);
BXInteger bx_object_get_json_int(json_t * object, const char * key, XXH3_state_t * state);
BXFloat bx_object_get_json_double(json_t * object, const char * key, XXH3_state_t * state);
BXString bx_object_get_json_string(json_t * object, const char * key, XXH3_state_t * state);
void bx_object_free_value(void * value);

#endif /* BX_OBJECT_H__ */