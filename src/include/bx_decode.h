#ifndef BX_DECODE_H__
#define BX_DECODE_H__

#include "bx_net.h"
#include "bx_object.h"
#include <jansson.h>

#include "bx_object_functions.h"

const BXObjectFunctions *bx_decode_select_decoder(enum e_BXObjectType decoder);
json_t *bx_decode_net(BXNetRequest *request);

#endif /* BX_DECODE_H__ */
