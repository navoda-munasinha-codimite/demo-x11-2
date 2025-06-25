#include "../context/client_context.h"
#include "client_hooks.h"
#include <winpr/sspicli.h>

#define TAG CLIENT_TAG("hooks-x11")

void terminateEventHandler(void* context, const TerminateEventArgs* e){
    printf("terminateEventHandler called\n");
    rdpContext* ctx = (rdpContext*)context;
    WINPR_UNUSED(e);
    freerdp_abort_connect_context(ctx);
}

BOOL setup_x11(clientContext* clicon){
    printf("setup_x11 called\n");

    clicon->x11event = CreateFileDescriptorEvent(NULL, FALSE, FALSE, clicon->xfds, WINPR_FD_READ);

    return TRUE;
}

BOOL pre_connect(freerdp* instance){
    printf("pre_connect called\n");

    UINT32 maxWidth = 0;
	UINT32 maxHeight = 0;

	WINPR_ASSERT(instance);

	rdpContext* context = instance->context;
	WINPR_ASSERT(context);
	clientContext* clicon = (clientContext*)context;

	rdpSettings* settings = context->settings;
	WINPR_ASSERT(settings);

    if (!freerdp_settings_set_bool(settings, FreeRDP_CertificateCallbackPreferPEM, TRUE)) return FALSE;

    // Disable certificate verification for self-signed certificates
    if (!freerdp_settings_set_bool(settings, FreeRDP_IgnoreCertificate, TRUE))
        return FALSE;

    if (!freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	{
		if (!setup_x11(clicon))
			return FALSE;
	}

    if (!freerdp_settings_set_uint32(settings, FreeRDP_OsMajorType, OSMAJORTYPE_UNIX))
		return FALSE;
	if (!freerdp_settings_set_uint32(settings, FreeRDP_OsMinorType, OSMINORTYPE_NATIVE_XSERVER))
		return FALSE;

    // PubSub_SubscribeChannelConnected(context->pubSub, xf_OnChannelConnectedEventHandler);
	// PubSub_SubscribeChannelDisconnected(context->pubSub, xf_OnChannelDisconnectedEventHandler);

    if (!freerdp_settings_get_string(settings, FreeRDP_Username) &&
	    !freerdp_settings_get_bool(settings, FreeRDP_CredentialsFromStdin) &&
	    !freerdp_settings_get_bool(settings, FreeRDP_SmartcardLogon))
	{
		char login_name[MAX_PATH] = { 0 };
		ULONG size = sizeof(login_name) - 1;

		if (GetUserNameExA(NameSamCompatible, login_name, &size))
		{
			if (!freerdp_settings_set_string(settings, FreeRDP_Username, login_name))
				return FALSE;

			WLog_INFO(TAG, "No user name set. - Using login name: %s",
			          freerdp_settings_get_string(settings, FreeRDP_Username));
		}
	}

    if (freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	{
		/* Check +auth-only has a username and password. */
		if (!freerdp_settings_get_string(settings, FreeRDP_Password))
		{
			WLog_INFO(TAG, "auth-only, but no password set. Please provide one.");
			return FALSE;
		}

		WLog_INFO(TAG, "Authentication only. Don't connect to X.");
	}

    if (!freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	{
		const char* KeyboardRemappingList = freerdp_settings_get_string(
		    clicon->common.context.settings, FreeRDP_KeyboardRemappingList);

		clicon->remap_table = freerdp_keyboard_remap_string_to_list(KeyboardRemappingList);
		if (!clicon->remap_table)
			return FALSE;
		// if (!xf_keyboard_init(clicon))
		// 	return FALSE;
		// if (!xf_keyboard_action_script_init(clicon))
		// 	return FALSE;
		// if (!xf_detect_monitors(clicon, &maxWidth, &maxHeight))
		// 	return FALSE;

        //for now, hard code max width and height
        maxWidth = 1024;
        maxHeight = 768;
	}

    if (maxWidth && maxHeight && !freerdp_settings_get_bool(settings, FreeRDP_SmartSizing))
	{
		if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth, maxWidth))
			return FALSE;
		if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight, maxHeight))
			return FALSE;
	}

    // xfc->fullscreen = freerdp_settings_get_bool(settings, FreeRDP_Fullscreen);
	// xfc->decorations = freerdp_settings_get_bool(settings, FreeRDP_Decorations);
	// xfc->grab_keyboard = freerdp_settings_get_bool(settings, FreeRDP_GrabKeyboard);
	// xfc->fullscreen_toggle = freerdp_settings_get_bool(settings, FreeRDP_ToggleFullscreen);
	// if (!freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	// 	xf_button_map_init(xfc);

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
    printf("post_final_disconnect called\n");
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
