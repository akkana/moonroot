/*
 * moonroot.c: draw the moon as a shaped window.
 *
 * Copyright 2004 by Akkana Peck.
 * You are free to use or modify this code under the Gnu Public License.
 */

#include "moonroot.h"

#include <stdio.h>
#include <stdlib.h>    // for getenv
#include <X11/keysym.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

static unsigned int fullmoonDiam = 174;
#include "fullmoon.xpm"

Display* dpy;
int screen;
Window win;
GC gc = 0;

int XWinSize = 500;
int YWinSize = 500;

static Pixmap moonpix;
static Pixmap moonmask;

void Quit()
{
    XFreePixmap(dpy, moonpix);
    exit(0);
}

void InitWindow()
{
    XpmAttributes xpmattr;
    int rv;

    if ((dpy = XOpenDisplay(getenv("DISPLAY"))) == 0)
    {
        fprintf(stderr, "Can't open display: %s\n", getenv("DISPLAY"));
        exit(1);
    }
    screen = DefaultScreen(dpy);

    XWinSize = fullmoonDiam;
    YWinSize = fullmoonDiam;
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                              0, 0, XWinSize, YWinSize, 3,
                              WhitePixel(dpy, screen),
                              BlackPixel(dpy, screen));
    if (!win)
    {
        fprintf(stderr, "Can't create window\n");
        exit(1);
    }

    XSelectInput(dpy, win,
                 ExposureMask
                 | KeyPressMask
                 | StructureNotifyMask
                );

    /* Draw the moon bits */
    xpmattr.valuemask = 0;
    rv = XpmCreatePixmapFromData(dpy, win, fullmoon_xpm,
                                 &moonpix,
                                 &moonmask,
                                 &xpmattr);
    if (rv != 0)
        Quit();

    XGCValues gcValues;
    gcValues.foreground = WhitePixel(dpy, screen);
    gcValues.background = BlackPixel(dpy, screen);
    gc = XCreateGC(dpy, win, GCForeground | GCBackground, &gcValues);

    XMapWindow(dpy, win);
}

void Draw()
{
    int shape_event_base, shape_error_base;
    time_t now;

    XCopyArea(dpy, moonpix, win, gc,
              0, 0,
              fullmoonDiam, fullmoonDiam,
              0, 0);

    time(&now);
    PaintDarkside(fullmoonDiam, now);

    if (XShapeQueryExtension(dpy, &shape_event_base, &shape_error_base))
	XShapeCombineMask(dpy, win, ShapeBounding,
			  0, 0, moonmask, ShapeSet);
}

int HandleEvent()
{
    XEvent event;
    time_t sec;
    char buffer[20];
    KeySym keysym;
    XComposeStatus compose;

    XNextEvent(dpy, &event);
    switch (event.type)
    {
      case Expose:
      case MapNotify:
          Draw();
          break;
      case ConfigureNotify:
          XWinSize = event.xconfigure.width;
          YWinSize = event.xconfigure.height;
          //printf("ConfigureNotify: now (%d, %d)\n", XWinSize, YWinSize);
          break;
      case ReparentNotify: /* When we make the window shaped? */
      case UnmapNotify:    /* e.g. move to all desktops? */
      case NoExpose:       /* No idea what this is */
          break;
      case KeyPress:
          XLookupString(&(event.xkey), buffer, sizeof buffer,
                        &keysym, &compose);
          switch (keysym)
          {
            case XK_q:
                return -1;
            default:
                break;
          }
          break;
      default:
          printf("Unknown event: %d\n", event.type);
    }
    return 0;
}

int main(int argc, char** argv)
{
    InitWindow();

    while (HandleEvent() >= 0)
        ;

    XFreePixmap(dpy, moonpix);
    return 0;
}

