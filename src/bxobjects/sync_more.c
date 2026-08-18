#include "../include/bx_sync_more.h"
#include "../include/bx_named_list.h"
#include "../include/bx_object.h"
#include "../include/bx_upsert.h"
#include "../include/bx_utils.h"
#include "../include/bxobjects/position.h"
#include <jansson.h>
#include <stdio.h>
#include <string.h>

#define F BX_FIELD
#define FS BX_FIELD_SQL

static BXillError up(MYSQL *conn, const char *table, json_t *item,
                     const BXJsonField *fields, size_t n) {
  return bx_json_upsert(conn, table, item, fields, n, NULL, 0);
}

static json_t *json_as_array(json_t *root) {
  if (json_is_array(root)) {
    return root;
  }
  if (json_is_object(root)) {
    json_t *data = json_object_get(root, "data");
    if (json_is_array(data)) {
      return data;
    }
  }
  return NULL;
}

static BXillError keep_going(BXillError e, const char *label) {
  if (e == NoError) {
    return NoError;
  }
  if (e == ErrorSQLReconnect) {
    return e;
  }
  bx_log_error("Step2 sync failed (%s): %d", label, (int)e);
  return NoError;
}

static BXillError country_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("name", BX_F_STR), F("name_short", BX_F_STR),
      F("iso3166_alpha2", BX_F_STR)};
  return up(conn, "country", item, f, 4);
}

static BXillError account_group_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("uuid", BX_F_UUID), F("account_no", BX_F_STR),
      F("name", BX_F_STR), F("parent_fibu_account_group_id", BX_F_UINT),
      F("is_active", BX_F_BOOL), F("is_locked", BX_F_BOOL)};
  return up(conn, "account_group", item, f, 7);
}

static BXillError client_service_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("name", BX_F_STR),
      F("default_is_billable", BX_F_BOOL),
      F("default_price_per_hour", BX_F_FLOAT), F("account_id", BX_F_UINT)};
  return up(conn, "client_service", item, f, 5);
}

static BXillError bank_account_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT),          F("name", BX_F_STR),
      F("owner", BX_F_STR),        F("owner_address", BX_F_STR),
      F("owner_zip", BX_F_STR),    F("owner_city", BX_F_STR),
      F("owner_country_code", BX_F_STR), F("bc_nr", BX_F_STR),
      F("bank_name", BX_F_STR),    F("bank_nr", BX_F_STR),
      F("bank_account_nr", BX_F_STR), F("iban_nr", BX_F_STR),
      F("currency_id", BX_F_UINT), F("account_id", BX_F_UINT),
      F("remarks", BX_F_STR),      F("invoice_mode", BX_F_STR),
      F("qr_invoice_iban", BX_F_STR), F("type", BX_F_STR)};
  return up(conn, "bank_account", item, f, 18);
}

static BXillError company_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("name", BX_F_STR), F("address", BX_F_STR),
      F("address_nr", BX_F_STR), F("postcode", BX_F_STR), F("city", BX_F_STR),
      F("country_id", BX_F_UINT), F("legal_form", BX_F_STR),
      F("country_name", BX_F_STR), F("mail", BX_F_STR),
      F("phone_fixed", BX_F_STR), F("phone_mobile", BX_F_STR),
      F("fax", BX_F_STR), F("url", BX_F_STR), F("ust_id_nr", BX_F_STR),
      F("mwst_nr", BX_F_STR), F("trade_register_nr", BX_F_STR)};
  return up(conn, "company_profile", item, f, 17);
}

static BXillError calendar_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("start", BX_F_STR), F("end", BX_F_STR),
      F("is_vat_subject", BX_F_BOOL), F("is_annual_reporting", BX_F_BOOL),
      F("created_at", BX_F_STR), F("updated_at", BX_F_STR),
      F("vat_accounting_method", BX_F_STR), F("vat_accounting_type", BX_F_STR)};
  return up(conn, "calendar_year", item, f, 9);
}

static BXillError business_year_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {F("id", BX_F_UINT), F("start", BX_F_STR),
                                  F("end", BX_F_STR), F("status", BX_F_STR),
                                  F("closed_at", BX_F_STR)};
  return up(conn, "business_year", item, f, 5);
}

static BXillError vat_period_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {F("id", BX_F_UINT), F("start", BX_F_STR),
                                  F("end", BX_F_STR), F("type", BX_F_STR),
                                  F("status", BX_F_STR),
                                  F("closed_at", BX_F_STR)};
  return up(conn, "vat_period", item, f, 6);
}

BXillError bx_lookups_walk_more(bXill *app, MYSQL *conn) {
  BXillError e;
  bx_log_debug("BX Walk extra lookups");
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/country?limit=$&offset=$",
                                   country_one),
                      "country")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list(app, conn, "2.0/account_groups?limit=$&offset=$",
                        account_group_one),
           "account_group")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_communication_kind_walk_items(app, conn),
                      "communication_kind")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_project_state_walk_items(app, conn),
                      "pr_project_state")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_project_type_walk_items(app, conn),
                      "pr_project_type")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_timesheet_status_walk_items(app, conn),
                      "timesheet_status")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_todo_status_walk_items(app, conn), "todo_status")) !=
      NoError) {
    return e;
  }
  if ((e = keep_going(bx_todo_priority_walk_items(app, conn),
                      "todo_priority")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_stock_walk_items(app, conn), "stock")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_stock_place_walk_items(app, conn), "stock_place")) !=
      NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list(app, conn, "2.0/client_service?limit=$&offset=$",
                        client_service_one),
           "client_service")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "3.0/banking/accounts?limit=$&offset=$",
                                   bank_account_one),
                      "bank_account")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/company_profile", company_one),
                      "company_profile")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list(app, conn,
                        "3.0/accounting/calendar_years?limit=$&offset=$",
                        calendar_one),
           "calendar_year")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "3.0/accounting/business_years",
                                   business_year_one),
                      "business_year")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "3.0/accounting/vat_periods",
                                   vat_period_one),
                      "vat_period")) != NoError) {
    return e;
  }
  return NoError;
}

static BXillError relation_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("contact_id", BX_F_UINT),
      F("contact_sub_id", BX_F_UINT), F("description", BX_F_STR),
      F("updated_at", BX_F_STR)};
  return up(conn, "contact_relation", item, f, 5);
}

static BXillError article_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("user_id", BX_F_UINT),
      F("article_type_id", BX_F_UINT), F("contact_id", BX_F_UINT),
      F("intern_code", BX_F_STR), F("intern_name", BX_F_STR),
      F("intern_description", BX_F_STR), F("purchase_price", BX_F_FLOAT),
      F("sale_price", BX_F_FLOAT), F("currency_id", BX_F_UINT),
      F("tax_id", BX_F_UINT), F("unit_id", BX_F_UINT), F("is_stock", BX_F_BOOL),
      F("stock_id", BX_F_UINT), F("stock_place_id", BX_F_UINT),
      F("stock_nr", BX_F_FLOAT), F("remarks", BX_F_STR),
      F("article_group_id", BX_F_UINT), F("account_id", BX_F_UINT)};
  return up(conn, "article", item, f, 19);
}

static BXillError note_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("user_id", BX_F_UINT), F("event_start", BX_F_STR),
      F("subject", BX_F_STR), F("info", BX_F_STR), F("contact_id", BX_F_UINT),
      F("project_id", BX_F_UINT), F("entry_id", BX_F_UINT),
      F("module_id", BX_F_UINT)};
  return up(conn, "note", item, f, 9);
}

static BXillError task_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("user_id", BX_F_UINT), F("finish_date", BX_F_STR),
      F("subject", BX_F_STR), F("info", BX_F_STR), F("contact_id", BX_F_UINT),
      F("sub_contact_id", BX_F_UINT), F("project_id", BX_F_UINT),
      F("todo_status_id", BX_F_UINT), F("todo_priority_id", BX_F_UINT),
      F("has_reminder", BX_F_BOOL), F("communication_kind_id", BX_F_UINT)};
  return up(conn, "task", item, f, 12);
}

static BXillError timesheet_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("user_id", BX_F_UINT), F("status_id", BX_F_UINT),
      F("client_service_id", BX_F_UINT), F("text", BX_F_STR),
      F("allowable_bill", BX_F_BOOL), F("contact_id", BX_F_UINT),
      F("sub_contact_id", BX_F_UINT), F("pr_project_id", BX_F_UINT),
      F("pr_package_id", BX_F_UINT), F("pr_milestone_id", BX_F_UINT),
      F("estimated_time", BX_F_STR), F("date", BX_F_STR),
      F("duration", BX_F_STR), F("running", BX_F_BOOL)};
  return up(conn, "timesheet", item, f, 15);
}

static BXillError purchase_order_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("document_nr", BX_F_STR), F("title", BX_F_STR),
      F("contact_id", BX_F_UINT), F("contact_sub_id", BX_F_UINT),
      F("user_id", BX_F_UINT), F("project_id", BX_F_UINT),
      F("language_id", BX_F_UINT), F("bank_account_id", BX_F_UINT),
      F("currency_id", BX_F_UINT), F("payment_type_id", BX_F_UINT),
      F("header", BX_F_STR), F("footer", BX_F_STR),
      F("template_slug", BX_F_STR)};
  return up(conn, "purchase_order", item, f, 14);
}

static BXillError bill_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_STR), F("created_at", BX_F_STR), F("document_no", BX_F_STR),
      F("status", BX_F_STR), F("vendor_ref", BX_F_STR), F("vendor", BX_F_STR),
      F("title", BX_F_STR), F("currency_code", BX_F_STR),
      F("pending_amount", BX_F_FLOAT), F("net", BX_F_FLOAT),
      F("gross", BX_F_FLOAT), F("bill_date", BX_F_STR), F("due_date", BX_F_STR),
      F("overdue", BX_F_BOOL)};
  return up(conn, "purchase_bill", item, f, 14);
}

static BXillError expense_one(MYSQL *conn, json_t *item) {
  static const BXJsonField f[] = {
      F("id", BX_F_STR), F("created_at", BX_F_STR), F("document_no", BX_F_STR),
      F("status", BX_F_STR), F("vendor", BX_F_STR), F("title", BX_F_STR),
      F("currency_code", BX_F_STR), F("paid_on", BX_F_STR),
      F("booking_account_id", BX_F_UINT), F("net", BX_F_FLOAT),
      F("gross", BX_F_FLOAT), F("chargeable_contact_id", BX_F_UINT)};
  return up(conn, "expense", item, f, 12);
}

static BXillError manual_one(MYSQL *conn, json_t *item) {
  static const BXJsonField head[] = {
      F("id", BX_F_UINT), F("type", BX_F_STR), F("date", BX_F_STR),
      F("reference_nr", BX_F_STR), F("created_by_user_id", BX_F_UINT),
      F("edited_by_user_id", BX_F_UINT), F("is_locked", BX_F_BOOL),
      F("locked_info", BX_F_STR)};
  BXillError e = up(conn, "manual_entry", item, head, 8);
  if (e != NoError) {
    return e;
  }
  uint64_t entry_id = (uint64_t)json_integer_value(json_object_get(item, "id"));
  json_t *lines = json_object_get(item, "entries");
  if (!json_is_array(lines)) {
    return NoError;
  }
  static const BXJsonField linef[] = {
      F("id", BX_F_UINT), F("date", BX_F_STR), F("debit_account_id", BX_F_UINT),
      F("credit_account_id", BX_F_UINT), F("tax_id", BX_F_UINT),
      F("description", BX_F_STR), F("amount", BX_F_FLOAT),
      F("currency_id", BX_F_UINT)};
  size_t n = json_array_size(lines);
  for (size_t i = 0; i < n; i++) {
    e = bx_json_upsert(conn, "manual_entry_line", json_array_get(lines, i),
                       linef, 8, "_entry_id", entry_id);
    if (e != NoError) {
      return e;
    }
  }
  return NoError;
}

BXillError bx_records_walk_more(bXill *app, MYSQL *conn) {
  BXillError e;
  bx_log_debug("BX Walk extra records");
  if ((e = keep_going(
           bx_walk_list(app, conn, "2.0/contact_relation?limit=$&offset=$",
                        relation_one),
           "contact_relation")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/article?limit=$&offset=$",
                                   article_one),
                      "article")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/note?limit=$&offset=$",
                                   note_one),
                      "note")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/task?limit=$&offset=$",
                                   task_one),
                      "task")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "2.0/timesheet?limit=$&offset=$",
                                   timesheet_one),
                      "timesheet")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list(app, conn, "3.0/purchase_orders?limit=$&offset=$",
                        purchase_order_one),
           "purchase_order")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn,
                                   "4.0/purchase/bills?limit=$&page=$",
                                   bill_one),
                      "purchase_bill")) != NoError) {
    return e;
  }
  if ((e = keep_going(bx_walk_list(app, conn, "4.0/expenses?limit=$&page=$",
                                   expense_one),
                      "expense")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list(app, conn,
                        "3.0/accounting/manual_entries?limit=$&offset=$",
                        manual_one),
           "manual_entry")) != NoError) {
    return e;
  }
  return NoError;
}

static const BXJsonField kb_quote_fields[] = {
    F("id", BX_F_UINT), F("document_nr", BX_F_STR), F("title", BX_F_STR),
    F("contact_id", BX_F_UINT), F("contact_sub_id", BX_F_UINT),
    F("user_id", BX_F_UINT), F("project_id", BX_F_UINT),
    F("language_id", BX_F_UINT), F("bank_account_id", BX_F_UINT),
    F("currency_id", BX_F_UINT), F("payment_type_id", BX_F_UINT),
    F("header", BX_F_STR), F("footer", BX_F_STR), F("total_gross", BX_F_FLOAT),
    F("total_net", BX_F_FLOAT), F("total_taxes", BX_F_FLOAT),
    F("total", BX_F_FLOAT), F("total_rounding_difference", BX_F_FLOAT),
    F("mwst_type", BX_F_INT), F("mwst_is_net", BX_F_UINT),
    F("show_position_taxes", BX_F_UINT), F("is_valid_from", BX_F_STR),
    F("is_valid_until", BX_F_STR), F("contact_address", BX_F_STR),
    F("kb_item_status_id", BX_F_INT), F("updated_at", BX_F_STR),
    F("template_slug", BX_F_STR)};

static const BXJsonField kb_order_fields[] = {
    F("id", BX_F_UINT), F("document_nr", BX_F_STR), F("title", BX_F_STR),
    F("contact_id", BX_F_UINT), F("contact_sub_id", BX_F_UINT),
    F("user_id", BX_F_UINT), F("project_id", BX_F_UINT),
    F("language_id", BX_F_UINT), F("bank_account_id", BX_F_UINT),
    F("currency_id", BX_F_UINT), F("payment_type_id", BX_F_UINT),
    F("header", BX_F_STR), F("footer", BX_F_STR), F("total_gross", BX_F_FLOAT),
    F("total_net", BX_F_FLOAT), F("total_taxes", BX_F_FLOAT),
    F("total", BX_F_FLOAT), F("total_rounding_difference", BX_F_FLOAT),
    F("mwst_type", BX_F_INT), F("mwst_is_net", BX_F_UINT),
    F("is_valid_from", BX_F_STR), F("contact_address", BX_F_STR),
    F("kb_item_status_id", BX_F_INT), F("updated_at", BX_F_STR)};

static const BXJsonField kb_delivery_fields[] = {
    F("id", BX_F_UINT), F("document_nr", BX_F_STR), F("title", BX_F_STR),
    F("contact_id", BX_F_UINT), F("user_id", BX_F_UINT),
    F("project_id", BX_F_UINT), F("language_id", BX_F_UINT),
    F("kb_item_status_id", BX_F_INT), F("is_valid_from", BX_F_STR),
    F("contact_address", BX_F_STR), F("updated_at", BX_F_STR)};

static BXillError kb_doc_one(bXill *app, MYSQL *conn, json_t *item,
                             const char *table, const char *get_fmt,
                             const BXJsonField *fields, size_t nfields,
                             const char *pos_table, const char *pos_parent) {
  json_t *positions = json_object_get(item, "positions");
  json_t *src = item;
  BXNetRequest *full = NULL;
  if (positions == NULL) {
    BXUInteger id = bx_object_get_json_uint(item, "id", NULL);
    full = bx_do_request(app->queue, NULL, (char *)get_fmt, (BXGeneric *)&id);
    if (full && full->decoded) {
      src = full->decoded;
      positions = json_object_get(src, "positions");
    }
  }
  BXillError e = bx_json_upsert(conn, table, src, fields, nfields, NULL, 0);
  if (e == NoError && pos_table && positions) {
    uint64_t id = (uint64_t)json_integer_value(json_object_get(src, "id"));
    e = bx_kb_positions_store(conn, pos_table, pos_parent, id, positions);
  }
  if (e == NoError && src) {
    uint64_t id = (uint64_t)json_integer_value(json_object_get(src, "id"));
    const char *kb =
        (strcmp(table, "kb_quote") == 0)
            ? "kb_offer"
            : (strcmp(table, "kb_order") == 0 ? "kb_order" : "kb_delivery");
    char path[96];
    snprintf(path, sizeof(path), "2.0/%s/%lu/comment", kb, (unsigned long)id);
    BXNetRequest *cr = bx_do_request(app->queue, NULL, path);
    if (cr && json_is_array(cr->decoded)) {
      static const BXJsonField cf[] = {
          F("id", BX_F_UINT),          F("text", BX_F_STR),
          F("user_id", BX_F_UINT),     F("user_name", BX_F_STR),
          F("date", BX_F_STR),         F("is_public", BX_F_BOOL),
          F("_document_type", BX_F_STR)};
      size_t cn = json_array_size(cr->decoded);
      for (size_t i = 0; i < cn; i++) {
        json_t *row = json_array_get(cr->decoded, i);
        json_object_set_new(row, "_document_type", json_string(kb));
        (void)bx_json_upsert(conn, "kb_comment", row, cf, 7, "_document_id",
                             id);
      }
    }
    bx_net_request_free(cr);
  }
  bx_net_request_free(full);
  return e;
}

static BXillError quote_one(bXill *app, MYSQL *conn, json_t *item) {
  return kb_doc_one(app, conn, item, "kb_quote", "2.0/kb_offer/$",
                    kb_quote_fields,
                    sizeof(kb_quote_fields) / sizeof(kb_quote_fields[0]),
                    "quote_position", "_quote_id");
}

static BXillError order_one(bXill *app, MYSQL *conn, json_t *item) {
  return kb_doc_one(app, conn, item, "kb_order", "2.0/kb_order/$",
                    kb_order_fields,
                    sizeof(kb_order_fields) / sizeof(kb_order_fields[0]),
                    "order_position", "_order_id");
}

static BXillError delivery_one(bXill *app, MYSQL *conn, json_t *item) {
  return kb_doc_one(app, conn, item, "kb_delivery", "2.0/kb_delivery/$",
                    kb_delivery_fields,
                    sizeof(kb_delivery_fields) / sizeof(kb_delivery_fields[0]),
                    NULL, NULL);
}

BXillError bx_kb_sales_walk_items(bXill *app, MYSQL *conn) {
  BXillError e;
  bx_log_debug("BX Walk quotes/orders/deliveries");
  if ((e = keep_going(
           bx_walk_list_app(app, conn, "2.0/kb_offer?limit=$&offset=$",
                            quote_one),
           "kb_quote")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list_app(app, conn, "2.0/kb_order?limit=$&offset=$",
                            order_one),
           "kb_order")) != NoError) {
    return e;
  }
  if ((e = keep_going(
           bx_walk_list_app(app, conn, "2.0/kb_delivery?limit=$&offset=$",
                            delivery_one),
           "kb_delivery")) != NoError) {
    return e;
  }
  return NoError;
}

BXillError bx_contact_extra_sync(bXill *app, MYSQL *conn, uint64_t contact_id) {
  BXUInteger id = {
      .type = BX_OBJECT_TYPE_UINTEGER, .isset = true, .value = contact_id};
  BXNetRequest *req = bx_do_request(
      app->queue, NULL, "2.0/contact/$/additional_address", (BXGeneric *)&id);
  if (req == NULL) {
    return ErrorNet;
  }
  json_t *arr = json_as_array(req->decoded);
  if (arr == NULL) {
    bx_net_request_free(req);
    return NoError;
  }
  static const BXJsonField f[] = {
      F("id", BX_F_UINT), F("name", BX_F_STR), F("name_addition", BX_F_STR),
      F("address", BX_F_STR), F("street_name", BX_F_STR),
      F("house_number", BX_F_STR), F("address_addition", BX_F_STR),
      F("postcode", BX_F_STR), F("city", BX_F_STR), F("country_id", BX_F_UINT),
      F("subject", BX_F_STR), F("description", BX_F_STR)};
  size_t n = json_array_size(arr);
  for (size_t i = 0; i < n; i++) {
    BXillError e = bx_json_upsert(conn, "additional_address",
                                  json_array_get(arr, i), f, 12,
                                  "_contact_id", contact_id);
    if (e != NoError) {
      bx_net_request_free(req);
      return e;
    }
  }
  bx_net_request_free(req);
  return NoError;
}

BXillError bx_invoice_extra_sync(bXill *app, MYSQL *conn, uint64_t invoice_id) {
  BXUInteger id = {
      .type = BX_OBJECT_TYPE_UINTEGER, .isset = true, .value = invoice_id};
  BXNetRequest *pay = bx_do_request(app->queue, NULL,
                                    "2.0/kb_invoice/$/payment", (BXGeneric *)&id);
  json_t *pay_arr = pay ? json_as_array(pay->decoded) : NULL;
  if (pay_arr) {
    static const BXJsonField f[] = {
        F("id", BX_F_UINT), F("date", BX_F_STR), F("value", BX_F_FLOAT),
        F("bank_account_id", BX_F_UINT), F("title", BX_F_STR),
        F("is_cash_discount", BX_F_BOOL), F("kb_invoice_id", BX_F_UINT)};
    size_t n = json_array_size(pay_arr);
    for (size_t i = 0; i < n; i++) {
      (void)bx_json_upsert(conn, "invoice_payment",
                           json_array_get(pay_arr, i), f, 7, "_invoice_id",
                           invoice_id);
    }
  }
  bx_net_request_free(pay);

  BXNetRequest *rem = bx_do_request(
      app->queue, NULL, "2.0/kb_invoice/$/kb_reminder", (BXGeneric *)&id);
  json_t *rem_arr = rem ? json_as_array(rem->decoded) : NULL;
  if (rem_arr) {
    static const BXJsonField f[] = {
        F("id", BX_F_UINT), F("kb_invoice_id", BX_F_UINT), F("title", BX_F_STR),
        F("is_valid_from", BX_F_STR), F("is_valid_to", BX_F_STR),
        F("reminder_level", BX_F_INT), F("is_sent", BX_F_BOOL),
        F("remaining_price", BX_F_FLOAT)};
    size_t n = json_array_size(rem_arr);
    for (size_t i = 0; i < n; i++) {
      (void)bx_json_upsert(conn, "invoice_reminder",
                           json_array_get(rem_arr, i), f, 8, "_invoice_id",
                           invoice_id);
    }
  }
  bx_net_request_free(rem);

  BXNetRequest *cmt = bx_do_request(app->queue, NULL, "2.0/kb_invoice/$/comment",
                                    (BXGeneric *)&id);
  json_t *cmt_arr = cmt ? json_as_array(cmt->decoded) : NULL;
  if (cmt_arr) {
    static const BXJsonField cf[] = {
        F("id", BX_F_UINT), F("text", BX_F_STR), F("user_id", BX_F_UINT),
        F("user_name", BX_F_STR), F("date", BX_F_STR),
        F("is_public", BX_F_BOOL), F("_document_type", BX_F_STR)};
    size_t n = json_array_size(cmt_arr);
    for (size_t i = 0; i < n; i++) {
      json_t *row = json_array_get(cmt_arr, i);
      json_object_set_new(row, "_document_type", json_string("kb_invoice"));
      (void)bx_json_upsert(conn, "kb_comment", row, cf, 7, "_document_id",
                           invoice_id);
    }
  }
  bx_net_request_free(cmt);
  return NoError;
}

BXillError bx_project_extra_sync(bXill *app, MYSQL *conn, uint64_t project_id) {
  BXUInteger id = {
      .type = BX_OBJECT_TYPE_UINTEGER, .isset = true, .value = project_id};
  BXNetRequest *ms = bx_do_request(app->queue, NULL,
                                   "3.0/projects/$/milestones", (BXGeneric *)&id);
  json_t *ms_arr = ms ? json_as_array(ms->decoded) : NULL;
  if (ms_arr) {
    static const BXJsonField f[] = {
        F("id", BX_F_UINT), F("name", BX_F_STR), F("end_date", BX_F_STR),
        F("comment", BX_F_STR), F("pr_parent_milestone_id", BX_F_UINT)};
    size_t n = json_array_size(ms_arr);
    for (size_t i = 0; i < n; i++) {
      (void)bx_json_upsert(conn, "project_milestone",
                           json_array_get(ms_arr, i), f, 5, "_project_id",
                           project_id);
    }
  }
  bx_net_request_free(ms);

  BXNetRequest *pk =
      bx_do_request(app->queue, NULL, "3.0/projects/$/packages", (BXGeneric *)&id);
  json_t *pk_arr = pk ? json_as_array(pk->decoded) : NULL;
  if (pk_arr) {
    static const BXJsonField f[] = {
        F("id", BX_F_UINT), F("name", BX_F_STR),
        F("spent_time_in_hours", BX_F_FLOAT),
        F("estimated_time_in_hours", BX_F_FLOAT), F("comment", BX_F_STR),
        F("pr_milestone_id", BX_F_UINT)};
    size_t n = json_array_size(pk_arr);
    for (size_t i = 0; i < n; i++) {
      (void)bx_json_upsert(conn, "project_package",
                           json_array_get(pk_arr, i), f, 6, "_project_id",
                           project_id);
    }
  }
  bx_net_request_free(pk);
  return NoError;
}
