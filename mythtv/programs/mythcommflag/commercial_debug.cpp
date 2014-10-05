//*****************************************************************************
// This code is meant for use in debugging the CommDetect class and shouldn't
// be used in normal compiled Myth versions.

#include "commercial_debug.h"

#ifdef SHOW_DEBUG_WIN

#include <X11/Xlib.h>
//#include <X11/extensions/Xvlib.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

// INPUT_PIX_FMT is used when we convert an input YUV420 frame to a RGB ZPixmap
// Since the commercial detection algorithms only look at the Y luma channel,
// it can be useful to display only that channel.
// But there may be times when you want to see the color images.
// If you want to display color images, use AV_PIX_FMT_YUV420P
// If you want display grayscale images, use AV_PIX_FMT_GRAY8
// #define INPUT_PIX_FMT   AV_PIX_FMT_YUV420P
#define INPUT_PIX_FMT   AV_PIX_FMT_GRAY8

Window comm_win;
GC comm_gc;
Display *comm_display;
int comm_width = 0;
int comm_width8 = 0;
int comm_height = 0;
int comm_depth = 24;
XImage *comm_image = NULL;
char *comm_buf = NULL;
struct SwsContext *scontext;

void comm_debug_init( int width, int height )
{
    comm_display = XOpenDisplay(NULL);

    Screen* comm_screen = DefaultScreenOfDisplay(comm_display);
    int comm_screen_num = DefaultScreen(comm_display);

    comm_depth = DefaultDepthOfScreen(comm_screen);

    comm_width = width;
    // FFmpeg likes linesizes that are multiples of 8.
    comm_width8 = (width + 7) & ~7;
    comm_height = height;

    comm_win = XCreateSimpleWindow(comm_display,
                                   DefaultRootWindow(comm_display),
                                   100, 100, comm_width, comm_height, 0,
                                   XWhitePixel(comm_display, comm_screen_num),
                                   XBlackPixel(comm_display, comm_screen_num) );

    XMapRaised(comm_display, comm_win);

    XSync(comm_display, 0);

    comm_gc = XCreateGC(comm_display, comm_win, 0, 0);

    comm_buf = new char[4 * comm_width8 * comm_height];
    memset(comm_buf, 0, 4 * comm_width8 * comm_height);

    comm_image = XCreateImage(comm_display, DefaultVisual(comm_display, 0),
                              comm_depth, ZPixmap, 0, comm_buf,
                              comm_width8, comm_height, 8, 0);

    XSync(comm_display, 0);

    printf( "Commercial Detection debug window created at %dx%dx%d\n",
        comm_width, comm_height, comm_depth );
}

static void comm_debug_show( AVPicture* pic);

void comm_debug_show( unsigned char *frame )
{
    AVPicture image_in;

    avpicture_fill(&image_in, (uint8_t *)frame, AV_PIX_FMT_YUV420P,
                   comm_width, comm_height);

    comm_debug_show(&image_in);
}

void comm_debug_show( VideoFrame *frame )
{
    AVPicture image_in;

    for (int i = 0; i < 3; i++)
    {
        image_in.data[i] = frame->buf + frame->offsets[i];
        image_in.linesize[i] = frame->pitches[i];
    }

    comm_debug_show(&image_in);
}

void comm_debug_show(AVPicture* pic)
{
    AVPicture image_out;
    AVPixelFormat av_format;

    switch (comm_depth)
    {
        case 16: av_format = AV_PIX_FMT_RGB565; break;
        case 24: av_format = AV_PIX_FMT_RGB32;  break;
        case 32: av_format = AV_PIX_FMT_RGB32; break;
        default:
            printf("Unable to display debug video window in %d depth.\n",
                   comm_depth);
            exit(1);
    }

    avpicture_fill(&image_out, (uint8_t *)comm_image->data, av_format,
                   comm_width8, comm_height);

    scontext = sws_getCachedContext(scontext, comm_width, comm_height,
        INPUT_PIX_FMT, comm_width, comm_height, av_format,
        SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!scontext)
    {
        printf("Cannot initialize the image conversion context");
        exit(1);
    }

    sws_scale(scontext, pic->data, pic->linesize, 0, comm_height,
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
