#ifndef BXILL_H__
#define BXILL_H__

#include "bx_conf.h"
#include "bx_net.h"
#include <mysql/mysql.h>

#ifndef BXILL_LIST_LIMIT
    #define BXILL_LIST_LIMIT 500
#endif /* BXILL_LIST_LIMIT */ 
#ifndef BXILL_DEFAULT_DRIFT
    #define BXILL_DEFAULT_DRIFT 10
#endif /* BXILL_DEFAULT_DRIFT */
#ifndef BXILL_DEFAULT_CACHE_CHECKPOINT
    #define BXILL_DEFAULT_CACHE_CHECKPOINT 5
#endif /* BXILL_DEFAULT_CACHE_CHECKPOINT */
#ifndef BXILL_DEFAULT_CACHE_DIR 
    #define BXILL_DEFAULT_CACHE_DIR "/tmp/"
#endif /* BXILL_DEFAULT_CACHE_DIR */    
#ifndef BXILL_STANDBY_SECONDS
    #define BXILL_STANDBY_SECONDS 600 /* 10 min standby */
#endif /* BXILL_STANDBY_SECONDS */
#ifndef BXILL_THREAD_SLEEP_MS
    #define BXILL_THREAD_SLEEP_MS 250
#endif /* BXILL_THREAD_SLEEP_MS */
#ifndef BXILL_THREAD_EXIT_MAX_COUNT
    #define BXILL_THREAD_EXIT_MAX_COUNT 10
#endif /* BXILL_THREAD_EXIT_MAX_COUNT */ 

enum e_ObjectState { Error = 0, NeedUpdate, NeedCreate, NeedNothing };
typedef enum e_ObjectState ObjectState;

enum e_ThreadList {
  CONTACT_THREAD,
  PROJECT_THREAD,
  INVOICE_THREAD,
  CONTACT_SECTOR_THREAD,
  RANDOM_ITEM_THREAD,

  MAX__THREAD_LIST
};

typedef enum e_BXillError {
  NoError = 0,
  ErrorGeneric,
  ErrorSQLGeneric,
  ErrorSQLInsert,
  ErrorSQLUpdate,
  ErrorSQLSelect,
  ErrorSQLReconnect,
  ErrorNet,
  ErrorJSON,
  ErrorDuplicateEntry,
  Error_MAX__
} BXillError;

typedef struct s_bXill bXill;
struct s_bXill {
  atomic_bool logthread;
  BXNet *net;
  BXNetRequestList *queue;
  BXConf *conf;
};

#endif /* BXILL_H__ */
