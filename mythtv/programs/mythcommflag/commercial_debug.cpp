//*****************************************************************************
// This code is meant for use in debugging the CommDetect class and shouldn't
// be used in normal compiled Myth versions.

#include "commercial_debug.h"

#ifdef SHOW_DEBUG_WIN

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

Window comm_root;
Window comm_win;
Window comm_curwin;
GC comm_gc;
Screen *comm_screen;
Display *comm_display;
int comm_width = 0;
int comm_height = 0;
int comm_screen_num;
int comm_depth = 24;
unsigned long comm_white, comm_black;
XImage *comm_image = NULL;
char *comm_buf = NULL;
AVPicture image_in, image_out;
struct SwsContext *scontext;
AVPixelFormat av_format;

void comm_debug_init( int width, int height )
{
    comm_display = XOpenDisplay(NULL);

    comm_screen = DefaultScreenOfDisplay(comm_display);
    comm_screen_num = DefaultScreen(comm_display);

    comm_root = DefaultRootWindow(comm_display);

    comm_white = XWhitePixel(comm_display, comm_screen_num);
    comm_black = XBlackPixel(comm_display, comm_screen_num);

    comm_depth = DefaultDepthOfScreen(comm_screen);

    comm_win = XCreateSimpleWindow(comm_display, comm_root, 100,
                                    100, width, height, 0,
                                    comm_white, comm_black );

    XMapRaised(comm_display, comm_win);

    XSync(comm_display, 0);

    comm_gc = XCreateGC(comm_display, comm_win, 0, 0);

    comm_buf = new char[4 * width * height];

    comm_image = XCreateImage(comm_display, DefaultVisual(comm_display, 0),
                              comm_depth, ZPixmap, 0, comm_buf,
                              width, height, 8, 0);

    XSync(comm_display, 0);

    comm_width = width;
    comm_height = height;

    printf( "Commercial Detection debug window created at %dx%dx%d\n",
        comm_width, comm_height, comm_depth );
}

void comm_debug_show( unsigned char *frame )
{
    avpicture_fill(&image_in, (uint8_t *)frame, PIX_FMT_YUV420P,
                   comm_width, comm_height);

    switch (comm_depth)
    {
        case 16: av_format = PIX_FMT_RGB565; break;
        case 24: av_format = PIX_FMT_RGB32;  break;
        case 32: av_format = PIX_FMT_RGB32; break;
        default:
            printf("Unable to display debug video window in %d depth.\n",
                   comm_depth);
            exit(1);
    }

    avpicture_fill(&image_out, (uint8_t *)comm_image->data, av_format,
                   comm_width, comm_height);

    scontext = sws_getCachedContext(scontext, comm_width, comm_height,
        PIX_FMT_YUV420P, comm_width, comm_height, av_format,
        SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!scontext)
    {
        printf("Cannot initialize the image conversion context");
        exit(1);
    }

    sws_scale(scontext, image_in.data, image_in.linesize, 0, comm_height,
              image_out.data, image_out.linesize);

    XPutImage(comm_display, comm_win, comm_gc, comm_image,
                0, 0, 0, 0, comm_width, comm_height );

    XSync(comm_display, 0);
}

void comm_debug_destroy()
{
    XFree(comm_image);
    delete comm_buf;
    XDestroyWindow(comm_display, comm_win);
    XCloseDisplay(comm_display);
}

#endif
