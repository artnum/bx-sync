#ifndef BXILL_H__
#define BXILL_H__

#include "bx_conf.h"
#include "bx_net.h"
#include <mysql/mysql.h>

#define BX_LIST_LIMIT 100

enum e_ObjectState { Error = 0, NeedUpdate, NeedCreate, NeedNothing };
typedef enum e_ObjectState ObjectState;

typedef struct s_bXill bXill;
struct s_bXill {
  atomic_bool logthread;
  BXNet *net;
  BXNetRequestList *queue;
  BXConf *conf;
};

#endif /* BXILL_H__ */
