#ifndef MYTHV4L2M2MCONTEXT_H
#define MYTHV4L2M2MCONTEXT_H

// MythTV
#include "mythcodeccontext.h"

class MythV4L2M2MContext : public MythCodecContext
{
  public:
    MythV4L2M2MContext(DecoderBase *Parent, MythCodecID CodecID);
    static MythCodecID GetSupportedCodec (AVCodecContext *Context,
                                          AVCodec       **Codec,
                                          const QString  &Decoder,
                                          uint            StreamType,
                                          AVPixelFormat  &PixFmt);
    bool        RetrieveFrame            (AVCodecContext *Context, VideoFrame *Frame, AVFrame *AvFrame) override;
    static bool GetBuffer                (AVCodecContext *Context, VideoFrame *Frame, AVFrame *AvFrame, int);
    static bool GetDRMBuffer             (AVCodecContext *Context, VideoFrame *Frame, AVFrame *AvFrame, int);
    static bool HaveV4L2Codecs           (AVCodecID Codec = AV_CODEC_ID_NONE);
};

#endif // MYTHV4L2M2MCONTEXT_H
