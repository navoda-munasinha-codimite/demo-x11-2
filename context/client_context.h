#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include <freerdp/freerdp.h>

typedef struct client_context
{
    rdpClientContext common; // Base context structure
    
    // Add any additional fields specific to the client context here
    HANDLE x11event;

} clientContext;


#endif // CLIENT_CONTEXT_H