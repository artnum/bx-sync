#ifndef BX_OBJECT_H__
#define BX_OBJECT_H__

#include <jansson.h>
#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xxhash.h>

#include "bx_object_value.h"

enum e_BXObjectType {
  BXTypeNone = 0,
  BXTypeInvoice = 1,
  BXTypeContact,
  BXTypeContactGroup,
  BXTypeContactSector,
  BXTypeInvoiceTax,
  BXTypeInvoicePositionCE,
  BXTypeInvoicePositionAE,
  BXTypeInvoicePositionDE,
  BXTypeInvoicePositionSE,
  BXTypeInvoicePositionPE,
  BXTypeInvoicePositionTE,
  BXTypeUser,
  BXTypeLanguage,
  BXTypeProject
};

BXBool bx_object_get_json_bool(json_t *object, const char *key,
                               XXH3_state_t *state);
BXInteger bx_object_get_json_int(json_t *object, const char *key,
                                 XXH3_state_t *state);
BXUInteger bx_object_get_json_uint(json_t *object, const char *key,
                                   XXH3_state_t *state);
BXFloat bx_object_get_json_double(json_t *object, const char *key,
                                  XXH3_state_t *state);
BXString bx_object_get_json_string(json_t *object, const char *key,
                                   XXH3_state_t *state);
BXUuid bx_object_get_json_uuid(json_t *object, const char *kex,
                               XXH3_state_t *state);
void bx_object_free_value(void *value);

#define bxo_getstr(st, member)                                                 \
  ({ st->member = bx_object_get_json_string(object, #member, hash_state); })
#define bxo_getbool(st, member)                                                \
  ({ st->member = bx_object_get_json_bool(object, #member, hash_state); })
#define bxo_getint(st, member)                                                 \
  ({ st->member = bx_object_get_json_int(object, #member, hash_state); })
#define bxo_getuint(st, member)                                                \
  ({ st->member = bx_object_get_json_uint(object, #member, hash_state); })
#define bxo_getdouble(st, member)                                              \
  ({ st->member = bx_object_get_json_double(object, #member, hash_state); })
#define bxo_getuuid(st, member)                                                \
  ({ st->member = bx_object_get_json_uuid(object, #member, hash_state); })
#define bxo_checksum(st)                                                       \
  ({                                                                           \
    st->checksum = XXH3_64bits_digest(hash_state);                             \
    XXH3_freeState(hash_state);                                                \
  })

#endif /* BX_OBJECT_H__ */
