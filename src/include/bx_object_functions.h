#ifndef BX_OBJECT_FUNCTIONS_H__
#define BX_OBJECT_FUNCTIONS_H__

#include "bxobjects/contact.h"
#include "bxobjects/invoice.h"
#include "bxobjects/position.h"
#include "bxobjects/taxes.h"

typedef struct s_BXObjectFunctions BXObjectFunctions;
struct s_BXObjectFunctions {
  enum e_BXObjectType type;
  void *(*decode_function)(void *);
  void (*free_function)(void *);
  void (*dump_function)(void *);
};

static const BXObjectFunctions FunctionHandlers[] = {
    {.type = BXTypeInvoice,
     .decode_function = bx_object_invoice_decode,
     .free_function = bx_object_invoice_free,
     .dump_function = bx_object_invoice_dump},
    {.type = BXTypeContact,
     .decode_function = bx_object_contact_decode,
     .free_function = bx_object_contact_free,
     .dump_function = bx_object_contact_dump},
    {.type = BXTypeInvoiceTax,
     .decode_function = bx_object_tax_decode,
     .free_function = bx_object_tax_free,
     .dump_function = NULL},
    {.type = BXTypeInvoicePositionCE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeInvoicePositionAE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeInvoicePositionTE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeInvoicePositionSE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeInvoicePositionPE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeInvoicePositionDE,
     .decode_function = bx_object_position_decode,
     .free_function = bx_object_position_free,
     .dump_function = bx_object_position_dump},
    {.type = BXTypeNone,
     .decode_function = NULL,
     .free_function = NULL,
     .dump_function = NULL}};

#endif /* BX_OBJECT_FUNCTIONS_H__ */
