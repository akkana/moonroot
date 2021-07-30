/*
 * moonroot.c: draw the moon as a shaped window.
 *
 * Copyright 2004 by Akkana Peck.
 * You are free to use or modify this code under the Gnu Public License.
 */

#include "moonroot.h"

#include <stdio.h>
#include <unistd.h>    // for fork
#include <stdlib.h>    // for getenv
#include <libgen.h>    // for basename
#include <time.h>      // for timezone
#include <X11/keysym.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <X11/Xmd.h>   // for CARD32
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* To ask window managers to turn off decorations.
 * See <Xm/MwmUtil.h> from libmotif-dev
 * but it applies even when not using Motif
 * so the structure is included here in case libmotif-dev isn't installed.
 */
#define PROP_MWM_HINTS_ELEMENTS             5
#define MWM_HINTS_DECORATIONS   (1L << 1)       /* MwmHints.decorations */

typedef struct _mwmhints {
    CARD32 flags;
    CARD32 functions;
    CARD32 decorations;
    INT32  input_mode;
    CARD32 status;
} MotifWmHints;

#include "fullmoon100.xpm"
#include "fullmoon174.xpm"
static char** fullmoonXPM = fullmoon174_xpm;
static unsigned int fullmoonDiam = 174;

Display* dpy;
int screen;
Window win;

GC gc = 0;

int XWinSize = 0;
int YWinSize = 0;

static Pixmap moonpix;
static Pixmap moonmask;

int lastMouseX=-1,
    lastMouseY=-1;

void Quit()
{
    XFreePixmap(dpy, moonpix);
    exit(0);
}

void InitWindow(int argc, char** argv)
{
    char* appname;
    XpmAttributes xpmattr;
    int rv;
    XClassHint classHint;
    XSizeHints size;

    MotifWmHints hints;
    Atom property;
    Atom state, skip;

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

    size.flags = PMinSize | PMaxSize;
    size.max_width = 0;
    size.min_width = XWinSize;
    size.max_height = 0;
    size.min_height = YWinSize;
    XSetWMNormalHints(dpy, win, &size);

    if (argv && argc > 1)
        appname = basename(argv[0]);
    else
        appname = "moonroot";

    XStoreName(dpy, win, appname);
    /* XStoreName is just a shortcut to XSetWMName */

    classHint.res_name = appname;
    /* this name is the one the window manager uses,
     * not the same as the XA_NAME prop set by XStoreName.
     */
    classHint.res_class = "MoonRoot";
    XSetClassHint(dpy, win, &classHint);

    state = XInternAtom(dpy, "_NET_WM_STATE", True);
    skip = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", True);

    hints.flags = 2;
    hints.decorations = 0;

    property = XInternAtom(dpy, "_MOTIF_WM_HINTS", True);

    XChangeProperty(dpy, win, property, property, 32,
                    PropModeReplace, (unsigned char *)&hints, 5);

    XChangeProperty(dpy, win, state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&skip, 1);

    if (XInternAtom (dpy, "_MOTIF_WM_INFO", True) != None)
    {
        MotifWmHints mwmhints;
        Atom hints = XInternAtom (dpy, "_MOTIF_WM_HINTS", True);

        mwmhints.flags = MWM_HINTS_DECORATIONS;
        mwmhints.decorations = 0;
        XChangeProperty (dpy, win, hints, hints, 32,
                         PropModeReplace,
                         (unsigned char *)&mwmhints, PROP_MWM_HINTS_ELEMENTS);
    }
    /* else window manager doesn't support MWM hints */

    XSelectInput(dpy, win,
                 ExposureMask
                 | KeyPressMask
                 | ButtonPressMask | ButtonReleaseMask | Button1MotionMask
                 | StructureNotifyMask);

    /* Draw the moon bits */
    xpmattr.valuemask = 0;
    rv = XpmCreatePixmapFromData(dpy, win, fullmoonXPM,
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
    XFlush(dpy);            /* Flush just in case */
}

void Draw()
{
    int shape_event_base, shape_error_base;
    time_t now;

    XCopyArea(dpy, moonpix, win, gc,
              0, 0,
              fullmoonDiam, fullmoonDiam,
              0, 0);

    /* time() appears to be UTC already,
     * though the man page isn't clear about it.
     */
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

    Window dummy;
    int curWinX, curWinY;

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

        case ButtonPress:
            lastMouseX = event.xbutton.x_root;
            lastMouseY = event.xbutton.y_root;
            break;

        case ButtonRelease:
            lastMouseX = lastMouseY = -1;
            break;

        case MotionNotify:
            if (lastMouseX > 0) {
                curWinX = event.xmotion.x_root - event.xmotion.x;
                curWinY = event.xmotion.y_root - event.xmotion.y;
                XMoveWindow(dpy, win,
                            curWinX + event.xmotion.x_root - lastMouseX,
                            curWinY + event.xmotion.y_root - lastMouseY);
            }
            lastMouseX = event.xmotion.x_root;
            lastMouseY = event.xmotion.y_root;

            //XFlush(dpy);
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

static void Usage()
{
    printf("MoonRoot version 0.7, by Akkana.\n\n");
    printf("Usage: moonroot [-s]\n");
    printf("\n-s gives a smaller moon.\n");
    exit(0);
}

int main(int argc, char** argv)
{
    while (argc > 1) {
        /* Smaller image */
        if (argv[1][0] == '-' && argv[1][1] == 's') {
            fullmoonXPM = fullmoon100_xpm;
            fullmoonDiam = 100;
        }
        else {
            Usage();
        }
        --argc;
        ++argv;
    }

    InitWindow(argc, argv);

    /* run in the background */
    if (fork() > 0)
        return 0;

    while (HandleEvent() >= 0)
        ;

    XFreePixmap(dpy, moonpix);
    return 0;
}
