#include "../context/client_context.h"
#include "client_hooks.h"
#include <winpr/sspicli.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include "../components/xf_utils.h"

#define TAG CLIENT_TAG("hooks-x11")

static int (*def_error_handler)(Display*, XErrorEvent*);

void terminateEventHandler(void* context, const TerminateEventArgs* e){
    printf("terminateEventHandler called\n");
    rdpContext* ctx = (rdpContext*)context;
    WINPR_UNUSED(e);
    freerdp_abort_connect_context(ctx);
}


static int error_handler(Display* d, XErrorEvent* ev)
{
	char buf[256] = { 0 };
	XGetErrorText(d, ev->error_code, buf, sizeof(buf));
	WLog_ERR(TAG, "%s", buf);
	winpr_log_backtrace(TAG, WLOG_ERROR, 20);

	if (def_error_handler)
		return def_error_handler(d, ev);

	return 0;
}

static int error_handler_ex(Display* d, XErrorEvent* ev)
{
	/*
	 * ungrab the keyboard, in case a debugger is running in
	 * another window. This make error_handler() a potential
	 * debugger breakpoint.
	 */

	XUngrabKeyboard(d, CurrentTime);
	XUngrabPointer(d, CurrentTime);
	return error_handler(d, ev);
}

static Atom get_supported_atom(clientContext* clicon, const char* atomName)
{
	const Atom atom = Logging_XInternAtom(clicon->log, clicon->display, atomName, False);

	for (unsigned long i = 0; i < clicon->supportedAtomCount; i++)
	{
		if (clicon->supportedAtoms[i] == atom)
			return atom;
	}

	return None;
}

static void check_extensions(clientContext* context)
{
	int xkb_opcode = 0;
	int xkb_event = 0;
	int xkb_error = 0;
	int xkb_major = XkbMajorVersion;
	int xkb_minor = XkbMinorVersion;

	if (XkbLibraryVersion(&xkb_major, &xkb_minor) &&
	    XkbQueryExtension(context->display, &xkb_opcode, &xkb_event, &xkb_error, &xkb_major,
	                      &xkb_minor))
	{
		context->xkbAvailable = TRUE;
	}
}

void xf_teardown_x11(clientContext* clicon)
{
	WINPR_ASSERT(clicon);

	if (clicon->display)
	{
		LogDynAndXCloseDisplay(clicon->log, clicon->display);
		clicon->display = NULL;
	}

	if (clicon->x11event)
	{
		(void)CloseHandle(clicon->x11event);
		clicon->x11event = NULL;
	}

	if (clicon->mutex)
	{
		(void)CloseHandle(clicon->mutex);
		clicon->mutex = NULL;
	}

	if (clicon->vscreen.monitors)
	{
		free(clicon->vscreen.monitors);
		clicon->vscreen.monitors = NULL;
	}
	clicon->vscreen.nmonitors = 0;

	free(clicon->supportedAtoms);
	clicon->supportedAtoms = NULL;
	clicon->supportedAtomCount = 0;
}

BOOL setup_x11(clientContext* clicon){
    printf("setup_x11 called\n");

    WINPR_ASSERT(clicon);
	clicon->UseXThreads = TRUE;

    if (clicon->UseXThreads)
	{
		if (!XInitThreads())
		{
			WLog_WARN(TAG, "XInitThreads() failure");
			clicon->UseXThreads = FALSE;
		}
	}

	clicon->display = XOpenDisplay(NULL);

    if (!clicon->display)
	{
		WLog_ERR(TAG, "failed to open display: %s", XDisplayName(NULL));
		WLog_ERR(TAG, "Please check that the $DISPLAY environment variable is properly set.");
		goto fail;
	}

    //enable debug mode
    WLog_INFO(TAG, "Enabling X11 debug mode.");
	XSynchronize(clicon->display, TRUE);

    def_error_handler = XSetErrorHandler(error_handler_ex);

    clicon->mutex = CreateMutex(NULL, FALSE, NULL);
    if (!clicon->mutex)
	{
		WLog_ERR(TAG, "Could not create mutex!");
		goto fail;
	}

    clicon->xfds = ConnectionNumber(clicon->display);
	clicon->screen_number = DefaultScreen(clicon->display);
	clicon->screen = ScreenOfDisplay(clicon->display, clicon->screen_number);
	clicon->big_endian = (ImageByteOrder(clicon->display) == MSBFirst);
	clicon->invert = TRUE;
	clicon->complex_regions = TRUE;

    clicon->NET_SUPPORTED = Logging_XInternAtom(clicon->log, clicon->display, "_NET_SUPPORTED", True);
	clicon->NET_SUPPORTING_WM_CHECK = Logging_XInternAtom(clicon->log, clicon->display, "_NET_SUPPORTING_WM_CHECK", True);

    if ((clicon->NET_SUPPORTED != None) && (clicon->NET_SUPPORTING_WM_CHECK != None))
	{
		Atom actual_type = 0;
		int actual_format = 0;
		unsigned long nitems = 0;
		unsigned long after = 0;
		unsigned char* data = NULL;
		int status = LogDynAndXGetWindowProperty(
		    clicon->log, clicon->display, RootWindowOfScreen(clicon->screen), clicon->NET_SUPPORTED, 0, 1024,
		    False, XA_ATOM, &actual_type, &actual_format, &nitems, &after, &data);

		if ((status == Success) && (actual_type == XA_ATOM) && (actual_format == 32))
		{
			clicon->supportedAtomCount = nitems;
			clicon->supportedAtoms = calloc(clicon->supportedAtomCount, sizeof(Atom));
			WINPR_ASSERT(clicon->supportedAtoms);
			memcpy(clicon->supportedAtoms, data, nitems * sizeof(Atom));
		}

		if (data)
			XFree(data);
	}

    clicon->XWAYLAND_MAY_GRAB_KEYBOARD =
	    Logging_XInternAtom(clicon->log, clicon->display, "_XWAYLAND_MAY_GRAB_KEYBOARD", False);
	clicon->NET_WM_ICON = Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ICON", False);
	clicon->MOTIF_WM_HINTS = Logging_XInternAtom(clicon->log, clicon->display, "_MOTIF_WM_HINTS", False);
	clicon->NET_NUMBER_OF_DESKTOPS =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_NUMBER_OF_DESKTOPS", False);
	clicon->NET_CURRENT_DESKTOP =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_CURRENT_DESKTOP", False);
	clicon->NET_WORKAREA = Logging_XInternAtom(clicon->log, clicon->display, "_NET_WORKAREA", False);
	clicon->NET_WM_STATE = get_supported_atom(clicon, "_NET_WM_STATE");
	clicon->NET_WM_STATE_MODAL = get_supported_atom(clicon, "_NET_WM_STATE_MODAL");
	clicon->NET_WM_STATE_STICKY = get_supported_atom(clicon, "_NET_WM_STATE_STICKY");
	clicon->NET_WM_STATE_MAXIMIZED_HORZ =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	clicon->NET_WM_STATE_MAXIMIZED_VERT =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	clicon->NET_WM_STATE_SHADED = get_supported_atom(clicon, "_NET_WM_STATE_SHADED");
	clicon->NET_WM_STATE_SKIP_TASKBAR = get_supported_atom(clicon, "_NET_WM_STATE_SKIP_TASKBAR");
	clicon->NET_WM_STATE_SKIP_PAGER = get_supported_atom(clicon, "_NET_WM_STATE_SKIP_PAGER");
	clicon->NET_WM_STATE_HIDDEN = get_supported_atom(clicon, "_NET_WM_STATE_HIDDEN");
	clicon->NET_WM_STATE_FULLSCREEN = get_supported_atom(clicon, "_NET_WM_STATE_FULLSCREEN");
	clicon->NET_WM_STATE_ABOVE = get_supported_atom(clicon, "_NET_WM_STATE_ABOVE");
	clicon->NET_WM_STATE_BELOW = get_supported_atom(clicon, "_NET_WM_STATE_BELOW");
	clicon->NET_WM_STATE_DEMANDS_ATTENTION =
	    get_supported_atom(clicon, "_NET_WM_STATE_DEMANDS_ATTENTION");
	clicon->NET_WM_FULLSCREEN_MONITORS = get_supported_atom(clicon, "_NET_WM_FULLSCREEN_MONITORS");
	clicon->NET_WM_NAME = Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_NAME", False);
	clicon->NET_WM_PID = Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_PID", False);
	clicon->NET_WM_WINDOW_TYPE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE", False);
	clicon->NET_WM_WINDOW_TYPE_NORMAL =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
	clicon->NET_WM_WINDOW_TYPE_DIALOG =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	clicon->NET_WM_WINDOW_TYPE_POPUP =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_POPUP", False);
	clicon->NET_WM_WINDOW_TYPE_POPUP_MENU =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
	clicon->NET_WM_WINDOW_TYPE_UTILITY =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_UTILITY", False);
	clicon->NET_WM_WINDOW_TYPE_DROPDOWN_MENU =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", False);
	clicon->NET_WM_STATE_SKIP_TASKBAR =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_STATE_SKIP_TASKBAR", False);
	clicon->NET_WM_STATE_SKIP_PAGER =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_STATE_SKIP_PAGER", False);
	clicon->NET_WM_MOVERESIZE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_MOVERESIZE", False);
	clicon->NET_MOVERESIZE_WINDOW =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_MOVERESIZE_WINDOW", False);
	clicon->UTF8_STRING = Logging_XInternAtom(clicon->log, clicon->display, "UTF8_STRING", FALSE);
	clicon->WM_PROTOCOLS = Logging_XInternAtom(clicon->log, clicon->display, "WM_PROTOCOLS", False);
	clicon->WM_DELETE_WINDOW = Logging_XInternAtom(clicon->log, clicon->display, "WM_DELETE_WINDOW", False);
	clicon->WM_STATE = Logging_XInternAtom(clicon->log, clicon->display, "WM_STATE", False);
	clicon->x11event = CreateFileDescriptorEvent(NULL, FALSE, FALSE, clicon->xfds, WINPR_FD_READ);

	clicon->NET_WM_ALLOWED_ACTIONS =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ALLOWED_ACTIONS", False);

	clicon->NET_WM_ACTION_CLOSE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_CLOSE", False);
	clicon->NET_WM_ACTION_MINIMIZE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_MINIMIZE", False);
	clicon->NET_WM_ACTION_MOVE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_MOVE", False);
	clicon->NET_WM_ACTION_RESIZE =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_RESIZE", False);
	clicon->NET_WM_ACTION_MAXIMIZE_HORZ =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
	clicon->NET_WM_ACTION_MAXIMIZE_VERT =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
	clicon->NET_WM_ACTION_FULLSCREEN =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_FULLSCREEN", False);
	clicon->NET_WM_ACTION_CHANGE_DESKTOP =
	    Logging_XInternAtom(clicon->log, clicon->display, "_NET_WM_ACTION_CHANGE_DESKTOP", False);

	if (!clicon->x11event)
	{
		WLog_ERR(TAG, "Could not create xfds event");
		goto fail;
	}

    check_extensions(clicon);

    clicon->vscreen.monitors = calloc(16, sizeof(MONITOR_INFO));

	if (!clicon->vscreen.monitors)
		goto fail;
	return TRUE;

fail:
	xf_teardown_x11(clicon);
	return FALSE;
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

    // clicon->fullscreen = freerdp_settings_get_bool(settings, FreeRDP_Fullscreen);
	// clicon->decorations = freerdp_settings_get_bool(settings, FreeRDP_Decorations);
	// clicon->grab_keyboard = freerdp_settings_get_bool(settings, FreeRDP_GrabKeyboard);
	// clicon->fullscreen_toggle = freerdp_settings_get_bool(settings, FreeRDP_ToggleFullscreen);
	// if (!freerdp_settings_get_bool(settings, FreeRDP_AuthenticationOnly))
	// 	xf_button_map_init(clicon);

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
