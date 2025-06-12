#ifndef BX_OBJECT_GENERIC_H__
#define BX_OBJECT_GENERIC_H__

#include <bx_object_value.h>
struct s_BXGenericObject {
    char ** keys;
    BXGeneric * values;
    size_t value_count;
};

#endif /* BX_OBJECT_GENERIC_H__ */