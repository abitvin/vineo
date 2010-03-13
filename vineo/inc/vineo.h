

// TODO bij nieuwe Vineo moet er een GL texture en AL src, buffers aangemaakt worden
// TODO audio moet beter syncen, op andere manier? Ook audio bufferen in een VineoAudioBuffer?
// NOTE Misschien is alutCreateBufferWaveform() ook een manier om te syncen? Als het bijvoorbeeld voor loopt?


/*
    NOTE: pts audio
    // basetime = nummer van BUFFER_SIZE
    ALint offset;
    alGetSourcei(g_sndSrc, AL_SAMPLE_OFFSET, &offset);
    offset += basetime * (BUFFER_SIZE/channels*8/bits);
    printf( "\rTime: %d:%05.02f", offset/rate/60, (offset%(rate*60))/(float)rate);
*/

#ifndef VINEO_H
#define VINEO_H


#include <AL/al.h>
#include <GL/gl.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


// FIXME alle onderstaande defines moeten dynamisch zijn
#define MAX_VIDEOQ_SIZE (10 * 256 * 1024)
#define MAX_AUDIOQ_SIZE (10 * 16 * 1024)
#define VID_BUFFER_SIZE 10      // aantal frames bufferen
#define BUFFER_SIZE 192000
#define NUM_BUFFERS 6               // FIXME: andere naam




typedef struct VineoVideoPicture {
    int64_t pts;
    char *data;
    int dataSize;
    int dataSizeMax;
    int width, height;
    struct VineoVideoPicture *next;
    struct VineoVideoPicture *prev;
} VineoVideoPicture;


typedef struct VineoVideoBuffer {
    VineoVideoPicture *first;
    VineoVideoPicture *last;
    int maxSize;
    int size;
} VineoVideoBuffer;


typedef struct VineoPacketQueue {
    AVPacketList *first, *last;
    int packets;        // total number of packets in this queue
    int size;           // total size in bytes
    int maxSize;        // maximum size in bytes
} VineoPacketQueue;


typedef struct Vineo
{
    // FFmpeg
    AVFormatContext *fmtCtx;    // file format context
    AVCodecContext *audCodecCtx;  // audio codec context
    AVCodecContext *vidCodecCtx;  // video codec context
    AVCodec *audCodec;            // TODO desc
    AVCodec *vidCodec;            // TODO desc

    // Queued Packets
    VineoPacketQueue audPktQueue;
    VineoPacketQueue vidPktQueue;

    // Video
    GLuint texGL;                 // OpenGL texture index
    VineoVideoBuffer vidBuffer;

    // Audio
    int audRate;
    int audChannels;
    int audBits;
    int audFormat;
    ALuint audSrcAL;                 // OpenAL audio source
    ALuint audBufAL[NUM_BUFFERS];    // OpenAL audio buffers

    // Audio decoding
    int bufferPlaying;          // current buffer playing
    char *data;                 // encoded audio packet data
    ALbyte *dataTmp;            // temp. raw encoded audio data
    size_t dataSize;            // current written bytes in *data
    size_t dataSizeMax;         // maximum bytes length of *data
    char *decData;              // raw decoded data
    size_t decDataSize;         // length decoded data

    // Video decoding
    struct SwsContext *sws;     // TODO desc
    uint8_t *frameBuffer;       // TODO desc
    AVFrame *frame;             // TODO desc
    AVFrame *frameRGB;          // TODO desc
    VineoVideoPicture *curVP;   // currently showing VideoPicture

    // Other
    int64_t time;               // current player time
    int64_t timeOffset;         // player time offset from av_gettime()
    int64_t startTime;          // start time offset in container file
    int idxAudio;               // index audio stream
    int idxVideo;               // index video stream
    int isPlaying;              // is opened so we can decode?
    char *custom;               // custom variable for u to use
} Vineo;


void vineoClose( Vineo *v );
void vineoDecode( Vineo *v );
Vineo *vineoNew();
void vineoOpen( Vineo *v, char *file );
VineoVideoPicture *vineoPicture( Vineo *v );
void vineoPlay( Vineo *v );
void vineoVolume( Vineo *v, ALfloat vol );


#endif
