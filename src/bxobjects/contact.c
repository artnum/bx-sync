#include "../include/bxobjects/contact.h"
#include "../include/bx_database.h"
#include "../include/bx_net.h"
#include "../include/bx_object.h"
#include "../include/bx_object_value.h"
#include "../include/bx_utils.h"
#include "../include/bxill.h"
#include "../include/bxobjects/country_code.h"
#include "../include/bxobjects/user.h"
#include <jansson.h>
#include <mysql/mysql.h>
#include <stddef.h>
#include <threads.h>
#include <unistd.h>

#define QUERY_UPDATE                                                           \
  "UPDATE contact SET contact_type_id = :contact_type_id,"                     \
  "salutation_id = :salutation_id, country = :country,"                        \
  "user_id = :user_id, owner_id = :owner_id, title_id = :title_id,"            \
  "salutation_form = :salutation_form, postcode = :postcode, nr = :nr,"        \
  "name_1 = :name_1, name_2 = :name_2, birthday = :birthday, address = "       \
  ":address"                                                                   \
  "city = :city, mail = :mail, mail_second = :mail_second,"                    \
  "phone_fixed = :phone_fixed, phone_fixed_second = :phone_fixed_second,"      \
  "phone_mobile = :phone_mobile, fax = :fax, url = :url, skype_name = "        \
  ":skype_name,"                                                               \
  "remarks = :remarks, updated_at = :updated_at, profile_image = "             \
  ":profile_image,"                                                            \
  "language_id = :language_id"                                                 \
  "_checksum = :_checksum, _last_updated = :_last_updated, _archived = "       \
  ":archived"                                                                  \
  " WHERE id = :id;"
#define QUERY_INSERT                                                           \
  "INSERT IGNORE INTO contact (id, contact_type_id, salutation_id, country,"   \
  "user_id, owner_id, title_id, salutation_form, postcode, nr, name_1, "       \
  "name_2,"                                                                    \
  "birthday, address, city, mail, mail_second, phone_fixed, "                  \
  "phone_fixed_second,"                                                        \
  "phone_mobile, fax, url, skype_name, remarks, updated_at, profile_image, "   \
  "language_id,"                                                               \
  "_checksum, _last_updated, _archived"                                        \
  ") VALUES (:id, :contact_type_id, :salutation_id, :country, :user_id, "      \
  ":owner_id,"                                                                 \
  ":title_id, :salutation_form, :postcode, :nr, :name_1, name_2, :birthday, "  \
  ":address,"                                                                  \
  ":city, :mail, :mail_second, :phone_fixed, :phone_fixed_second, "            \
  ":phone_mobile,"                                                             \
  ":fax, :url, :skype_name, :remarks, :updated_at, :profile_image, "           \
  ":language_id, "                                                             \
  ":_checksum, :_last_updated, :_archived"                                     \
  ");"

void bx_object_contact_free(void *data) {
  BXObjectContact *contact = (BXObjectContact *)data;
  if (contact == NULL) {
    return;
  }

  bx_object_free_value(&contact->postcode);
  bx_object_free_value(&contact->nr);
  bx_object_free_value(&contact->name_1);
  bx_object_free_value(&contact->name_2);
  bx_object_free_value(&contact->birthday);
  bx_object_free_value(&contact->address);
  bx_object_free_value(&contact->city);
  bx_object_free_value(&contact->mail);
  bx_object_free_value(&contact->mail_second);
  bx_object_free_value(&contact->phone_fixed);
  bx_object_free_value(&contact->phone_fixed_second);
  bx_object_free_value(&contact->phone_mobile);
  bx_object_free_value(&contact->fax);
  bx_object_free_value(&contact->url);
  bx_object_free_value(&contact->skype_name);
  bx_object_free_value(&contact->remarks);
  bx_object_free_value(&contact->language_id);
  bx_object_free_value(&contact->contact_groupd_ids);
  bx_object_free_value(&contact->contact_branch_ids);
  bx_object_free_value(&contact->updated_at);
  bx_object_free_value(&contact->profile_image);

  free(contact);
}

void bx_object_contact_store(MYSQL *mysql, BXObjectContact *contact) {
  BXDatabaseQuery *query = bx_database_new_query(
      mysql,
      "INSERT INTO contact (id, user_id, contact_type_id, country_id, "
      "owner_id, title_id, salutation_form, postcode, nr, name_1, name_2, "
      "address, birthday, updated_at, city, mail, mail_second, phone_fixed, "
      "phone_fixed_second, phone_mobile, phone_fax, url, skype_name, remarks, "
      "lanuage_id, contact_group_ids, branch_ids, profile_image, "
      "_checksum, _last_updated"
      "VALUES(:id, :user_id, :contact_type_id, :country_id, "
      ":owner_id, :title_id, :salutation_form, :postcode, :nr, :name_1, "
      ":name_2, "
      ":address, :birthday, :updated_at, :city, :mail, :mail_second, "
      ":phone_fixed, "
      ":phone_fixed_second, :phone_mobile, :phone_fax, url, :skype_name, "
      ":remarks, "
      ":lanuage_id, :contact_group_ids, :branch_ids, :profile_image, "
      ":_checksum, :_last_updated)");

  bx_database_add_param_int32(query, ":id", &contact->id);
  bx_database_add_param_int32(query, ":user_id", &contact->user_id);
  bx_database_add_param_int32(query, ":contact_type_id",
                              &contact->contact_type_id);
  bx_database_add_param_int32(query, ":salutation_id", &contact->salutation_id);
  bx_database_add_param_int32(query, ":owner_id", &contact->owner_id);
  bx_database_add_param_int32(query, ":title_id", &contact->title_id);
  bx_database_add_param_int32(query, ":salutation_form",
                              &contact->salutation_form);

  int64_t *contact_group_ids =
      bx_int_string_array_to_int_array(contact->contact_groupd_ids.value);
  if (contact_group_ids != NULL) {

    for (int i = 1; i <= contact_group_ids[0]; i++) {
      BXDatabaseQuery *select_cgi = bx_database_new_query(
          mysql, "SELECT group_id,contact_id FROM contact_group_to_contact_id "
                 "WHERE group_id = :gid AND contact_id = :cid;");
      bx_database_add_param_uint32(select_cgi, ":gid", &contact_group_ids[i]);
      bx_database_add_param_uint32(select_cgi, ":cid", &contact->id.value);
      bx_database_execute(select_cgi);
      bx_database_results(select_cgi);

      if (select_cgi->results == NULL ||
          select_cgi->results->column_count <= 0) {
        BXDatabaseQuery *insert_cgi = bx_database_new_query(
            mysql,
            "INSERT INTO contact_group_to_contact_id (group_id, contact_id) "
            "VALUES(:gid, :cid);");
        if (select_cgi == NULL) {
          /* TODO handle failure */
        }
        bx_database_add_param_uint32(insert_cgi, ":gid", &contact_group_ids[i]);
        bx_database_add_param_uint32(insert_cgi, ":cid", &contact->id.value);
        bx_database_execute(insert_cgi);
        bx_database_free_query(insert_cgi);
      }
      bx_database_free_query(select_cgi);
    }
    free(contact_group_ids);
  }

  bx_database_free_query(query);
}

void *bx_object_contact_decode(void *jroot) {
  json_t *object = (json_t *)jroot;
  BXObjectContact *contact = NULL;
  XXH3_state_t *hashState = XXH3_createState();
  if (hashState == NULL) {
    return NULL;
  }
  XXH3_64bits_reset(hashState);
  contact = calloc(1, sizeof(*contact));
  if (contact == NULL) {
    return NULL;
  }
  contact->type = BXTypeContact;

  /* integer */
  contact->id = bx_object_get_json_uint(object, "id", hashState);
  contact->user_id = bx_object_get_json_uint(object, "user_id", hashState);
  contact->contact_type_id =
      bx_object_get_json_uint(object, "contact_type_id", hashState);
  contact->salutation_id =
      bx_object_get_json_uint(object, "salutation_id", hashState);
  contact->country_id =
      bx_object_get_json_uint(object, "country_id", hashState);
  contact->owner_id = bx_object_get_json_uint(object, "owner_id", hashState);
  contact->title_id = bx_object_get_json_uint(object, "title_id", hashState);
  contact->salutation_form =
      bx_object_get_json_uint(object, "salutation_form", hashState);
  contact->language_id =
      bx_object_get_json_uint(object, "language_id", hashState);

  /* string */
  contact->postcode = bx_object_get_json_string(object, "postcode", hashState);
  contact->nr = bx_object_get_json_string(object, "nr", hashState);
  contact->name_1 = bx_object_get_json_string(object, "name_1", hashState);
  contact->name_2 = bx_object_get_json_string(object, "name_2", hashState);
  contact->address = bx_object_get_json_string(object, "address", hashState);
  contact->birthday = bx_object_get_json_string(object, "birthday", hashState);
  contact->updated_at =
      bx_object_get_json_string(object, "updated_at", hashState);
  contact->city = bx_object_get_json_string(object, "city", hashState);
  contact->mail = bx_object_get_json_string(object, "mail", hashState);
  contact->mail_second =
      bx_object_get_json_string(object, "mail_second", hashState);
  contact->phone_fixed =
      bx_object_get_json_string(object, "phone_fixed", hashState);
  contact->phone_fixed_second =
      bx_object_get_json_string(object, "phone_fixed_second", hashState);
  contact->phone_mobile =
      bx_object_get_json_string(object, "phone_mobile", hashState);
  contact->fax = bx_object_get_json_string(object, "fax", hashState);
  contact->url = bx_object_get_json_string(object, "url", hashState);
  contact->skype_name =
      bx_object_get_json_string(object, "skype_name", hashState);
  contact->remarks = bx_object_get_json_string(object, "remarks", hashState);
  contact->contact_groupd_ids =
      bx_object_get_json_string(object, "contact_group_ids", hashState);
  contact->contact_branch_ids =
      bx_object_get_json_string(object, "contact_branch_ids", hashState);
  contact->profile_image =
      bx_object_get_json_string(object, "profile_image", hashState);

  contact->checksum = XXH3_64bits_digest(hashState);
  XXH3_freeState(hashState);
  return contact;
}

void bx_object_contact_dump(void *data) {
  BXObjectContact *contact = (BXObjectContact *)data;
  if (contact == NULL) {
    return;
  }

  _bx_dump_print_title("### DUMP CONTACT '%s' CS:%lx ###", contact->nr.value,
                       contact->checksum);
  _bx_dump_any("id", &contact->id, 1);
  _bx_dump_any("user_id", &contact->user_id, 1);
  _bx_dump_any("contact_type_id", &contact->contact_type_id, 1);
  _bx_dump_any("salutation_id", &contact->salutation_id, 1);
  _bx_dump_any("country_id", &contact->country_id, 1);
  _bx_dump_any("owner_id", &contact->owner_id, 1);
  _bx_dump_any("title_id", &contact->title_id, 1);
  _bx_dump_any("salutation_form", &contact->salutation_form, 1);
  _bx_dump_any("postcode", &contact->postcode, 1);
  _bx_dump_any("nr", &contact->nr, 1);
  _bx_dump_any("name_1", &contact->name_1, 1);
  _bx_dump_any("name_2", &contact->name_2, 1);
  _bx_dump_any("address", &contact->address, 1);
  _bx_dump_any("birthday", &contact->birthday, 1);
  _bx_dump_any("updated_at", &contact->updated_at, 1);
  _bx_dump_any("city", &contact->city, 1);
  _bx_dump_any("mail", &contact->mail, 1);
  _bx_dump_any("mail_second", &contact->mail_second, 1);
  _bx_dump_any("phone_fixed", &contact->phone_fixed, 1);
  _bx_dump_any("phone_fixed_second", &contact->phone_fixed_second, 1);
  _bx_dump_any("phone_mobile", &contact->phone_mobile, 1);
  _bx_dump_any("fax", &contact->fax, 1);
  _bx_dump_any("url", &contact->url, 1);
  _bx_dump_any("skype_name", &contact->skype_name, 1);
  _bx_dump_any("remarks", &contact->remarks, 1);
  _bx_dump_any("language_id", &contact->language_id, 1);
  _bx_dump_any("contact_group_ids", &contact->contact_groupd_ids, 1);
  _bx_dump_any("contact_branch_ids", &contact->contact_branch_ids, 1);
  _bx_dump_any("profile_image", &contact->profile_image, 1);
}

#if 0 /* Silence compiler, not used yet but will be */
static void contact_group_sync(bXill *app, MYSQL *conn,
                               BXObjectContact *contact) {
  BXDatabaseQuery *query = NULL;
  uint64_t *group_ids = (uint64_t *)bx_int_string_array_to_int_array(
      contact->contact_groupd_ids.value);
  if (group_ids == NULL) {
    return;
  }
  BXInteger item;
  item.isset = true;
  item.type = BX_OBJECT_TYPE_INTEGER;
  for (int i = 0; i <= *group_ids; i++) {
    item.value = *(group_ids + i);
    if (!bx_contact_group_sync_item(app, conn, (BXGeneric *)&item)) {
      continue;
    }
    query =
        bx_database_new_query(conn, "SELECT contact_group FROM cg2c WHERE "
                                    "contact = :cid AND contact_group = :cgid");
    if (query == NULL) {
      continue;
    }
    bx_database_add_param_uint64(query, ":cid", &contact->id.value);
    bx_database_add_param_uint64(query, ":cgid", &item.value);
    bx_database_execute(query);
    bx_database_results(query);
    if (query->results == NULL || query->row_count <= 0) {
      bx_database_free_query(query);
      query = NULL;
      query = bx_database_new_query(conn,
                                    "INSERT INTO cg2c (contact, contact_group)"
                                    " VALUES (:cid, :cgid)");
      if (query == NULL) {
        continue;
      }
      bx_database_add_param_uint64(query, ":cid", &contact->id.value);
      bx_database_add_param_uint64(query, ":cgid", &item.value);
      bx_database_execute(query);
    }
    /* query is allocated*/
    bx_database_free_query(query);
  }

  free(group_ids);
}
#endif

bool bx_contact_is_in_database(MYSQL *conn, BXGeneric *item) {
  BXDatabaseQuery *query =
      bx_database_new_query(conn, "SELECT id FROM contact WHERE id = :id;");
  if (query == NULL) {
    return false;
  }
  bx_database_add_bxtype(query, ":id", item);
  if (!bx_database_execute(query) || !bx_database_results(query)) {
    bx_database_free_query(query);
    return false;
  }

  if (query->results == NULL || query->results->column_count == 0) {
    bx_database_free_query(query);
    return false;
  }

  bx_database_free_query(query);
  return true;
}

BXillError _bx_contact_sync_item(bXill *app, MYSQL *conn, json_t *item,
                                 BXBool is_archived, Cache *c) {
  assert(app != NULL);
  assert(item != NULL);

  BXObjectContact *contact = bx_object_contact_decode(item);
  if (contact == NULL) {
    return ErrorGeneric;
  }

  CacheState state =
      cache_check_item(c, (BXGeneric *)&contact->id, contact->checksum);
  if (state == CacheOk) {
    bx_object_contact_free(contact);
    return NoError;
  }
  BXDatabaseQuery *query = NULL;
  if (state == CacheNotSet) {
    query = bx_database_new_query(conn, QUERY_INSERT);
  } else if (state == CacheNotSync) {
    bx_log_debug("Update contact");
    query = bx_database_new_query(conn, QUERY_UPDATE);
  } else {
    bx_object_contact_free(contact);
    return ErrorGeneric;
  }
  if (query == NULL) {
    bx_object_contact_free(contact);
    return ErrorGeneric;
  }

  if (!bx_user_is_in_database(conn, (BXGeneric *)&contact->user_id)) {
    bx_user_sync_item(app, conn, (BXGeneric *)&contact->user_id);
  }
  if (contact->user_id.value != contact->owner_id.value &&
      !bx_user_is_in_database(conn, (BXGeneric *)&contact->owner_id)) {
    bx_user_sync_item(app, conn, (BXGeneric *)&contact->owner_id);
  }

  uint64_t now = time(NULL);
  bx_database_add_bxtype(query, ":id", (BXGeneric *)&contact->id);
  bx_database_add_bxtype(query, ":contact_type_id",
                         (BXGeneric *)&contact->contact_type_id);
  bx_database_add_bxtype(query, ":salutation_id",
                         (BXGeneric *)&contact->salutation_id);
  bx_database_add_bxtype(query, ":user_id", (BXGeneric *)&contact->user_id);
  bx_database_add_bxtype(query, ":owner_id", (BXGeneric *)&contact->owner_id);
  bx_database_add_bxtype(query, ":title_id", (BXGeneric *)&contact->title_id);
  bx_database_add_bxtype(query, ":salutation_form",
                         (BXGeneric *)&contact->salutation_form);
  bx_database_add_bxtype(query, ":language_id",
                         (BXGeneric *)&contact->language_id);
  bx_database_add_bxtype(query, ":postcode", (BXGeneric *)&contact->postcode);
  bx_database_add_bxtype(query, ":nr", (BXGeneric *)&contact->nr);
  bx_database_add_bxtype(query, ":name_1", (BXGeneric *)&contact->name_1);
  bx_database_add_bxtype(query, ":name_2", (BXGeneric *)&contact->name_2);
  bx_database_add_bxtype(query, ":birthday", (BXGeneric *)&contact->birthday);
  bx_database_add_bxtype(query, ":address", (BXGeneric *)&contact->address);
  bx_database_add_bxtype(query, ":city", (BXGeneric *)&contact->city);
  bx_database_add_bxtype(query, ":mail_second",
                         (BXGeneric *)&contact->mail_second);
  bx_database_add_bxtype(query, ":phone_fixed_second",
                         (BXGeneric *)&contact->phone_fixed_second);
  bx_database_add_bxtype(query, ":phone_fixed",
                         (BXGeneric *)&contact->phone_fixed);
  bx_database_add_bxtype(query, ":phone_mobile",
                         (BXGeneric *)&contact->phone_mobile);
  bx_database_add_bxtype(query, ":fax", (BXGeneric *)&contact->fax);
  bx_database_add_bxtype(query, ":url", (BXGeneric *)&contact->url);
  bx_database_add_bxtype(query, ":skype_name",
                         (BXGeneric *)&contact->skype_name);
  bx_database_add_bxtype(query, ":remarks", (BXGeneric *)&contact->remarks);
  bx_database_add_bxtype(query, ":updated_at",
                         (BXGeneric *)&contact->updated_at);
  bx_database_add_bxtype(query, ":profile_image",
                         (BXGeneric *)&contact->profile_image);

  bx_database_add_bxtype(query, ":_archived", (BXGeneric *)&is_archived);
  bx_database_add_param_uint64(query, ":_checksum", &contact->checksum);
  bx_database_add_param_uint64(query, ":_last_updated", &now);
  bx_database_add_param_char(
      query, ":country",
      (void *)bx_country_list_get_code(contact->country_id.value), 2);

  if (!bx_database_execute(query) || !bx_database_results(query)) {
    BXillError e = query->need_reconnect ? ErrorSQLReconnect : ErrorGeneric;
    bx_database_free_query(query);
    return e;
  }
  if (query->warning_rows == 0 && !query->has_failed) {
    cache_set_item(c, (BXGeneric *)&contact->id, contact->checksum);
  }
  bx_database_free_query(query);
  query = NULL;
  /*
    contact_group_sync(app, conn, contact);

    uint64_t *branch_ids = (uint64_t *)bx_int_string_array_to_int_array(
        contact->contact_branch_ids.value);
    if (branch_ids != NULL) {
      free(branch_ids);
    }
  */
  bx_object_contact_free(contact);

  return NoError;
}

#define GET_CONTACT_PATH "2.0/contact/$?show_archived=$"
BXillError bx_contact_sync_item(bXill *app, MYSQL *conn, BXGeneric *item,
                                BXBool show_archived, Cache *c) {
  BXNetRequest *request =
      bx_do_request(app->queue, NULL, GET_CONTACT_PATH, item, &show_archived);
  if (request == NULL) {
    return false;
  }
  if (request->response == NULL || request->response->http_code != 200) {
    bx_net_request_free(request);
    return false;
  }
  bool retVal =
      _bx_contact_sync_item(app, conn, request->decoded, show_archived, c);
  bx_net_request_free(request);
  return retVal;
}

#define WALK_CONTACT_PATH "2.0/contact?limit=$&offset=$&show_archived=$"
BXillError bx_contact_walk_items(bXill *app, MYSQL *conn, Cache *c) {
  bx_log_debug("BX Walk Contact Items");
  BXInteger offset = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = 0};
  const BXInteger limit = {
      .type = BX_OBJECT_TYPE_INTEGER, .isset = true, .value = BX_LIST_LIMIT};
  BXBool show_archived = {
      .type = BX_OBJECT_TYPE_BOOL, .isset = true, .value = false};

  size_t arr_len = 0;
  for (int i = 0; i < 2; i++) {
    do {
      arr_len = 0;
      BXNetRequest *request = bx_do_request(app->queue, NULL, WALK_CONTACT_PATH,
                                            &limit, &offset, &show_archived);
      if (request == NULL) {
        return ErrorGeneric;
      }
      if (!json_is_array(request->decoded)) {
        bx_net_request_free(request);
        return ErrorGeneric;
      }

      arr_len = json_array_size(request->decoded);
      for (size_t j = 0; j < arr_len; j++) {
        BXillError e = _bx_contact_sync_item(
            app, conn, json_array_get(request->decoded, j), show_archived, c);
        if (e == ErrorSQLReconnect) {
          bx_net_request_free(request);
          return e;
        }
      }
      bx_net_request_free(request);
      offset.value += limit.value;
    } while (arr_len > 0);
    /* alternate between archived and not archived, show_archived trigger "show
     * only archived users" */
    show_archived.value = true;
    offset.value = 0;
  }
  return NoError;
}
