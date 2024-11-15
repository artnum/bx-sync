#ifndef BX_CONF_H__
#define BX_CONF_H__

#include <stdint.h>
#include <bx_mutex.h>

typedef struct s_BXConf BXConf;
typedef struct s_BXConfValue BXConfValue;

enum e_BXConfValueType {
    StringType = 1,
    ByteType,
    IntegerType,
    FloatType,
    BoolType
};

struct s_BXConfValue {
    uint64_t key;
    enum e_BXConfValueType type;
    char * s_value;
    int i_value;
    double f_value;
    BXConfValue * next;
    BXMutex hold;
};

struct s_BXConf {
    BXMutex mutex;
    BXConfValue * head;
};

BXConf * bx_conf_init();
void bx_conf_destroy(BXConf ** conf);
void bx_conf_dump(BXConf * conf);
bool bx_conf_set(BXConf * conf, const char * key, void * value, enum e_BXConfValueType type);
void bx_conf_release(BXConf * conf, const char * key);
const char * bx_conf_get_string(BXConf * conf, const char * key);
bool bx_conf_loadfile(BXConf * conf, const char * filepath);

#endif /* BX_CONF_H__ */