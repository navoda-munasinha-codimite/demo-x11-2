#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include <freerdp/freerdp.h>
#include <freerdp/locale/keyboard.h>
#include <X11/Xlib.h>
#include "../components/xf_monitor.h"

typedef struct vir_screen VIRTUAL_SCREEN;

typedef struct _FullscreenMonitors
{
	INT32 top;
	INT32 bottom;
	INT32 left;
	INT32 right;
} FullscreenMonitors;

typedef struct work_area{
	INT32 x;
	INT32 y;
	UINT32 width;
	UINT32 height;
} WorkArea;

typedef struct client_context
{
    rdpClientContext common; // Base context structure
    
    // Add any additional fields specific to the client context here

	BOOL actionScriptExists;
	WorkArea workArea;
    FullscreenMonitors fullscreenMonitors;


    // for X11 setup
    int xfds; // File descriptor for X11 events
    HANDLE x11event;
    BOOL UseXThreads;
	Display* display;
	HANDLE mutex;
	int screen_number;
	Screen* screen;
	BOOL big_endian;
	BOOL invert;
	BOOL complex_regions;
    wLog* log;
	unsigned long supportedAtomCount;
    Atom* supportedAtoms;
	BOOL xkbAvailable;
	VIRTUAL_SCREEN vscreen;
	int current_desktop;


    Atom NET_SUPPORTED;
    Atom NET_SUPPORTING_WM_CHECK;
    Atom XWAYLAND_MAY_GRAB_KEYBOARD;
    Atom NET_WM_ICON;
    Atom MOTIF_WM_HINTS;
    Atom NET_NUMBER_OF_DESKTOPS;
    Atom NET_CURRENT_DESKTOP;
    Atom NET_WORKAREA;
    Atom NET_WM_STATE;
    Atom NET_WM_STATE_MODAL;
    Atom NET_WM_STATE_STICKY;
    Atom NET_WM_STATE_MAXIMIZED_HORZ;
    Atom NET_WM_STATE_MAXIMIZED_VERT;
    Atom NET_WM_STATE_SHADED;
    Atom NET_WM_STATE_SKIP_TASKBAR;
    Atom NET_WM_STATE_SKIP_PAGER;
    Atom NET_WM_STATE_HIDDEN;
    Atom NET_WM_STATE_FULLSCREEN;
    Atom NET_WM_STATE_ABOVE;
    Atom NET_WM_STATE_BELOW;
    Atom NET_WM_STATE_DEMANDS_ATTENTION;
    Atom NET_WM_FULLSCREEN_MONITORS;
    Atom NET_WM_NAME;
    Atom NET_WM_PID;
    Atom NET_WM_WINDOW_TYPE;
    Atom NET_WM_WINDOW_TYPE_NORMAL;
    Atom NET_WM_WINDOW_TYPE_DIALOG;
    Atom NET_WM_WINDOW_TYPE_POPUP;
    Atom NET_WM_WINDOW_TYPE_POPUP_MENU;
    Atom NET_WM_WINDOW_TYPE_UTILITY;
    Atom NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
    Atom NET_WM_MOVERESIZE;
    Atom NET_MOVERESIZE_WINDOW;
    Atom UTF8_STRING;
    Atom WM_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
    Atom WM_STATE;
    Atom NET_WM_ALLOWED_ACTIONS;
    Atom NET_WM_ACTION_CLOSE;
    Atom NET_WM_ACTION_MINIMIZE;
    Atom NET_WM_ACTION_MOVE;
    Atom NET_WM_ACTION_RESIZE;
    Atom NET_WM_ACTION_MAXIMIZE_HORZ;
    Atom NET_WM_ACTION_MAXIMIZE_VERT;
    Atom NET_WM_ACTION_FULLSCREEN;
    Atom NET_WM_ACTION_CHANGE_DESKTOP;





    FREERDP_REMAP_TABLE* remap_table;

} clientContext;


#endif // CLIENT_CONTEXT_H