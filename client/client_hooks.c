#include "../context/client_context.h"
#include "client_hooks.h"

#define TAG CLIENT_TAG("hooks-x11")

void terminateEventHandler(void* context, const TerminateEventArgs* e){
    rdpContext* ctx = (rdpContext*)context;
    WINPR_UNUSED(e);
    freerdp_abort_connect_context(ctx);
}

BOOL pre_connect(freerdp* instance){
    printf("pre_connect called\n");
    return TRUE;
}

BOOL post_connect(freerdp* instance){
    printf("post_connect called\n");
    return TRUE;
}

void post_disconnect(freerdp* instance){
    printf("post_disconnect called\n");
}

void post_final_disconnect(freerdp* instance)
{
    clientContext* clicon = NULL;
    rdpContext* context = NULL;

    if (!instance || !instance->context)
        return;

    context = instance->context;
    clicon = (clientContext*)context;

    // xf_keyboard_free(clicon);
    // xf_teardown_x11(clicon);
}

int logon_error_info(freerdp* instance, UINT32 data, UINT32 type){
    clientContext* clicon = (clientContext*)instance->context;
    const char* str_data = freerdp_get_logon_error_info_data(data);
    const char* str_type = freerdp_get_logon_error_info_type(type);
    WLog_INFO(TAG, "Logon Error Info %s [%s]", str_data, str_type);
    return 1;
}
