#ifndef BXILL_H__
#define BXILL_H__

#include "bx_conf.h"
#include "bx_net.h"
#include "index.h"
#include <mysql/mysql.h>

#define BXILL_USER_CACHE 0
#define BXILL_CONTACT_CACHE 1
#define BXILL_PROJECT_CACHE 2
#define BXILL_BULK_PROJECT_CACHE 3

#define BX_LIST_LIMIT 500
#define BXILL_DEFAULT_DRIFT 10
#define BXILL_DEFAULT_CACHE_CHECKPOINT 5
#define BXILL_DEFAULT_CACHE_DIR "/tmp/"
#define BXILL_STANDBY_SECONDS 600 /* 10 min standby */
#define BXILL_THREAD_SLEEP_MS 2500
#define BXILL_THREAD_EXIT_MAX_COUNT 10
#define BXILL_INDEX_MAX_SIZE                                                   \
  50000 /* 50000 indexes, if you cross this line, maybe another solution       \
           should be investigated */

enum e_ObjectState { Error = 0, NeedUpdate, NeedCreate, NeedNothing };
typedef enum e_ObjectState ObjectState;

enum e_ThreadList {
  CONTACT_THREAD,
  PROJECT_THREAD,
  INVOICE_THREAD,
  RANDOM_ITEM_THREAD,

  MAX__THREAD_LIST
};

typedef enum e_BXillError {
  NoError = 0,
  ErrorGeneric,
  ErrorSQLReconnect,
  ErrorNet,
  ErrorJSON,
  ErrorDuplicateEntry,
  ErrorMissingForeignEntry,

  Error_MAX__
} BXillError;

typedef enum { BXRetval_generic, BXRetVal_checksum } BXRetVal_t;

typedef struct {
  BXRetVal_t type;
  bool success;
} BXRetVal;

typedef struct {
  BXRetVal r;
  uint64_t data;
} BXRetValChecksum;

typedef struct s_bXill bXill;
struct s_bXill {
  atomic_bool logthread;
  BXNet *net;
  BXNetRequestList *queue;
  BXConf *conf;
  Indexes indexes;
};

#endif /* BXILL_H__ */
