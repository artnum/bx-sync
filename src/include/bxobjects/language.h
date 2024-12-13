#ifndef LANGUAGE_H__
#define LANGUAGE_H__

#include <bx_object_value.h>
#include <bxill.h>

typedef struct s_BXObjectLanguage BXObjectLanguage;

struct s_BXObjectLanguage {
    enum e_BXObjectType type;

    uint64_t checksum;

    BXUInteger id;
    BXUInteger date_format_id;
    BXString name;
    BXString decimal_point;
    BXString thousands_separator;
    BXString date_format;
    BXString iso_639_1;
};

void bx_language_free(BXObjectLanguage * language);
bool bx_language_update_db(bXill * app, BXObjectLanguage * language);
bool bx_language_insert_db(bXill * app, BXObjectLanguage * language);
bool bx_language_load (bXill * app);

#endif /* LANGUAGE_H__ */