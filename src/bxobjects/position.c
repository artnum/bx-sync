#include <bxobjects/position.h>
#include <bx_object.h>
#include <bx_utils.h>
#include <assert.h>
#include <jansson.h>

/* xxh3_32bits */
#define KbPositionCustom    0x2cf90d36
#define KbPositionArticle   0xb40e589d
#define KbPositionText      0x54907120
#define KbPositionSubtotal  0x6448960f
#define KbPositionPagebreak 0x40e1fad6
#define KbPositionDiscount  0xd54e4502

static inline void * _bx_kb_position_custom(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionCE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionCE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);
    position->remote_unit_id = bx_object_get_json_int(jroot, "unit_id", hashState);
    position->remote_account_id = bx_object_get_json_int(jroot, "account_id", hashState);
    position->remote_tax_id = bx_object_get_json_int(jroot, "tax_id", hashState);
    position->remote_internal_pos = bx_object_get_json_int(jroot, "internal_pos", hashState);
    position->remote_parent_id = bx_object_get_json_int(jroot, "parent_id", hashState);

    position->remote_unit_name = bx_object_get_json_string(jroot, "unit_name", hashState);
    position->remote_text = bx_object_get_json_string(jroot, "text", hashState);
    position->remote_pos = bx_object_get_json_string(jroot, "pos", hashState);

    position->remote_is_optional = bx_object_get_json_bool(jroot, "is_optional", hashState);

    position->remote_amount = bx_object_get_json_double(jroot, "amount", hashState);
    position->remote_tax_value = bx_object_get_json_double(jroot, "tax_value", hashState);
    position->remote_unit_price = bx_object_get_json_double(jroot, "unit_price", hashState);
    position->remote_discount_in_percent = bx_object_get_json_double(jroot, "discount_in_percent", hashState);
    position->remote_position_total = bx_object_get_json_double(jroot, "position_total", hashState);

    position->checksum = XXH3_64bits_digest(hashState);

    return position;
}
static inline void * _bx_kb_position_article(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionAE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionAE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);
    position->remote_unit_id = bx_object_get_json_int(jroot, "unit_id", hashState);
    position->remote_account_id = bx_object_get_json_int(jroot, "account_id", hashState);
    position->remote_tax_id = bx_object_get_json_int(jroot, "tax_id", hashState);
    position->remote_internal_pos = bx_object_get_json_int(jroot, "internal_pos", hashState);
    position->remote_parent_id = bx_object_get_json_int(jroot, "parent_id", hashState);
    position->remote_article_id = bx_object_get_json_int(jroot, "article_id", hashState);

    position->remote_unit_name = bx_object_get_json_string(jroot, "unit_name", hashState);
    position->remote_text = bx_object_get_json_string(jroot, "text", hashState);
    position->remote_pos = bx_object_get_json_string(jroot, "pos", hashState);

    position->remote_is_optional = bx_object_get_json_bool(jroot, "is_optional", hashState);

    position->remote_amount = bx_object_get_json_double(jroot, "amount", hashState);
    position->remote_tax_value = bx_object_get_json_double(jroot, "tax_value", hashState);
    position->remote_unit_price = bx_object_get_json_double(jroot, "unit_price", hashState);
    position->remote_discount_in_percent = bx_object_get_json_double(jroot, "discount_in_percent", hashState);
    position->remote_position_total = bx_object_get_json_double(jroot, "position_total", hashState);

    position->checksum = XXH3_64bits_digest(hashState);

    return position;
}
static inline void * _bx_kb_position_text(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionTE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionTE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);
    position->remote_internal_pos = bx_object_get_json_int(jroot, "internal_pos", hashState);
    position->remote_parent_id = bx_object_get_json_int(jroot, "parent_id", hashState);

    position->remote_text = bx_object_get_json_string(jroot, "text", hashState);
    position->remote_pos = bx_object_get_json_string(jroot, "pos", hashState);

    position->remote_is_optional = bx_object_get_json_bool(jroot, "is_optional", hashState);
    position->remote_show_pos_nr = bx_object_get_json_bool(jroot, "show_pos_nr", hashState);

    position->checksum = XXH3_64bits_digest(hashState);

    return position;
}
static inline void * _bx_kb_position_subtotal(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionSE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionSE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);
    position->remote_internal_pos = bx_object_get_json_int(jroot, "internal_pos", hashState);
    position->remote_parent_id = bx_object_get_json_int(jroot, "parent_id", hashState);

    position->remote_text = bx_object_get_json_string(jroot, "text", hashState);

    position->remote_is_optional = bx_object_get_json_bool(jroot, "is_optional", hashState);

    position->remote_value = bx_object_get_json_double(jroot, "value", hashState);

    position->checksum = XXH3_64bits_digest(hashState);

    return position;
}
static inline void * _bx_kb_position_page_break(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionPE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionPE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);
    position->remote_internal_pos = bx_object_get_json_int(jroot, "internal_pos", hashState);
    position->remote_parent_id = bx_object_get_json_int(jroot, "parent_id", hashState);

    position->remote_is_optional = bx_object_get_json_bool(jroot, "is_optional", hashState);

    position->checksum = XXH3_64bits_digest(hashState);
    
    return position;
}
static inline void * _bx_kb_position_discount(json_t * jroot, XXH3_state_t * hashState)
{
    assert(jroot != NULL);
    BXObjectPositionDE * position = NULL;
    position = calloc(1, sizeof(*position));
    if (position == NULL) { return NULL; }

    bx_utils_gen_id(&position->id);

    position->type = BXTypeInvoicePositionDE;

    position->remote_id = bx_object_get_json_int(jroot, "id", hashState);

    position->remote_text = bx_object_get_json_string(jroot, "text", hashState);

    position->remote_is_percentual = bx_object_get_json_bool(jroot, "is_percentual", hashState);

    position->remote_value = bx_object_get_json_double(jroot, "value", hashState);
    position->remote_discount_total = bx_object_get_json_double(jroot, "discount_total", hashState);

    position->checksum = XXH3_64bits_digest(hashState);

    return position;
}


void * bx_object_position_decode(void * object)
{
    assert(object != NULL);
    void * retval = NULL;
    json_t * jroot = (json_t *)object;

    json_t * value = json_object_get(jroot, "type");
    if (value == NULL || !json_is_string(value)) { return NULL; }

    const char * type = json_string_value(value);
    if (type == NULL) {
        return NULL;
    }

    XXH3_state_t * hashState = XXH3_createState();
    if (hashState == NULL) {
        return NULL;
    }
    XXH3_64bits_reset(hashState);
    switch(XXH32(type, strlen(type), 0)) {
        case KbPositionCustom:
            retval = _bx_kb_position_custom(jroot, hashState);
            break;
        case KbPositionArticle:
            retval = _bx_kb_position_article(jroot, hashState);
            break;
        case KbPositionText:
            retval = _bx_kb_position_text(jroot, hashState);
            break;
        case KbPositionSubtotal:
            retval = _bx_kb_position_subtotal(jroot, hashState);
            break;
        case KbPositionPagebreak:
            retval = _bx_kb_position_page_break(jroot, hashState);
            break;
        case KbPositionDiscount:
            retval = _bx_kb_position_discount(jroot, hashState);
            break;
        default:
            break;
    }
    XXH3_freeState(hashState);
    return retval;
}

static inline void _bx_object_position_ce_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionCE * position = (BXObjectPositionCE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_unit_id);
    bx_object_free_value(&position->remote_account_id);
    bx_object_free_value(&position->remote_tax_id);
    bx_object_free_value(&position->remote_internal_pos);
    bx_object_free_value(&position->remote_parent_id);
    bx_object_free_value(&position->remote_unit_name);
    bx_object_free_value(&position->remote_text);
    bx_object_free_value(&position->remote_pos);
    bx_object_free_value(&position->remote_is_optional);
    bx_object_free_value(&position->remote_amount);
    bx_object_free_value(&position->remote_tax_value);
    bx_object_free_value(&position->remote_unit_price);
    bx_object_free_value(&position->remote_discount_in_percent);
    bx_object_free_value(&position->remote_position_total);
    free(position);
}

static inline void _bx_object_position_ae_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionAE * position = (BXObjectPositionAE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_unit_id);
    bx_object_free_value(&position->remote_account_id);
    bx_object_free_value(&position->remote_tax_id);
    bx_object_free_value(&position->remote_internal_pos);
    bx_object_free_value(&position->remote_parent_id);
    bx_object_free_value(&position->remote_article_id);
    bx_object_free_value(&position->remote_unit_name);
    bx_object_free_value(&position->remote_text);
    bx_object_free_value(&position->remote_pos);
    bx_object_free_value(&position->remote_is_optional);
    bx_object_free_value(&position->remote_amount);
    bx_object_free_value(&position->remote_tax_value);
    bx_object_free_value(&position->remote_unit_price);
    bx_object_free_value(&position->remote_discount_in_percent);
    bx_object_free_value(&position->remote_position_total);
    free(position);
}

static inline void _bx_object_position_te_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionTE * position = (BXObjectPositionTE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_internal_pos);
    bx_object_free_value(&position->remote_parent_id);
    bx_object_free_value(&position->remote_text);
    bx_object_free_value(&position->remote_pos);
    bx_object_free_value(&position->remote_is_optional);
    bx_object_free_value(&position->remote_show_pos_nr);
    free(position);
}

static inline void _bx_object_position_se_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionSE * position = (BXObjectPositionSE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_internal_pos);
    bx_object_free_value(&position->remote_parent_id);
    bx_object_free_value(&position->remote_text);
    bx_object_free_value(&position->remote_is_optional);
    free(position);
}

static inline void _bx_object_position_pe_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionPE * position = (BXObjectPositionPE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_internal_pos);
    bx_object_free_value(&position->remote_parent_id);
    bx_object_free_value(&position->remote_is_optional);
    free(position);
}

static inline void _bx_object_position_de_free(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionDE * position = (BXObjectPositionDE *)data;

    bx_object_free_value(&position->remote_id);
    bx_object_free_value(&position->remote_text);
    bx_object_free_value(&position->remote_is_percentual);
    bx_object_free_value(&position->remote_value);
    bx_object_free_value(&position->remote_discount_total);
    free(position);
}

void bx_object_position_free(void * data)
{
    assert(data != NULL);
    enum e_BXObjectType type = *(enum e_BXObjectType *)data;

    switch(type) {
        case BXTypeInvoicePositionCE:
            _bx_object_position_ce_free(data);
            break;
        case BXTypeInvoicePositionAE:
            _bx_object_position_ae_free(data);
            break;
        case BXTypeInvoicePositionTE:
            _bx_object_position_te_free(data);
            break;
        case BXTypeInvoicePositionSE:
            _bx_object_position_se_free(data);
            break;
        case BXTypeInvoicePositionPE:
            _bx_object_position_pe_free(data);
            break;
        case BXTypeInvoicePositionDE:
            _bx_object_position_de_free(data);
            break;
        default:
            break;
    }
}
static inline void _bx_object_position_ce_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionCE * position = (BXObjectPositionCE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("unit_id", &position->remote_unit_id, 1);
    _bx_dump_any("account_id", &position->remote_account_id, 1);
    _bx_dump_any("tax_id", &position->remote_tax_id, 1);
    _bx_dump_any("internal_pos", &position->remote_internal_pos, 1);
    _bx_dump_any("parent_id", &position->remote_parent_id, 1);
    _bx_dump_any("unit_name", &position->remote_unit_name, 1);
    _bx_dump_any("text", &position->remote_text, 1);
    _bx_dump_any("pos", &position->remote_pos, 1);
    _bx_dump_any("is_optional", &position->remote_is_optional, 1);
    _bx_dump_any("amount", &position->remote_amount, 1);
    _bx_dump_any("tax_value", &position->remote_tax_value, 1);
    _bx_dump_any("unit_price", &position->remote_unit_price, 1);
    _bx_dump_any("discount_in_percent", &position->remote_discount_in_percent, 1);
    _bx_dump_any("position_total", &position->remote_position_total, 1);
}

static inline void _bx_object_position_ae_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionAE * position = (BXObjectPositionAE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("unit_id", &position->remote_unit_id, 1);
    _bx_dump_any("account_id", &position->remote_account_id, 1);
    _bx_dump_any("tax_id", &position->remote_tax_id, 1);
    _bx_dump_any("internal_pos", &position->remote_internal_pos, 1);
    _bx_dump_any("parent_id", &position->remote_parent_id, 1);
    _bx_dump_any("article_id", &position->remote_article_id, 1);
    _bx_dump_any("unit_name", &position->remote_unit_name, 1);
    _bx_dump_any("text", &position->remote_text, 1);
    _bx_dump_any("pos", &position->remote_pos, 1);
    _bx_dump_any("is_optional", &position->remote_is_optional, 1);
    _bx_dump_any("amount", &position->remote_amount, 1);
    _bx_dump_any("tax_value", &position->remote_tax_value, 1);
    _bx_dump_any("unit_price", &position->remote_unit_price, 1);
    _bx_dump_any("discount_in_percent", &position->remote_discount_in_percent, 1);
    _bx_dump_any("position_total", &position->remote_position_total, 1);
}

static inline void _bx_object_position_te_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionTE * position = (BXObjectPositionTE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("internal_pos", &position->remote_internal_pos, 1);
    _bx_dump_any("parent_id", &position->remote_parent_id, 1);
    _bx_dump_any("text", &position->remote_text, 1);
    _bx_dump_any("pos", &position->remote_pos, 1);
    _bx_dump_any("is_optional", &position->remote_is_optional, 1);
    _bx_dump_any("show_pos_nr", &position->remote_show_pos_nr, 1);
}

static inline void _bx_object_position_se_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionSE * position = (BXObjectPositionSE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("internal_pos", &position->remote_internal_pos, 1);
    _bx_dump_any("parent_id", &position->remote_parent_id, 1);
    _bx_dump_any("text", &position->remote_text, 1);
    _bx_dump_any("is_optional", &position->remote_is_optional, 1);
    _bx_dump_any("value", &position->remote_value, 1);
}

static inline void _bx_object_position_pe_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionPE * position = (BXObjectPositionPE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("internal_pos", &position->remote_internal_pos, 1);
    _bx_dump_any("parent_id", &position->remote_parent_id, 1);
    _bx_dump_any("is_optional", &position->remote_is_optional, 1);

}

static inline void _bx_object_position_de_dump(void * data) 
{
    if (data == NULL) { return; }
    BXObjectPositionDE * position = (BXObjectPositionDE *)data;

    _bx_dump_print_title("### DUMP ID:%lx CS:%lx ###", position->id, position->checksum);
    _bx_dump_any("id", &position->remote_id, 1);
    _bx_dump_any("text", &position->remote_text, 1);
    _bx_dump_any("is_percentual", &position->remote_is_percentual, 1);
    _bx_dump_any("value", &position->remote_value, 1);
    _bx_dump_any("discount_total", &position->remote_discount_total, 1);
}

void bx_object_position_dump(void * data)
{
    assert(data != NULL);
    enum e_BXObjectType type = *(enum e_BXObjectType *)data;
    switch(type) {
        case BXTypeInvoicePositionCE:
            _bx_object_position_ce_dump(data);
            break;
        case BXTypeInvoicePositionAE:
            _bx_object_position_ae_dump(data);
            break;
        case BXTypeInvoicePositionTE:
            _bx_object_position_te_dump(data);
            break;
        case BXTypeInvoicePositionSE:
            _bx_object_position_se_dump(data);
            break;
        case BXTypeInvoicePositionPE:
            _bx_object_position_pe_dump(data);
            break;
        case BXTypeInvoicePositionDE:
            _bx_object_position_de_dump(data);
            break;
        default:
            break;
    }
}