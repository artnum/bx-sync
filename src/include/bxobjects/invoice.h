#ifndef BX_OBJECT_INVOICE_H__
#define BX_OBJECT_INVOICE_H__

#include "../bx_object.h"
#include "../bx_object_value.h"
#include "../bxill.h"
#include "position.h"
#include "tax.h"

typedef struct s_BXObjectPositions BXObjectPositions;
struct s_BXObjectPositions {
  char *type;

  uint64_t id;
  uint64_t checksum;
  BXUInteger remote_id;
  BXUInteger remote_unit_id;
  BXUInteger remote_account_id;
  BXUInteger remote_tax_id;
  BXUInteger remote_pos;
  BXUInteger remote_internal_pos;
  BXUInteger remote_parent_id;

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

typedef struct s_BXObjectInvoice BXObjectInvoice;
struct s_BXObjectInvoice {
  enum e_BXObjectType type;

  uint64_t checksum;

  BXUInteger id;
  BXUInteger contact_id;
  BXUInteger contact_sub_id;
  BXUInteger language_id;
  BXUInteger user_id;
  BXUInteger project_id;
  BXUInteger bank_account_id;
  BXUInteger currency_id;
  BXUInteger payment_type_id;
  BXUInteger esr_id;
  BXUInteger qr_invoice_id;
  BXUInteger tva_type;
  BXUInteger kb_item_status;
  BXUInteger mwst_type;
  BXUInteger mwst_is_net;
  BXUInteger show_position_taxes;

  BXFloat total_gross;
  BXFloat total_net;
  BXFloat total_taxes;
  BXFloat total_received_payments;
  BXFloat total_credit_vouchers;
  BXFloat total_remaining_payments;
  BXFloat total;
  BXFloat total_rounding_difference;

  BXBool tva_is_net;

  BXString title;
  BXString reference;
  BXString api_reference;
  BXString document_nr;
  BXString template_slug;
  BXString is_valid_from;
  BXString is_valid_to;
  BXString contact_address;
  BXString updated_at;
  BXString viewed_by_client_at;
  BXString header;
  BXString footer;
  BXString network_link;

  int bx_object_taxes_count;
  BXObjectTax **remote_taxes;
  int bx_object_remote_positions_count;
  BXObjectGenericPosition **remote_positions;
};

void *bx_object_invoice_decode(void *data);
void bx_object_invoice_free(void *data);
void bx_object_invoice_dump(void *data);
void bx_invoice_walk_items(bXill *app, MYSQL *conn);

#endif /* BX_OBJECT_INVOICE_H__ */
