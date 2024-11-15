#include "bx_object.h"
#include <bx_decode.h>

const BXObjectFunctions * bx_decode_select_decoder(enum e_BXObjectType decoder)
{
    assert(decoder != BXTypeNone);

    for (int i = 0; FunctionHandlers[i].type != BXTypeNone; i++) {
        if (FunctionHandlers[i].type == decoder) {
            return &FunctionHandlers[i];
        }
    }

    return NULL;
}

json_t * bx_decode_net(BXNetRequest * request)
{
    assert(request != NULL);
    assert(request->response != NULL);
    assert(request->response->data_len > 0);
    assert(request->response->data != NULL);

    json_error_t error;
    json_t * jroot = json_loadb(
        request->response->data, 
        request->response->data_len,
        0,
        &error
    );

    return jroot;
}