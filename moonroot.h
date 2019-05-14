/*
 * moonroot.h: draw the moon as a shaped window.
 *
 * Copyright 2004 by Akkana Peck.
 * You are free to use or modify this code under the Gnu Public License.
 */

#include <X11/Xlib.h>

extern Display* dpy;
extern int screen;
extern Window win;
extern GC gc;

extern int XWinSize;
extern int YWinSize;

extern void PaintDarkside(int moonsize, time_t date);

