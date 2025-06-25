#include <stdio.h>
#include "client/client.h"
#include "context/client_context.h"

int main(int argc, char* argv[]){

    int status = 0;
    int returnCode = 0;
    HANDLE thread = NULL;
    DWORD dwExitCode = 0;

    clientContext* cli_context = NULL;

    rdpContext* context = NULL;
    rdpSettings* settings = NULL;
    RDP_CLIENT_ENTRY_POINTS clientEntryPoints = { 0 };

	clientEntryPoints.Size = sizeof(RDP_CLIENT_ENTRY_POINTS);
	clientEntryPoints.Version = RDP_CLIENT_INTERFACE_VERSION;

    RdpClientEntry(&clientEntryPoints);

    context = freerdp_client_context_new(&clientEntryPoints);
    if (!context) return 1;

    settings = context->settings;
	cli_context = (clientContext*)context;

    status = freerdp_client_settings_parse_command_line(context->settings, argc, argv, FALSE);
    if (status){
		returnCode = freerdp_client_settings_command_line_status_print(settings, status, argc, argv);
        goto out;
	}

    if (!stream_dump_register_handlers(context, CONNECTION_STATE_MCS_CREATE_REQUEST, FALSE))
		goto out;

	if (freerdp_client_start(context) != 0)
		goto out;

    thread = freerdp_client_get_thread(context);

	(void)WaitForSingleObject(thread, INFINITE);
	GetExitCodeThread(thread, &dwExitCode);
	printf("Exited on code: %lu\n", dwExitCode);
	freerdp_client_stop(context);

out:
	freerdp_client_context_free(context);
    return returnCode;
}