#include "bxobjects/position.h"
#include <bxobjects/invoice.h>
#include <bx_object.h>
#include <bx_utils.h>
#include <assert.h>

void bx_object_invoice_free(void * data)
{
    BXObjectInvoice * invoice = (BXObjectInvoice *)data;
    if (invoice == NULL) { return; }
    
    bx_object_free_value(&invoice->remote_document_nr);
    bx_object_free_value(&invoice->remote_is_valid_from);
    bx_object_free_value(&invoice->remote_is_valid_to);
    bx_object_free_value(&invoice->remote_contact_address);
    bx_object_free_value(&invoice->remote_template_slug);
    bx_object_free_value(&invoice->remote_updated_at);
    bx_object_free_value(&invoice->remote_reference);
    bx_object_free_value(&invoice->remote_api_reference);
    bx_object_free_value(&invoice->remote_viewed_by_client_at);
    if (invoice->remote_taxes != NULL) { 
        for(int i = 0; i < invoice->bx_object_taxes_count; i++) {
            bx_object_tax_free(invoice->remote_taxes[i]);
        }
        free(invoice->remote_taxes); 
    }
    if (invoice->remote_positions != NULL) {
        for (int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
            bx_object_position_free(invoice->remote_positions[i]);
        }
        free(invoice->remote_positions);
    }
    free(invoice);
}

void bx_object_invoice_dump(void * data)
{
    assert(data != NULL);

    BXObjectInvoice * invoice = (BXObjectInvoice *)data;

    _bx_dump_print_title("### DUMP '%s' ID:%lx CS:%lx ###", invoice->remote_document_nr.value, invoice->id, invoice->checksum);
    _bx_dump_any("id", &invoice->remote_id, 1);
    _bx_dump_any("user_id", &invoice->remote_user_id, 1);
    _bx_dump_any("contact_id", &invoice->remote_contact_id, 1);
    _bx_dump_any("contact_subid", &invoice->remote_contact_subid, 1);
    _bx_dump_any("project_id", &invoice->remote_project_id, 1);
    _bx_dump_any("bank_account_id", &invoice->remote_bank_account_id, 1);
    _bx_dump_any("currency_id", &invoice->remote_currency_id, 1);
    _bx_dump_any("payment_type_id", &invoice->remote_payment_type_id, 1);
    _bx_dump_any("tva_id", &invoice->remote_tva_type, 1);
    _bx_dump_any("kb_item_status", &invoice->remote_kb_item_status, 1);
    _bx_dump_any("esr_id", &invoice->remote_esr_id, 1);
    _bx_dump_any("qr_invoice_id", &invoice->remote_qr_invoice_id, 1);

    _bx_dump_any("total_gross", &invoice->remote_total_gross, 1);
    _bx_dump_any("total_net", &invoice->remote_total_net, 1);
    _bx_dump_any("total_taxes", &invoice->remote_total_taxes, 1);
    _bx_dump_any("total_reveived_payments", &invoice->remote_total_received_payments, 1);
    _bx_dump_any("total_credit_vouchers", &invoice->remote_total_credit_vouchers, 1);
    _bx_dump_any("total_remaining_payments", &invoice->remote_total_remaining_payments, 1);
    _bx_dump_any("total", &invoice->remote_total, 1);
    _bx_dump_any("total_rounding_difference", &invoice->remote_total_rounding_difference, 1);

    _bx_dump_any("is_valid_from", &invoice->remote_is_valid_from, 1);
    _bx_dump_any("is_valid_to", &invoice->remote_is_valid_to, 1);
    _bx_dump_any("contact_address", &invoice->remote_contact_address, 1);
    _bx_dump_any("template_slug", &invoice->remote_template_slug, 1);
    _bx_dump_any("updated_at", &invoice->remote_updated_at, 1);
    _bx_dump_any("reference", &invoice->remote_reference, 1);
    _bx_dump_any("api_reference", &invoice->remote_api_reference, 1);
    _bx_dump_any("viewed_by_client_at", &invoice->remote_viewed_by_client_at, 1);

    _bx_dump_print_subtitle("Taxes");
    for (int i = 0; i < invoice->bx_object_taxes_count; i++) {
        bx_object_tax_dump(invoice->remote_taxes[i]);
    }
    
    _bx_dump_print_subtitle("Positions");
    for (int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
        bx_object_position_dump(invoice->remote_positions[i]);
    }
}

void * bx_object_invoice_decode(void * object)
{
    json_t * jroot = (json_t *)object;
    BXObjectInvoice * invoice = NULL;
    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    invoice = calloc(1, sizeof(*invoice));
    if (invoice == NULL) {
        return NULL;
    }
    invoice->type = BXTypeInvoice;
    bx_utils_gen_id(&invoice->id);

    /* integer */
    invoice->remote_id =                        bx_object_get_json_uint(jroot, "id", hashState);
    invoice->remote_user_id =                   bx_object_get_json_uint(jroot, "user_id", hashState);
    invoice->remote_contact_id =                bx_object_get_json_uint(jroot, "contact_id", hashState);
    invoice->remote_contact_subid =             bx_object_get_json_uint(jroot, "contact_sub_id", hashState);
    invoice->remote_project_id =                bx_object_get_json_uint(jroot, "user_id", hashState);
    invoice->remote_bank_account_id =           bx_object_get_json_uint(jroot, "bank_account_id", hashState);
    invoice->remote_currency_id =               bx_object_get_json_uint(jroot, "currency_id", hashState);
    invoice->remote_payment_type_id =           bx_object_get_json_uint(jroot, "payment_type_id", hashState);
    invoice->remote_tva_type =                  bx_object_get_json_uint(jroot, "mwst_type", hashState);
    invoice->remote_kb_item_status =            bx_object_get_json_uint(jroot, "kb_item_status", hashState);
    invoice->remote_esr_id =                    bx_object_get_json_uint(jroot, "esr_id", hashState);
    invoice->remote_qr_invoice_id =             bx_object_get_json_uint(jroot, "qr_invoice_id", hashState);

    /* double */
    invoice->remote_total_gross =               bx_object_get_json_double(jroot, "total_gross", hashState);
    invoice->remote_total_net =                 bx_object_get_json_double(jroot, "total_net", hashState);
    invoice->remote_total_taxes =               bx_object_get_json_double(jroot, "total_taxes", hashState);
    invoice->remote_total_received_payments =   bx_object_get_json_double(jroot, "total_received_payments", hashState);
    invoice->remote_total_credit_vouchers =     bx_object_get_json_double(jroot, "total_credit_vouchers", hashState);
    invoice->remote_total_remaining_payments =  bx_object_get_json_double(jroot, "total_remaining_payments", hashState);
    invoice->remote_total =                     bx_object_get_json_double(jroot, "total", hashState);
    invoice->remote_total_rounding_difference = bx_object_get_json_double(jroot, "total_rounding_difference", hashState);

    /* string */
    invoice->remote_document_nr =               bx_object_get_json_string(jroot, "document_nr", hashState);
    invoice->remote_is_valid_from =             bx_object_get_json_string(jroot, "is_valid_from", hashState);
    invoice->remote_is_valid_to =               bx_object_get_json_string(jroot, "is_valid_to", hashState);
    invoice->remote_contact_address =           bx_object_get_json_string(jroot,"contact_address", hashState);
    invoice->remote_template_slug =             bx_object_get_json_string(jroot, "template_slug", hashState);
    invoice->remote_updated_at =                bx_object_get_json_string(jroot, "updated_at", hashState);
    invoice->remote_reference =                 bx_object_get_json_string(jroot, "reference", hashState);
    invoice->remote_api_reference =             bx_object_get_json_string(jroot, "api_reference", hashState);
    invoice->remote_viewed_by_client_at =       bx_object_get_json_string(jroot, "viewed_by_client_at", hashState);

    invoice->remote_tva_is_net =                bx_object_get_json_bool(jroot, "tva_is_net", hashState);

    json_t * value = json_object_get(jroot, "taxs");
    if (value != NULL && json_is_array(value)) {
        invoice->bx_object_taxes_count = json_array_size(value);
        invoice->remote_taxes = calloc(invoice->bx_object_taxes_count, sizeof(invoice->remote_taxes));
        if (invoice->remote_taxes != NULL) {
            for (int i = 0; i < invoice->bx_object_taxes_count; i++) {
                invoice->remote_taxes[i] = bx_object_tax_decode(json_array_get(value, i));
            }
        }
    }
    value = NULL;

    value = json_object_get(jroot, "positions");
    if (value != NULL && json_is_array(value)) {
        invoice->bx_object_remote_positions_count = json_array_size(value);
        invoice->remote_positions = calloc(invoice->bx_object_remote_positions_count, sizeof(*invoice->remote_positions));
        if (invoice->remote_positions != NULL) {
            for(int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
                invoice->remote_positions[i] = bx_object_position_decode(json_array_get(value, i));
            }
        }
    }
    invoice->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);
    return invoice;
}
