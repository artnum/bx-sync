#ifndef BX_DECODE_H__
#define BX_DECODE_H__

#include <jansson.h>
#include <bx_net.h>
#include <bx_object.h>


const BXObjectFunctions * bx_decode_select_decoder(const char * decoder);
json_t * bx_decode_net(BXNetRequest * request);

#endif /* BX_DECODE_H__ */