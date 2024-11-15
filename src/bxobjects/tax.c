#include "bx_object_value.h"
#include "bx_utils.h"
#include <bxobjects/tax.h>
#include <bx_object.h>
#include <stdlib.h>
#include <assert.h>

void * bx_object_tax_decode(void * jroot) 
{
    assert(jroot != NULL);
    BXObjectTax * tax = NULL;
    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    tax = calloc(1, sizeof(*tax));
    if (tax == NULL) {
        return NULL;
    }
    tax->type = BXTypeInvoiceTax;
    bx_utils_gen_id(&tax->id);

    tax->remote_percentage = bx_object_get_json_double(jroot, "percentage", hashState);
    tax->remote_value = bx_object_get_json_double(jroot, "value", hashState);

    tax->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);

    return tax;
}

void bx_object_tax_dump(void * data)
{
    BXObjectTax * tax = (BXObjectTax *)data;
    if (tax == NULL) {return; }
    _bx_dump_print_title("### DUMP TAX ID:%lx CS:%lx ###", tax->id, tax->checksum);
    _bx_dump_any("percentage", &tax->remote_percentage, 2);
    _bx_dump_any("value", &tax->remote_value, 2);
}

void bx_object_tax_free(void * data)
{
    if (data != NULL) { free(data); }
}