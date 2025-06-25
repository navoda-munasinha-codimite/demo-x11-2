#ifndef CLIENT_HOOKS_H
#define CLIENT_HOOKS_H

#include <freerdp/freerdp.h>

void terminateEventHandler(void* context, const TerminateEventArgs* e);
BOOL pre_connect(freerdp* instance);
BOOL post_connect(freerdp* instance);
void post_disconnect(freerdp* instance);
void post_final_disconnect(freerdp* instance);
int logon_error_info(freerdp* instance, UINT32 data, UINT32 type);

#endif // CLIENT_HOOKS_H