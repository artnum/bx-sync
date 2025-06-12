#ifndef COUNTRY_CODE_H__
#define COUNTRY_CODE_H__

#include "../bxill.h"

struct s_Country {
  int bx_id;
  char code[3];
};

typedef struct s_Country Country;
bool bx_country_code_load(bXill *app);
const char *bx_country_list_get_code(int id);
void bx_country_list_free(void);
void bx_country_list_dump(void);

#endif /* COUNTRY_CODE_H__ */
