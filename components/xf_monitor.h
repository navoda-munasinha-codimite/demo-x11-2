/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * X11 Monitor Handling
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREERDP_CLIENT_X11_MONITOR_H
#define FREERDP_CLIENT_X11_MONITOR_H

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

// #include "../context/client_context.h"
// #include "xf_window.h"
typedef struct client_context clientContext;

typedef struct minitor_info
{
	RECTANGLE_16 area;
	RECTANGLE_16 workarea;
	BOOL primary;
} MONITOR_INFO;

typedef struct vir_screen
{
	UINT32 nmonitors;
	RECTANGLE_16 area;
	RECTANGLE_16 workarea;
	MONITOR_INFO* monitors;
} VIRTUAL_SCREEN;


FREERDP_API int xf_list_monitors(clientContext* clicon);
FREERDP_API BOOL xf_detect_monitors(clientContext* clicon, UINT32* pWidth, UINT32* pHeight);
FREERDP_API void xf_monitors_free(clientContext* clicon);

#endif /* FREERDP_CLIENT_X11_MONITOR_H */
