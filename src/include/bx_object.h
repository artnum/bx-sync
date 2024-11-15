#ifndef BX_OBJECT_H__
#define BX_OBJECT_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <jansson.h>
#include <xxhash.h>

#include <bx_object_value.h>
#include <bxobjects/invoice.h>
#include <bxobjects/contact.h>

typedef struct s_BXObjectRelations BXObjectRelations;
struct s_BXObjectRelations {
    const char * name;
};

typedef struct s_BXObjectFunctions BXObjectFunctions;
struct s_BXObjectFunctions {
    const char * name;
    void * (*decode_function)(void *);
    void (*free_function)(void *);
    void (*dump_function)(void *);
};

BXBool bx_object_get_json_bool(json_t * object, const char * key, XXH3_state_t * state);
BXInteger bx_object_get_json_int(json_t * object, const char * key, XXH3_state_t * state);
BXFloat bx_object_get_json_double(json_t * object, const char * key, XXH3_state_t * state);
BXString bx_object_get_json_string(json_t * object, const char * key, XXH3_state_t * state);
void bx_object_free_value(void * value);

static const BXObjectFunctions FunctionHandlers[] = {
    {
        .name = "invoice",
        .decode_function = bx_object_invoice_decode,
        .free_function = bx_object_invoice_free,
        .dump_function = bx_object_invoice_dump
    },
    {
        .name = "contact",
        .decode_function = bx_object_contact_decode,
        .free_function = bx_object_contact_free,
        .dump_function = bx_object_contact_dump
    },
    {.name = NULL, .decode_function = NULL, .free_function = NULL, .dump_function = NULL }
};

#endif /* BX_OBJECT_H__ */