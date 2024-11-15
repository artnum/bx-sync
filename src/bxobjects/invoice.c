#include <bxobjects/invoice.h>
#include <bx_object.h>
#include <bx_utils.h>
#include <assert.h>

static const char * type = "invoice";
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
    if (invoice->remote_taxes != NULL) { free(invoice->remote_taxes); }
    if (invoice->remote_positions != NULL) {
        for (int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
            bx_object_free_value(&invoice->remote_positions[i].remote_unit_name);
            bx_object_free_value(&invoice->remote_positions[i].remote_text);
            bx_object_free_value(&invoice->remote_positions[i].remote_type);
        }
        free(invoice->remote_positions);
    }
    free(invoice);
}

static void * get(BXObjectInvoice * object, const char * key)
{
    if (object == NULL) { return NULL; }
    if (key == NULL) { return NULL; }
    if (strcasecmp(key, "document_nr") == 0) { return &object->remote_document_nr; }
    if (strcasecmp(key, "user_id") == 0) { return &object->remote_user_id; }
    if (strcasecmp(key, "contact_id") == 0) { return &object->remote_contact_id; }
    if (strcasecmp(key, "contact_subid") == 0) { return &object->remote_contact_subid; }
    if (strcasecmp(key, "project_id") == 0) { return &object->remote_project_id; }
    if (strcasecmp(key, "bank_account_id") == 0) { return &object->remote_bank_account_id; }
    if (strcasecmp(key, "currency_id") == 0) { return &object->remote_currency_id; }
    if (strcasecmp(key, "payment_type_id") == 0) { return &object->remote_payment_type_id; }
    if (strcasecmp(key, "tva_id") == 0) { return &object->remote_tva_type; }
    if (strcasecmp(key, "kb_item_status") == 0) { return &object->remote_kb_item_status; }
    if (strcasecmp(key, "esr_id") == 0) { return &object->remote_esr_id; }
    if (strcasecmp(key, "qr_invoice_id") == 0) { return &object->remote_qr_invoice_id; }
    if (strcasecmp(key, "total_gross") == 0) { return &object->remote_total_gross; }
    if (strcasecmp(key, "total_net") == 0) { return &object->remote_total_net; }
    if (strcasecmp(key, "total_taxes") == 0) { return &object->remote_total_taxes; }
    if (strcasecmp(key, "total_reveived_payments") == 0) { return &object->remote_total_received_payments; }
    if (strcasecmp(key, "total_credit_vouchers") == 0) { return &object->remote_total_credit_vouchers; }
    if (strcasecmp(key, "total_remaining_payments") == 0) { return &object->remote_total_remaining_payments; }
    if (strcasecmp(key, "total") == 0) { return &object->remote_total; }
    if (strcasecmp(key, "total_rounding_difference") == 0) { return &object->remote_total_rounding_difference; }
    if (strcasecmp(key, "is_valid_from") == 0) { return &object->remote_is_valid_from; }
    if (strcasecmp(key, "is_valid_to") == 0) { return &object->remote_is_valid_to; }
    if (strcasecmp(key, "contact_address") == 0) { return &object->remote_contact_address; }
    if (strcasecmp(key, "template_slug") == 0) { return &object->remote_template_slug; }
    if (strcasecmp(key, "updated_at") == 0) { return &object->remote_updated_at; }
    if (strcasecmp(key, "reference") == 0) { return &object->remote_reference; }
    if (strcasecmp(key, "api_reference") == 0) { return &object->remote_api_reference; }
    if (strcasecmp(key, "viewed_by_client_at") == 0) { return &object->remote_viewed_by_client_at; }
    return NULL;
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
        _bx_dump_any("value", &invoice->remote_taxes[i].remote_value, 2);
        _bx_dump_any("percentage", &invoice->remote_taxes[i].remote_percentage, 2);
    }
    
    _bx_dump_print_subtitle("Positions");
    for (int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
        _bx_dump_any("id", &invoice->remote_positions[i].remote_id, 2);
        _bx_dump_any("unit_id", &invoice->remote_positions[i].remote_unit_id, 2);
        _bx_dump_any("account_id", &invoice->remote_positions[i].remote_account_id, 2);
        _bx_dump_any("tax_id", &invoice->remote_positions[i].remote_tax_id, 2);
        _bx_dump_any("pos", &invoice->remote_positions[i].remote_pos, 2);
        _bx_dump_any("internal_pos", &invoice->remote_positions[i].remote_internal_pos, 2);
        _bx_dump_any("parent_id", &invoice->remote_positions[i].remote_parent_id, 2);

        _bx_dump_any("amount", &invoice->remote_positions[i].remote_amount, 2);
        _bx_dump_any("tax_value", &invoice->remote_positions[i].remote_tax_value, 2);
        _bx_dump_any("unit_price", &invoice->remote_positions[i].remote_unit_price, 2);
        _bx_dump_any("discount_in_percent", &invoice->remote_positions[i].remote_discount_in_percent, 2);
        _bx_dump_any("position_total", &invoice->remote_positions[i].remote_position_total, 2);

        _bx_dump_any("unit_name", &invoice->remote_positions[i].remote_unit_name, 2);
        _bx_dump_any("text", &invoice->remote_positions[i].remote_text, 2);
        _bx_dump_any("type", &invoice->remote_positions[i].remote_type, 2);
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
    invoice->type = type;
    bx_utils_gen_id(&invoice->id);

    /* integer */
    invoice->remote_id =                        bx_object_get_json_int(object, "id", hashState);
    invoice->remote_user_id =                   bx_object_get_json_int(object, "user_id", hashState);
    invoice->remote_contact_id =                bx_object_get_json_int(object, "contact_id", hashState);
    invoice->remote_contact_subid =             bx_object_get_json_int(object, "contact_sub_id", hashState);
    invoice->remote_project_id =                bx_object_get_json_int(object, "user_id", hashState);
    invoice->remote_bank_account_id =           bx_object_get_json_int(object, "bank_account_id", hashState);
    invoice->remote_currency_id =               bx_object_get_json_int(object, "currency_id", hashState);
    invoice->remote_payment_type_id =           bx_object_get_json_int(object, "payment_type_id", hashState);
    invoice->remote_tva_type =                  bx_object_get_json_int(object, "mwst_type", hashState);
    invoice->remote_kb_item_status =            bx_object_get_json_int(object, "kb_item_status", hashState);
    invoice->remote_esr_id =                    bx_object_get_json_int(object, "esr_id", hashState);
    invoice->remote_qr_invoice_id =             bx_object_get_json_int(object, "qr_invoice_id", hashState);

    /* double */
    invoice->remote_total_gross =               bx_object_get_json_double(object, "total_gross", hashState);
    invoice->remote_total_net =                 bx_object_get_json_double(object, "total_net", hashState);
    invoice->remote_total_taxes =               bx_object_get_json_double(object, "total_taxes", hashState);
    invoice->remote_total_received_payments =   bx_object_get_json_double(object, "total_received_payments", hashState);
    invoice->remote_total_credit_vouchers =     bx_object_get_json_double(object, "total_credit_vouchers", hashState);
    invoice->remote_total_remaining_payments =  bx_object_get_json_double(object, "total_remaining_payments", hashState);
    invoice->remote_total =                     bx_object_get_json_double(object, "total", hashState);
    invoice->remote_total_rounding_difference = bx_object_get_json_double(object, "total_rounding_difference", hashState);

    /* string */
    invoice->remote_document_nr =               bx_object_get_json_string(object, "document_nr", hashState);
    invoice->remote_is_valid_from =             bx_object_get_json_string(object, "is_valid_from", hashState);
    invoice->remote_is_valid_to =               bx_object_get_json_string(object, "is_valid_to", hashState);
    invoice->remote_contact_address =           bx_object_get_json_string(object,"contact_address", hashState);
    invoice->remote_template_slug =             bx_object_get_json_string(object, "template_slug", hashState);
    invoice->remote_updated_at =                bx_object_get_json_string(object, "updated_at", hashState);
    invoice->remote_reference =                 bx_object_get_json_string(object, "reference", hashState);
    invoice->remote_api_reference =             bx_object_get_json_string(object, "api_reference", hashState);
    invoice->remote_viewed_by_client_at =       bx_object_get_json_string(object, "viewed_by_client_at", hashState);

    invoice->remote_tva_is_net =                bx_object_get_json_bool(object, "tva_is_net", hashState);

    json_t * value = json_object_get(object, "taxs");
    if (value != NULL && json_is_array(value)) {
        invoice->bx_object_taxes_count = json_array_size(value);
        invoice->remote_taxes = calloc(invoice->bx_object_taxes_count, sizeof(*invoice->remote_taxes));
        if (invoice->remote_taxes != NULL) {
            for (int i = 0; i < invoice->bx_object_taxes_count; i++) {
                json_t * o = json_array_get(value, i);
                invoice->remote_taxes[i].remote_value = bx_object_get_json_double(o, "value", hashState);
                invoice->remote_taxes[i].remote_percentage = bx_object_get_json_double(o, "percentage", hashState);
            }
        }
    }
    value = NULL;

    value = json_object_get(object, "positions");
    if (value != NULL && json_is_array(value)) {
        invoice->bx_object_remote_positions_count = json_array_size(value);
        invoice->remote_positions = calloc(invoice->bx_object_remote_positions_count, sizeof(*invoice->remote_positions));
        if (invoice->remote_positions != NULL) {
            for(int i = 0; i < invoice->bx_object_remote_positions_count; i++) {
                json_t * o = json_array_get(value, i);
                BXObjectPositions * p = &invoice->remote_positions[i];

                p->remote_id = bx_object_get_json_int(o, "id", hashState);
                p->remote_unit_id = bx_object_get_json_int(o, "unit_id", hashState);
                p->remote_account_id = bx_object_get_json_int(o, "account_id", hashState);
                p->remote_tax_id = bx_object_get_json_int(o, "tax_id", hashState);
                p->remote_pos = bx_object_get_json_int(o, "pos", hashState);
                p->remote_internal_pos = bx_object_get_json_int(o, "internal_pos", hashState);
                p->remote_parent_id = bx_object_get_json_int(o, "parent_id", hashState);

                p->remote_amount = bx_object_get_json_double(o, "amount", hashState);
                p->remote_tax_value = bx_object_get_json_double(o, "tax_value", hashState);
                p->remote_unit_price = bx_object_get_json_double(o, "unit_price", hashState);
                p->remote_discount_in_percent = bx_object_get_json_double(o, "discount_in_percent", hashState);
                p->remote_position_total = bx_object_get_json_double(o, "position_total", hashState);

                p->remote_unit_name = bx_object_get_json_string(o, "unit_name", hashState);
                p->remote_text = bx_object_get_json_string(o, "text", hashState);
                p->remote_type = bx_object_get_json_string(o, "type", hashState);

                p->remote_is_optional = bx_object_get_json_bool(o, "is_optional", hashState);
            }
        }
    }
    invoice->checksum = XXH3_64bits_digest(hashState);
    XXH3_freeState(hashState);
    return invoice;
}
