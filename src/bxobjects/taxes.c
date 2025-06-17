#include "../include/bxobjects/taxes.h"
#include "../include/bx_object.h"
#include <assert.h>
#include <stdlib.h>

void *bx_object_tax_decode(void *jroot) {
  assert(jroot != NULL);
  BXObjectTax *tax = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hashState);
  tax = calloc(1, sizeof(*tax));
  if (tax == NULL) {
    return NULL;
  }
  tax->type = BXTypeInvoiceTax;
  tax->id = bx_object_get_json_uint(jroot, "value", hashState);
  tax->uuid = bx_object_get_json_uuid(jroot, "uuid", hashState);
  tax->name = bx_object_get_json_string(jroot, "name", hashState);
  tax->digit = bx_object_get_json_int(jroot, "digit", hashState);
  tax->taxtype = bx_object_get_json_string(jroot, "type", hashState);
  tax->account_id = bx_object_get_json_uint(jroot, "account_id", hashState);
  tax->tax_settlement_type =
      bx_object_get_json_string(jroot, "tax_settlement_type", hashState);
  tax->value = bx_object_get_json_double(jroot, "value", hashState);
  tax->net_tax_value =
      bx_object_get_json_string(jroot, "net_tax_value", hashState);
  tax->start_year = bx_object_get_json_int(jroot, "start_year", hashState);
  tax->end_year = bx_object_get_json_int(jroot, "end_year", hashState);
  tax->start_month = bx_object_get_json_int(jroot, "start_month", hashState);
  tax->end_month = bx_object_get_json_int(jroot, "end_month", hashState);
  tax->display_name =
      bx_object_get_json_string(jroot, "display_name", hashState);
  tax->is_active = bx_object_get_json_bool(jroot, "is_active", hashState);

  tax->checksum = XXH3_64bits_digest(hashState);
  XXH3_freeState(hashState);

  return tax;
}

void bx_object_tax_free(void *data) {
  BXObjectTax *tax = (BXObjectTax *)data;

  if (tax == NULL) {
    return;
  }
  bx_object_free_value(&tax->id);
  bx_object_free_value(&tax->uuid);
  bx_object_free_value(&tax->name);
  bx_object_free_value(&tax->digit);
  bx_object_free_value(&tax->taxtype);
  bx_object_free_value(&tax->account_id);
  bx_object_free_value(&tax->tax_settlement_type);
  bx_object_free_value(&tax->value);
  bx_object_free_value(&tax->net_tax_value);
  bx_object_free_value(&tax->start_year);
  bx_object_free_value(&tax->start_month);
  bx_object_free_value(&tax->end_year);
  bx_object_free_value(&tax->end_month);
  bx_object_free_value(&tax->display_name);
  bx_object_free_value(&tax->is_active);
  free(tax);
}
