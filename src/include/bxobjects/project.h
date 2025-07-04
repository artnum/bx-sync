#ifndef PROJECT_H__
#define PROJECT_H__

#include "../bx_ids_cache.h"
#include "../bx_object.h"
#include "../bx_object_value.h"
#include "../bxill.h"

typedef struct s_BXObjectProject BXObjectProject;
struct s_BXObjectProject {
  enum e_BXObjectType type;

  uint64_t checksum;

  BXUInteger id;
  BXUuid uuid;
  BXString nr;
  BXString name;
  BXString start_date;
  BXString end_date;
  BXString comment;
  BXUInteger pr_state_id;
  BXUInteger pr_project_type_id;
  BXUInteger contact_id;
  BXUInteger contact_sub_id;
  BXUInteger pr_invoice_type_id;
  BXFloat pr_invoice_type_amount;
  BXUInteger pr_budget_type_id;
  BXFloat pr_budget_type_amount;
};

BXillError bx_project_walk_item(bXill *app, MYSQL *conn, Cache *cache);
BXillError bx_project_sync_item(bXill *app, MYSQL *conn, BXGeneric *item,
                                Cache *cache);
bool bx_project_is_in_database(MYSQL *conn, BXGeneric *item);
#endif /* PROJECT_H__ */
