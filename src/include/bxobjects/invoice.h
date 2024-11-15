#ifndef BX_OBJECT_INVOICE_H__
#define BX_OBJECT_INVOICE_H__

#include <bx_object_value.h>

typedef struct s_BXObjectPositions BXObjectPositions;
struct s_BXObjectPositions
{
    char * type;

    uint64_t id;
    uint64_t checksum;
    BXInteger remote_id;
    BXInteger remote_unit_id;
    BXInteger remote_account_id;
    BXInteger remote_tax_id;
    BXInteger remote_pos;
    BXInteger remote_internal_pos;
    BXInteger remote_parent_id;

    BXBool remote_is_optional;

    BXFloat remote_amount;
    BXFloat remote_tax_value;
    BXFloat remote_unit_price;
    BXFloat remote_discount_in_percent;
    BXFloat remote_position_total;

    BXString remote_unit_name;
    BXString remote_text;
    BXString remote_type;
};

typedef struct s_BXObjectTaxes BXObjectTaxes;
struct s_BXObjectTaxes {
    const char * type;

    uint64_t id;
    uint64_t checksum;

    BXFloat remote_percentage;
    BXFloat remote_value;
};

typedef struct s_BXObjectInvoice BXObjectInvoice;
struct s_BXObjectInvoice {
    const char * type;

    uint64_t id;
    uint64_t checksum;

    BXInteger remote_id;
    BXInteger remote_contact_id;
    BXInteger remote_contact_subid;
    BXInteger remote_user_id;
    BXInteger remote_project_id;
    BXInteger remote_bank_account_id;
    BXInteger remote_currency_id;
    BXInteger remote_payment_type_id;
    BXInteger remote_esr_id;
    BXInteger remote_qr_invoice_id;
    BXInteger remote_tva_type;
    BXInteger remote_kb_item_status;

    BXFloat remote_total_gross;
    BXFloat remote_total_net;
    BXFloat remote_total_taxes;
    BXFloat remote_total_received_payments;
    BXFloat remote_total_credit_vouchers;
    BXFloat remote_total_remaining_payments;
    BXFloat remote_total;
    BXFloat remote_total_rounding_difference;

    BXBool remote_tva_is_net;

    BXString remote_reference;
    BXString remote_api_reference;
    BXString remote_document_nr;
    BXString remote_template_slug;
    BXString remote_is_valid_from;
    BXString remote_is_valid_to;
    BXString remote_contact_address;
    BXString remote_updated_at;
    BXString remote_viewed_by_client_at;

    int bx_object_taxes_count;
    BXObjectTaxes * remote_taxes;
    int bx_object_remote_positions_count;
    BXObjectPositions * remote_positions;
};

void * bx_object_invoice_decode(void * data);
void bx_object_invoice_free(void * data);
void bx_object_invoice_dump(void * data);

#endif /* BX_OBJECT_INVOICE_H__ */