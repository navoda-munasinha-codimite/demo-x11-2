#include <stdio.h>
#include "client.h"
#include "../context/client_context.h"
#include <locale.h>
#include "client_hooks.h"
#include <winpr/synch.h>
#include "../errors/error.h"


#define TAG CLIENT_TAG("client-x11")


static BOOL client_global_init(void){
	// NOLINTNEXTLINE(concurrency-mt-unsafe)
	(void)setlocale(LC_ALL, "");

	if (freerdp_handle_signals() != 0)
		return FALSE;

	return TRUE;
}

static void client_global_uninit(void){
    printf("client_global_uninit called\n");
}

static BOOL client_new(freerdp* instance, rdpContext* context){
    clientContext* clicon = (clientContext*)instance->context;
	WINPR_ASSERT(context);
	WINPR_ASSERT(clicon);
	instance->PreConnect = pre_connect;
	instance->PostConnect = post_connect;
	instance->PostDisconnect = post_disconnect;
	instance->PostFinalDisconnect = post_final_disconnect;
	instance->LogonErrorInfo = logon_error_info;
	instance->GetAccessToken = client_cli_get_access_token;
	PubSub_SubscribeTerminate(context->pubSub, terminateEventHandler);
	return TRUE;
}

static void client_free(WINPR_ATTR_UNUSED freerdp* instance, rdpContext* context){
    if (!context)
		return;

	if (context->pubSub)
	{
		PubSub_UnsubscribeTerminate(context->pubSub, terminateEventHandler);
#ifdef WITH_XRENDER
		PubSub_UnsubscribeZoomingChange(context->pubSub, xf_ZoomingChangeEventHandler);
		PubSub_UnsubscribePanningChange(context->pubSub, xf_PanningChangeEventHandler);
#endif
	}
}

static BOOL handle_window_events(freerdp* instance){
    return TRUE;
}

static DWORD WINAPI client_thread(LPVOID param){
    DWORD exit_code = 0;
	DWORD waitStatus = 0;
	HANDLE inputEvent = NULL;

	freerdp* instance = (freerdp*)param;
	WINPR_ASSERT(instance);

	const BOOL status = freerdp_connect(instance);
	rdpContext* context = instance->context;
	WINPR_ASSERT(context);
	clientContext* clicon = (clientContext*)instance->context;
	WINPR_ASSERT(clicon);

	rdpSettings* settings = context->settings;
	WINPR_ASSERT(settings);

	if (!status)
	{
		UINT32 error = freerdp_get_last_error(instance->context);
		exit_code = (uint32_t)map_error_to_exit_code(error);
	}
	else
		exit_code = XF_EXIT_SUCCESS;

	if (!status)
		goto end;

	/* --authonly ? */
	if (freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	{
		WLog_ERR(TAG, "Authentication only, exit status %d", !status);
		goto disconnect;
	}

	if (!status)
	{
		WLog_ERR(TAG, "Freerdp connect error exit status %d", !status);
		exit_code = freerdp_error_info(instance);

		if (freerdp_get_last_error(instance->context) == FREERDP_ERROR_AUTHENTICATION_FAILED)
			exit_code = XF_EXIT_AUTH_FAILURE;
		else if (exit_code == ERRINFO_SUCCESS)
			exit_code = XF_EXIT_CONN_FAILED;

		goto disconnect;
	}

	inputEvent = clicon->x11event;

	while (!freerdp_shall_disconnect_context(instance->context))
	{
		HANDLE handles[MAXIMUM_WAIT_OBJECTS] = { 0 };
		DWORD nCount = 0;
		handles[nCount++] = inputEvent;

		/*
		 * win8 and server 2k12 seem to have some timing issue/race condition
		 * when a initial sync request is send to sync the keyboard indicators
		 * sending the sync event twice fixed this problem
		 */
		// if (freerdp_focus_required(instance))
		// {
		// 	xf_keyboard_focus_in(clicon);
		// 	xf_keyboard_focus_in(clicon);
		// }

		{
			DWORD tmp =
			    freerdp_get_event_handles(context, &handles[nCount], ARRAYSIZE(handles) - nCount);

			if (tmp == 0)
			{
				WLog_ERR(TAG, "freerdp_get_event_handles failed");
				break;
			}

			nCount += tmp;
		}

		// if (clicon->window)
		// 	xf_floatbar_hide_and_show(clicon->window->floatbar);

		waitStatus = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);

		if (waitStatus == WAIT_FAILED)
			break;

		{
			if (!freerdp_check_event_handles(context))
			{
				if (client_auto_reconnect_ex(instance, handle_window_events))
					continue;
				else
				{
					/*
					 * Indicate an unsuccessful connection attempt if reconnect
					 * did not succeed and no other error was specified.
					 */
					const UINT32 error = freerdp_get_last_error(instance->context);

					if (freerdp_error_info(instance) == 0)
						exit_code = (uint32_t)map_error_to_exit_code(error);
				}

				if (freerdp_get_last_error(context) == FREERDP_ERROR_SUCCESS)
					WLog_ERR(TAG, "Failed to check FreeRDP file descriptor");

				break;
			}
		}

		if (!handle_window_events(instance))
			break;
	}

	if (!exit_code)
	{
		exit_code = freerdp_error_info(instance);

		if (exit_code == XF_EXIT_DISCONNECT &&
		    freerdp_get_disconnect_ultimatum(context) == Disconnect_Ultimatum_user_requested)
		{
			/* This situation might be limited to Windows XP. */
			WLog_INFO(TAG, "Error info says user did not initiate but disconnect ultimatum says "
			               "they did; treat this as a user logoff");
			exit_code = XF_EXIT_LOGOFF;
		}
	}

disconnect:
	freerdp_disconnect(instance);
    
end:
	ExitThread(exit_code);
	return exit_code;

}   

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