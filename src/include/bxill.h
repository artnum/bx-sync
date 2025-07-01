#ifndef BXILL_H__
#define BXILL_H__

#include "bx_conf.h"
#include "bx_net.h"
#include <mysql/mysql.h>

#define BX_LIST_LIMIT 500
#define BXILL_DEFAULT_DRIFT 10
#define BXILL_DEFAULT_CACHE_CHECKPOINT 5
#define BXILL_DEFAULT_CACHE_DIR "/tmp/"
#define BXILL_STANDBY_SECONDS 600 /* 10 min standby */
#define BXILL_THREAD_SLEEP_MS 250
#define BXILL_THREAD_EXIT_MAX_COUNT 10
enum e_ObjectState { Error = 0, NeedUpdate, NeedCreate, NeedNothing };
typedef enum e_ObjectState ObjectState;

typedef enum e_BXillError {
  NoError = 0,
  ErrorGeneric,
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
