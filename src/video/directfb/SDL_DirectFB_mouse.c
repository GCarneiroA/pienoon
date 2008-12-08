/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "SDL_DirectFB_video.h"

#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"

static SDL_Cursor *DirectFB_CreateCursor(SDL_Surface * surface, int hot_x,
                                         int hot_y);
static int DirectFB_ShowCursor(SDL_Cursor * cursor);
static void DirectFB_MoveCursor(SDL_Cursor * cursor);
static void DirectFB_FreeCursor(SDL_Cursor * cursor);
static void DirectFB_WarpMouse(SDL_Mouse * mouse, SDL_WindowID windowID,
                               int x, int y);
static void DirectFB_FreeMouse(SDL_Mouse * mouse);

void
DirectFB_InitMouse(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);
    SDL_Mouse mouse;

    SDL_zero(mouse);
    mouse.CreateCursor = DirectFB_CreateCursor;
    mouse.ShowCursor = DirectFB_ShowCursor;
    mouse.MoveCursor = DirectFB_MoveCursor;
    mouse.FreeCursor = DirectFB_FreeCursor;
    mouse.WarpMouse = DirectFB_WarpMouse;
    mouse.FreeMouse = DirectFB_FreeMouse;
    mouse.cursor_shown = 1;
    SDL_SetMouseIndexId(0, 0);  /* ID == Index ! */
    devdata->mouse = SDL_AddMouse(&mouse, 0, "Mouse", 0, 0, 1);
}

void
DirectFB_QuitMouse(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    SDL_DelMouse(devdata->mouse);
}

/* Create a cursor from a surface */
static SDL_Cursor *
DirectFB_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
    SDL_VideoDevice *dev = SDL_GetVideoDevice();

    SDL_DFB_DEVICEDATA(dev);
    DFB_CursorData *curdata;
    DFBResult ret;
    DFBSurfaceDescription dsc;
    SDL_Cursor *cursor;
    Uint32 *dest;
    Uint32 *p;
    int pitch, i;

    SDL_DFB_CALLOC(cursor, 1, sizeof(*cursor));
    SDL_DFB_CALLOC(curdata, 1, sizeof(*curdata));

    dsc.flags =
        DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
    dsc.caps = DSCAPS_VIDEOONLY;
    dsc.width = surface->w;
    dsc.height = surface->h;
    dsc.pixelformat = DSPF_ARGB;

    SDL_DFB_CHECKERR(devdata->dfb->
                     CreateSurface(devdata->dfb, &dsc, &curdata->surf));
    curdata->hotx = hot_x;
    curdata->hoty = hot_y;
    cursor->driverdata = curdata;

    SDL_DFB_CHECKERR(curdata->surf->
                     Lock(curdata->surf, DSLF_WRITE, (void *) &dest, &pitch));

    /* Relies on the fact that this is only called with ARGB surface. */
    p = surface->pixels;
    for (i = 0; i < surface->h; i++)
        memcpy((char *) dest + i * pitch, (char *) p + i * surface->pitch,
               4 * surface->w);

    curdata->surf->Unlock(curdata->surf);
    return cursor;
  error:
    return NULL;
}

/* Show the specified cursor, or hide if cursor is NULL */
static int
DirectFB_ShowCursor(SDL_Cursor * cursor)
{
    SDL_DFB_CURSORDATA(cursor);
    DFBResult ret;
    SDL_WindowID wid;

    wid = SDL_GetFocusWindow();
    if (wid < 0)
        return -1;
    else {
        SDL_Window *window = SDL_GetWindowFromID(wid);
        SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

        if (display) {
            DFB_DisplayData *dispdata =
                (DFB_DisplayData *) display->driverdata;
            DFB_WindowData *windata = (DFB_WindowData *) window->driverdata;

            if (cursor)
                SDL_DFB_CHECKERR(windata->window->
                                 SetCursorShape(windata->window,
                                                curdata->surf, curdata->hotx,
                                                curdata->hoty));

            /* fprintf(stdout, "Cursor is %s\n", cursor ? "on" : "off"); */
            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCooperativeLevel(dispdata->layer,
                                                 DLSCL_ADMINISTRATIVE));
            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCursorOpacity(dispdata->layer,
                                              cursor ? 0xC0 : 0x00));
            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCooperativeLevel(dispdata->layer,
                                                 DLSCL_SHARED));
        }
    }

    return 0;
  error:
    return -1;
}

/* This is called when a mouse motion event occurs */
static void
DirectFB_MoveCursor(SDL_Cursor * cursor)
{

}

/* Free a window manager cursor */
static void
DirectFB_FreeCursor(SDL_Cursor * cursor)
{
    SDL_DFB_CURSORDATA(cursor);

    SDL_DFB_RELEASE(curdata->surf);
    SDL_DFB_FREE(cursor->driverdata);
    SDL_DFB_FREE(cursor);
}

/* Warp the mouse to (x,y) */
static void
DirectFB_WarpMouse(SDL_Mouse * mouse, SDL_WindowID windowID, int x, int y)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
    DFB_WindowData *windata = (DFB_WindowData *) window->driverdata;
    DFBResult ret;
    int cx, cy;

    SDL_DFB_CHECKERR(windata->window->GetPosition(windata->window, &cx, &cy));
    SDL_DFB_CHECKERR(dispdata->layer->
                     WarpCursor(dispdata->layer, cx + x, cy + y));

  error:
    return;
}

/* Free the mouse when it's time */
static void
DirectFB_FreeMouse(SDL_Mouse * mouse)
{
    /* nothing yet */
}

/* vi: set ts=4 sw=4 expandtab: */
