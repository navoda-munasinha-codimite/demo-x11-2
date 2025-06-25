# Initial FreeRDP Investigation Findings

## Overview
During the initial exploration of FreeRDP, I discovered several key architectural patterns that enable custom RDP client implementation.

## Key Discoveries

### 1. Custom Context Architecture
FreeRDP supports custom context structures that extend the base `rdpClientContext`. This allows for custom workflow implementation and state management.

- **File**: `client_context.h`
- **Purpose**: Defines a custom `clientContext` structure that extends `rdpClientContext`
- **Benefits**: Enables application-specific data storage and custom event handling

**Code Implementation:**
```c
typedef struct client_context
{
    rdpClientContext common; // Base context structure
    
    // Add any additional fields specific to the client context here
    HANDLE x11event;

} clientContext;
```

### 2. Entry Points System
FreeRDP uses an entry points system (`RDP_CLIENT_ENTRY_POINTS`) to instantiate and configure the RDP context.

- **Implementation**: `RdpClientEntry` function in `client.c`
- **Instantiation**: Uses `freerdp_client_context_new` function to create the context
- **Configuration**: Sets up callback functions for various RDP lifecycle events

**Code Implementation:**
```c
int RdpClientEntry(RDP_CLIENT_ENTRY_POINTS* pEntryPoints)
{
    pEntryPoints->GlobalInit = client_global_init;
    pEntryPoints->GlobalUninit = client_global_uninit;
    pEntryPoints->ContextSize = sizeof(clientContext);
    pEntryPoints->ClientNew = client_new;
    pEntryPoints->ClientFree = client_free;
    pEntryPoints->ClientStart = client_start;
    pEntryPoints->ClientStop = freerdp_client_common_stop;
    return 0;
}
```

### 3. Callback Functions
The entry points allow configuration of callback functions for different phases of the RDP connection:

**Configured in `client_new` callback:**
- `PreConnect`: Called before establishing connection
- `PostConnect`: Called after successful connection
- `PostDisconnect`: Called after disconnection
- `PostFinalDisconnect`: Called for final cleanup
- `LogonErrorInfo`: Handles logon error information
- `GetAccessToken`: Handles access token retrieval

**Code Implementation:**
```c
static BOOL client_new(freerdp* instance, rdpContext* context){
    clientContext* clicon = (clientContext*)instance->context;
    WINPR_ASSERT(context);
    WINPR_ASSERT(clicon);
    
    // Configure callback functions
    instance->PreConnect = pre_connect;
    instance->PostConnect = post_connect;
    instance->PostDisconnect = post_disconnect;
    instance->PostFinalDisconnect = post_final_disconnect;
    instance->LogonErrorInfo = logon_error_info;
    instance->GetAccessToken = client_cli_get_access_token;
    
    // Subscribe to terminate events
    PubSub_SubscribeTerminate(context->pubSub, terminateEventHandler);
    return TRUE;
}
```

### 4. Hook System
Created a separate hook system for organizing callback implementations:

- **File**: `client_hooks.c`
- **Purpose**: Contains dummy implementations of all callback functions
- **Structure**: Provides a clean separation of concerns for event handling

**Code Implementation (Dummy Functions):**
```c
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

void terminateEventHandler(void* context, const TerminateEventArgs* e){
    rdpContext* ctx = (rdpContext*)context;
    WINPR_UNUSED(e);
    freerdp_abort_connect_context(ctx);
}
```

### 5. Context Initialization
The client context is properly initialized and managed through the lifecycle:

**Context Creation:**
```c
static int client_start(rdpContext* context){
    clientContext* clicon = (clientContext*)context;
    rdpSettings* settings = context->settings;

    if (!freerdp_settings_get_string(settings, FreeRDP_ServerHostname))
    {
        WLog_ERR(TAG, "error: server hostname was not specified with /v:<server>[:port]");
        return -1;
    }

    if (!(clicon->common.thread = CreateThread(NULL, 0, client_thread, context->instance, 0, NULL)))
    {
        WLog_ERR(TAG, "failed to create client thread");
        return -1;
    }

    return 0;
}
```

**Context Cleanup:**
```c
static void client_free(WINPR_ATTR_UNUSED freerdp* instance, rdpContext* context){
    if (!context)
        return;

    if (context->pubSub)
    {
        PubSub_UnsubscribeTerminate(context->pubSub, terminateEventHandler);
    }
}
```

### 6. Build System Configuration
Created a comprehensive CMake build system that handles FreeRDP dependencies and vcpkg integration:

- **File**: `CMakeLists.txt`
- **Purpose**: Manages library dependencies, include paths, and source file compilation
- **Key Features**: vcpkg integration, X11 support, and modular source management

**vcpkg Integration:**
```cmake
# Get vcpkg root from environment variable
set(VCPKG_ROOT $ENV{VCPKG_ROOT}/installed/x64-linux)

# Verify vcpkg root exists
if(NOT EXISTS ${VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable not set or path doesn't exist: ${VCPKG_ROOT}")
endif()
```

**Library Discovery:**
```cmake
# Find required FreeRDP libraries
find_library(FREERDP_LIB        
    NAMES freerdp3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)

find_library(FREERDP_CLIENT_LIB 
    NAMES freerdp-client3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)

find_library(WINPR_LIB          
    NAMES winpr3
    PATHS "${VCPKG_ROOT}/lib" 
    REQUIRED
    NO_DEFAULT_PATH)
```

**Source File Management:**
```cmake
set(SOURCES
    main.c
    client/client.c                          # Main client implementation
    client/client_hooks.c                    # Callback hook implementations
    channels/remdesk/common/remdesk_common.c # Remote desktop common functionality
    channels/remdesk/client/remdesk_main.c   # Remote desktop client channel
    errors/error.c                           # Error handling utilities
)
```

**Include Path Configuration:**
```cmake
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/channels/remdesk/common
        ${CMAKE_CURRENT_SOURCE_DIR}/channels/remdesk/client
        "${VCPKG_ROOT}/include"              # vcpkg headers
        "${VCPKG_ROOT}/include/freerdp3"     # FreeRDP headers
        "${VCPKG_ROOT}/include/winpr3"       # WinPR headers
        ${X11_INCLUDE_DIR}                   # X11 headers
)
```

**Library Linking:**
```cmake
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${FREERDP_CLIENT_LIB}    # FreeRDP client library
        ${FREERDP_LIB}           # Core FreeRDP library
        ${WINPR_LIB}             # Windows Portable Runtime
        ${OPENSSL_SSL_LIB}       # SSL support
        ${OPENSSL_CRYPTO_LIB}    # Cryptographic functions
        ${CJSON_LIB}             # JSON parsing
        ${X11_LIBRARIES}         # X11 core libraries
        ${X11_Xext_LIB}          # X11 extensions
        pthread                  # Threading support
        m                        # Math library
        dl                       # Dynamic linking
)
```