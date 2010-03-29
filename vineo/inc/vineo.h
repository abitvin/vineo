

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


#define VINEO_MAX_VIDEOQ_SIZE (10 * 256 * 1024)
#define VINEO_MAX_AUDIOQ_SIZE (10 * 16 * 1024)
#define VINEO_AUDIO_BUFFER_SIZE 192000
#define VINEO_AUDIO_NUM_BUFFERS 6


typedef struct VineoPacketQueue {
    AVPacketList *first, *last;
    int packets;        // total number of packets in this queue
    int size;           // total size in bytes
    int max_size;       // maximum size in bytes
} VineoPacketQueue;


typedef struct Vineo
{
    // FFmpeg
    AVFormatContext *fmt_ctx;       // file format context
    AVCodecContext *aud_codec_ctx;  // audio codec context
    AVCodecContext *vid_codec_ctx;  // video codec context
    AVCodec *aud_codec;
    AVCodec *vid_codec;

    // Queued Packets
    VineoPacketQueue aud_pkt_queue;
    VineoPacketQueue vid_pkt_queue;

    // Video
    GLuint tex_gl;              // OpenGL texture index

    // Audio
    int aud_rate;
    int aud_channels;
    int aud_bits;
    int aud_format;
    ALuint aud_src_al;          // OpenAL audio source
    ALuint aud_buf_al[VINEO_AUDIO_NUM_BUFFERS]; // OpenAL audio buffers

    // Audio decoding
    int buffer_playing;         // current buffer playing
    char *data;                 // encoded audio packet data
    ALbyte *data_tmp;           // temp. raw encoded audio data
    size_t data_size;           // current written bytes in *data
    size_t data_size_max;       // maximum bytes length of *data
    char *dec_data;             // raw decoded data
    size_t dec_data_size;       // length decoded data

    // Video decoding
    struct SwsContext *sws;         // sws context
    unsigned char *frame_buffer;    // data buffer for frame_rgba
    AVFrame *frame;                 // decoded frame
    AVFrame *frame_rgba;            // converted frame to RGBA

    // Other
    int64_t cur_pts;            // current pts time
    int64_t time;               // current player time
    int64_t time_offset;        // player time offset from av_gettime()
    int64_t start_time;         // start time offset in container file
    int idx_audio;              // index audio stream
    int idx_video;              // index video stream
    int is_opened;              // is succesfully opened?
    int is_playing;             // is playing?
    int is_frame_finished;      // used for flushing
} Vineo;


void vineoClose( Vineo *v );
void vineoDecode( Vineo *v );
void vineoFlush( Vineo *v );
Vineo *vineoNew();
void vineoOpen( Vineo *v, char *file );
void vineoPlay( Vineo *v );
void vineoVolume( Vineo *v, ALfloat vol );


#endif
