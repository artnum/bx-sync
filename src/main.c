#include <bx_conf.h>
#include <bx_net.h>
#include <bx_decode.h>
#include <bx_object.h>
#include <bxobjects/invoice.h>
#include <bx_utils.h>

#include <jansson.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) 
{
    BXConf * conf = NULL;

    bx_utils_init();

    conf = bx_conf_init();
    if (!bx_conf_loadfile(conf, "conf.json")) {
        bx_conf_destroy(&conf);
        return -1;
    }
    bx_conf_dump(conf);

    BXNet * net = bx_net_init(conf);
    if (net == NULL) {
        fprintf(stderr, "Net configuration failed");
        exit(0);
    }
    BXNetRequestList * queue = bx_net_request_list_init(net);
    pthread_t request_thread = bx_net_loop(queue);
    
    BXNetRequest * request = NULL;
    int c = 0;
    do {
        request = bx_net_request_new(BXTypeInvoice, "2.0", "kb_invoice", NULL);

        bx_net_request_add_param(request, "limit", "100");
        bx_net_request_add_param(request, "offset", "0");

        bx_net_request_list_add(queue, request);

        /*request = bx_net_request_new(BXTypeContact, "2.0", "contact", NULL);
        bx_net_request_list_add(queue, request);*/

        request = bx_net_request_list_remove(queue, true);
        if (request != NULL) {
            if (request->response != NULL) {
                if (request->response->data != NULL) {
                    json_t * jroot = bx_decode_net(request);
                    free(request->response->data);
                    free(request->response);
                    request->response = NULL;
                    if (jroot != NULL) {
                        request->decoded = jroot;
                        const BXObjectFunctions * decoder = bx_decode_select_decoder(request->decoder);
                        void * object = NULL;
                        if (decoder->decode_function == NULL) {
                            fprintf(stderr, "No decoder found for '%d'\n", request->decoder);

                        } else {
                            if (json_is_array(request->decoded)) {
                                for (size_t i = 0; i < json_array_size(jroot); i++) {
                                    object = decoder->decode_function(json_array_get(jroot, i));
                                    decoder = bx_decode_select_decoder(*(enum e_BXObjectType *)object);
                                    if (decoder->dump_function != NULL) { decoder->dump_function(object); }
                                    if (decoder->free_function != NULL) { decoder->free_function(object); }
                                }
                            } else {
                                object = decoder->decode_function(jroot);
                                decoder = bx_decode_select_decoder(*(enum e_BXObjectType *)object);
                                if (decoder->dump_function != NULL) { decoder->dump_function(object); }
                                if (decoder->free_function != NULL) { decoder->free_function(object); }
                            }
                        }
                        json_decref(request->decoded);
                    } else {
                        fprintf(stderr, "Failed json decode\n");
                    }
                    
                }
            }
            
        }
        usleep(1000000);
    } while (c++ < 10);
    atomic_store(&queue->run, 0);
    pthread_join(request_thread, NULL);
    bx_net_request_list_destroy(queue);
    bx_net_destroy(&net);
    bx_conf_destroy(&conf);
    return 0;
}