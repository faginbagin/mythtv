//*****************************************************************************
// This code is meant for use in debugging the CommDetect class and shouldn't
// be used in normal compiled Myth versions.

// #define SHOW_DEBUG_WIN

#ifdef SHOW_DEBUG_WIN

#include "mythframe.h"

extern void comm_debug_init( int width, int height );
extern void comm_debug_show( unsigned char *frame );
extern void comm_debug_show( VideoFrame *frame );
extern void comm_debug_destroy();

#endif

