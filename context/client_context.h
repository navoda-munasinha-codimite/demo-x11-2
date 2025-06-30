#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include <freerdp/freerdp.h>
#include <freerdp/locale/keyboard.h>

typedef struct client_context
{
    rdpClientContext common; // Base context structure
    
    // Add any additional fields specific to the client context here


} clientContext;


#endif // CLIENT_CONTEXT_H