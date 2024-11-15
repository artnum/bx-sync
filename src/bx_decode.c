#include <bx_decode.h>

const BXObjectFunctions * bx_decode_select_decoder(const char * decoder)
{
    assert(decoder != NULL);

    for (int i = 0; FunctionHandlers[i].name != NULL; i++) {
        if (strcmp(FunctionHandlers[i].name, decoder) == 0) {
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