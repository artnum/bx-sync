#ifndef BX_OBJECT_TAX_H__
#define BX_OBJECT_TAX_H__

#include <bx_object.h>
#include <stdint.h>
#include <bx_object_value.h>

typedef struct s_BXObjectTax BXObjectTax;
struct s_BXObjectTax {
    enum e_BXObjectType type;

    uint64_t id;
    uint64_t checksum;

    BXFloat remote_percentage;
    BXFloat remote_value;
};

void * bx_object_tax_decode(void * jroot);
void bx_object_tax_dump(void * data);
void bx_object_tax_free(void * data);

#endif /* BX_OBJECT_TAX_H__ */