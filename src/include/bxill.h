#ifndef BXILL_H__
#define BXILL_H__

#include <bx_net.h>
#include <mysql/mysql.h>

enum e_ObjectState {
    Error = 0,
    NeedUpdate,
    NeedCreate,
    NeedNothing
};
typedef enum e_ObjectState ObjectState;

typedef struct s_bXill bXill;
struct s_bXill {
    BXNet * net;
    BXNetRequestList * queue;
    MYSQL * mysql;
};

#endif /* BXILL_H__ */