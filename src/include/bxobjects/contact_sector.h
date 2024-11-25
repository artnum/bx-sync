#ifndef BX_OBJECT_CONTACT_SECTOR_H__
#define BX_OBJECT_CONTACT_SECTOR_H__

#include <bx_object_value.h>
#include <bxill.h>

typedef struct s_BXObjectContactSector BXObjectContactSector;
struct s_BXObjectContactSector {
    enum e_BXObjectType type;

    uint64_t checksum;

    BXInteger remote_id;
    BXString remote_name;
};
bool bx_contact_sector_walk_items(bXill * app);

#endif /* BX_OBJECT_CONTACT_SECTOR_H__ */