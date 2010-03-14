#include "vineo.h"



static void vineoFillPacketQueue( Vineo *v );
static int vineoNextDataAudio( Vineo *v, void *data, int length );
static int vineoNextPacket( Vineo *v, int stream, AVPacket *retPkt );
static void vineoNextPacketAudio( Vineo *v );



void vineoClose( Vineo *v )
{
    if( !v ) {
        return;
    }


    // clear video packets
    AVPacketList *tmp, *pktList = v->aud_pkt_queue.first;

    while( pktList )
    {
        tmp = pktList;
        pktList = tmp->next;

        av_free_packet( &tmp->pkt );
        av_free( tmp );
    }


    // clear audio packets
    pktList = v->vid_pkt_queue.first;

    while( pktList )
    {
        tmp = pktList;
        pktList = tmp->next;

        av_free_packet( &tmp->pkt );
        av_free( tmp );
    }


    // clear OpenAL en OpenGL resources
    glDeleteTextures( 1, &v->tex_gl );
    alSourceStop( v->aud_src_al );                // stop the source or else the buffer won't be freed
    alSourcei( v->aud_src_al, AL_BUFFER, 0 );     // free bufferdata that still exists
    alDeleteBuffers( NUM_BUFFERS, v->aud_buf_al );
    alDeleteSources( 1, &v->aud_src_al );


    // clear ffmpeg video stream
    if( v->idx_video >= 0 )
    {
        sws_freeContext( v->sws );
        av_free( v->frame_buffer );
        av_free( v->frame_rgba );
        av_free( v->frame );
        avcodec_close( v->vid_codec_ctx );
    }


    // clear ffmpeg audio stream
    if( v->idx_audio >= 0 )
    {
        av_free( v->dec_data );
        av_free( v->data );
        av_free( v->data_tmp );
        avcodec_close( v->aud_codec_ctx );
    }


    // close ffmpeg context
    if( v->fmt_ctx ) {
        av_close_input_file( v->fmt_ctx );
    }


    // free the Vineo structure
    av_free( v );


    // NOTE liefs zou ik een av_unregister_all willen hebben, maar deze bestaat niet
    // we gaan er nu van uit dat je niet hoeft te unregisteren omdat je altijd ffmpeg nodig
    // hebt in je app. en bij afsluiten van je app automatisch gefreed wordt. Tis nie mooi dat wel...
}



void vineoDecode( Vineo *v )
{
    if( !v ) {
        return;
    }


    // fill the packet queue
    vineoFillPacketQueue( v );


    // update player time
    v->time = av_gettime() - v->time_offset + v->start_time;



    double pts = 0.0f;
    AVPacket pkt;
    int frame_finished = 0;


    // decode new video frame
    while( v->time > v->cur_pts )
    {
        if( !vineoNextPacket( v, v->idx_video, &pkt ) ) {
            break;
        }

        //v->vid_codec_ctx->reordered_opaque = packet.pts;
        avcodec_decode_video( v->vid_codec_ctx, v->frame, &frame_finished, pkt.data, pkt.size );
        //printf( "pFrame->opaque: %i\n", pFrame->opaque );

        /* NOTE FIXME moeten we hier wat mee?
        if( packet.dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE )
        {
            printf( "FROM FRAME OPAQUE?," );
            pts = *(uint64_t *)pFrame->opaque;
        }
        else*/

        //printf( "reordered_opaque: %i, %i\n", packet.pts, pFrame->reordered_opaque );

        /*if( packet.dts == AV_NOPTS_VALUE && v->frame->reordered_opaque != AV_NOPTS_VALUE )
        {
            // NOTE FIXME ik weet niet of dit werkt?
            //printf( "FROM FRAME OPAQUE?," );
            pts = v->frame->reordered_opaque;
        }
        else*/ if( pkt.dts != AV_NOPTS_VALUE )
        {
            //printf( "FROM PACKET DTS,   " );
            pts = pkt.dts;
        }
        else
        {
            //printf( "NO PTS,            " );
            pts = 0;
        }

        pts *= av_q2d( v->fmt_ctx->streams[v->idx_video]->time_base );
        v->cur_pts = pts * AV_TIME_BASE;

        if( frame_finished && v->cur_pts >= v->time )
        {
            sws_scale(
                v->sws,
                v->frame->data,
                v->frame->linesize,
                0,
                v->vid_codec_ctx->height,
                v->frame_rgba->data,
                v->frame_rgba->linesize
            );

            int w = v->vid_codec_ctx->width;
            int h = v->vid_codec_ctx->height;
            unsigned char *a = v->frame_rgba->data[0];
            unsigned char *b = a + 2;
            unsigned char *end = &v->frame_rgba->data[0][w*h*4];

            for( ; a < end; a += 4, b += 4 )
            {
                *a ^= *b;
                *b ^= *a;
                *a ^= *b;
            }

            glBindTexture( GL_TEXTURE_2D, v->tex_gl );
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, v->frame_rgba->data[0] );
        }

        av_free_packet( &pkt );
    }


    // fill audio buffer
    ALuint buf = 0;
    ALint processed = 0;
    ALint state = 0;
    ALenum err;


    // NOTE FIXME ffplay.c
    /*
    sdl_audio_callback()

    audio_decode_frame()
    {
        pts = is->audio_clock;
        *pts_ptr = pts;
        n = 2 * dec->channels;
        is->audio_clock += (double)data_size /(double)(n * dec->sample_rate);
    }

    synchronize_audio(), AV_SYNC_EXTERNAL_CLOCK

    get_audio_clock()
    */


    for(;;)
    {
        processed = 0;
        alGetSourcei( v->aud_src_al, AL_BUFFERS_PROCESSED, &processed );

        if( processed == 0 )
        {
            alGetSourcei( v->aud_src_al, AL_SOURCE_STATE, &state );

            if( alGetError() != AL_NO_ERROR )
            {
                printf( "Error checking source state...\n" );
                break;
            }

            if( state != AL_PLAYING )
            {
                alSourcePlay( v->aud_src_al );

                if( alGetError() != AL_NO_ERROR )
                {
                    printf( "Error restarting playback...\n" );
                    break;
                }
            }
            else
            {
                //alGetSourcei( g_sndSrc, AL_SAMPLE_OFFSET, &offset );
                //printf( "\roffset: %i", offset );
                //alutSleep( 0.01 );
            }

            break;
        }


        double clock = 0.0f;
        int fixAudioSync = 0;
        int count = 1;

        // kijken of we wel gesyncd lopen met de g_timeCur, anders droppen we wat frames
        do
        {
            if( clock > 0 )
            {
                fixAudioSync = 1;
                printf( "Dropping audio frame\n" );
            }

            count = vineoNextDataAudio( v, v->data_tmp, BUFFER_SIZE );

            v->buffer_playing += count;
            clock = (float)v->buffer_playing / (float)( ( v->aud_bits / 8 ) * v->aud_channels * v->aud_rate ) * AV_TIME_BASE;
        }
        while( v->time > clock && count > 0 );



        if( count > 0 )
        {
            alSourceUnqueueBuffers( v->aud_src_al, 1, &buf );

            if( buf != 0 )
            {
                if( fixAudioSync )
                {
                    //double timeFix = clock - g_timeCur;
                    double timeFix = clock - v->time;
                    count = timeFix * ( (float)( ( v->aud_bits / 8 ) * v->aud_channels * v->aud_rate ) );
                    // round to nice buffer length
                    count /= ( ( v->aud_bits / 8 ) * v->aud_channels );
                    count *= ( ( v->aud_bits / 8 ) * v->aud_channels );
                    //count = BUFFER_SIZE - count;
                    printf( "- fix audio syncing, count: %i, time: %f\n", count, timeFix );
                }

                alBufferData( buf, v->aud_format, v->data_tmp, count, v->aud_rate );
                alSourceQueueBuffers( v->aud_src_al, 1, &buf );
            }

            if( ( err = alGetError() ) != AL_NO_ERROR )
            {
                printf( "Error buffering data (%i)...\n", err );
                break;
            }
        }
        else
        {
            break;
        }
    }
}



static void vineoFillPacketQueue( Vineo *v )
{
    // NOTE FIXME lelijk code! kan vast anders
    if( v->idx_audio > -1 && v->idx_video > -1 &&
        v->aud_pkt_queue.size > v->aud_pkt_queue.max_size &&
        v->vid_pkt_queue.size > v->vid_pkt_queue.max_size ) {
            return;
    }
    else if( v->idx_audio > -1 && v->idx_video < 0 &&
             v->aud_pkt_queue.size > v->aud_pkt_queue.max_size ) {
                return;
    }
    else if( v->idx_video > -1 && v->idx_audio < 0 &&
             v->vid_pkt_queue.size > v->vid_pkt_queue.max_size ) {
                return;
    }
    else if( v->idx_video < 0 && v->idx_audio < 0 ) {
        return;
    }

    AVPacket pkt;
    AVPacketList *pktList;

    while( av_read_frame( v->fmt_ctx, &pkt ) >= 0 )
    {
        if( pkt.stream_index == v->idx_audio )
        {
            if( av_dup_packet( &pkt ) == 0 )
            {
                pktList = av_malloc( sizeof(AVPacketList) );

                if( !pktList )
                {
                    printf( "Error @ vimeoFillPacketQueue @ av_malloc()\n" );
                    return;
                }

                pktList->pkt = pkt;
                pktList->next = NULL;

                if( !v->aud_pkt_queue.last ) {
                    v->aud_pkt_queue.first = pktList;
                }
                else {
                    v->aud_pkt_queue.last->next = pktList;
                }

                v->aud_pkt_queue.last = pktList;
                v->aud_pkt_queue.size += pkt.size + sizeof(*pktList);
                v->aud_pkt_queue.packets++;
            }
        }
        else if( pkt.stream_index == v->idx_video )
        {
            if( av_dup_packet( &pkt ) == 0 )
            {
                pktList = av_malloc( sizeof(AVPacketList) );

                if( !pktList )
                {
                    printf( "Error @ vimeoFillPacketQueue @ av_malloc()\n" );
                    return;
                }

                pktList->pkt = pkt;
                pktList->next = NULL;

                if( !v->vid_pkt_queue.last ) {
                    v->vid_pkt_queue.first = pktList;
                }
                else {
                    v->vid_pkt_queue.last->next = pktList;
                }

                v->vid_pkt_queue.last = pktList;
                v->vid_pkt_queue.size += pkt.size + sizeof(*pktList);
                v->vid_pkt_queue.packets++;
            }
        }
        else
        {
            av_free_packet( &pkt );
        }

        // NOTE zie FIXME boven
        if( v->idx_audio > -1 && v->idx_video > -1 &&
            v->aud_pkt_queue.size > v->aud_pkt_queue.max_size &&
            v->vid_pkt_queue.size > v->vid_pkt_queue.max_size ) {
                return;
        }
        else if( v->idx_audio > -1 && v->idx_video < 0 &&
                 v->aud_pkt_queue.size > v->aud_pkt_queue.max_size ) {
                    return;
        }
        else if( v->idx_video > -1 && v->idx_audio < 0 &&
                 v->vid_pkt_queue.size > v->vid_pkt_queue.max_size ) {
                    return;
        }
        else if( v->idx_video < 0 && v->idx_audio < 0 ) {
            return;
        }
    }
}


static int vineoNextDataAudio( Vineo *v, void *data, int length )
{
    int dec = 0;

    while( dec < length )
    {
        // If there's any pending decoded data, deal with it first
        if( v->dec_data_size > 0 )
        {
            // Get the amount of bytes remaining to be written, and clamp to
            // the amount of decoded data we have
            size_t rem = length - dec;

            if( rem > v->dec_data_size ) {
                rem = v->dec_data_size;
            }

            // Copy the data to the app's buffer and increment
            memcpy( data, v->dec_data, rem );
            data = (char*)data + rem;
            dec += rem;

            // If there's any decoded data left, move it to the front of the
            // buffer for next time
            if( rem < v->dec_data_size ) {
                memmove( v->dec_data, &v->dec_data[rem], v->dec_data_size - rem );
            }

            v->dec_data_size -= rem;
        }

        // Check if we need to get more decoded data
        if( v->dec_data_size == 0 )
        {
            size_t insize = v->data_size;
            int size, len;

            if( insize == 0 )
            {
                vineoNextPacketAudio( v );

                // If there's no more input data, break and return what we have
                if( v->data_size == 0 ) {
                    break;
                }

                insize = v->data_size;
                memset( &v->data[insize], 0, FF_INPUT_BUFFER_PADDING_SIZE );
            }

            // Clear the input padding bits
            // Decode some data, and check for errors
            size = AVCODEC_MAX_AUDIO_FRAME_SIZE;

            while( ( len = avcodec_decode_audio2( v->aud_codec_ctx, (int16_t*)v->dec_data, &size, (unsigned char*)v->data, insize ) ) == 0 )
            {
                if( size > 0 ) {
                    break;
                }

                vineoNextPacketAudio( v );

                if( insize == v->data_size ) {
                    break;
                }

                insize = v->data_size;
                memset( &v->data[insize], 0, FF_INPUT_BUFFER_PADDING_SIZE );
            }

            // we got an error! return what we got.
            if( len < 0 ) {
                break;
            }

            if( len > 0 )
            {
                // If any input data is left, move it to the start of the
                // buffer, and decrease the buffer size

                size_t rem = insize - len;

                if( rem ) {
                    memmove( v->data, &v->data[len], rem );
                }

                v->data_size = rem;
            }

            // Set the output buffer size
            v->dec_data_size = size;
        }
    }

    // Return the number of bytes we were able to get
    return dec;
}



static int vineoNextPacket( Vineo *v, int stream, AVPacket *retPkt )
{
    AVPacketList *pktList;

    if( stream == v->idx_audio )
    {
        pktList = v->aud_pkt_queue.first;

        if( pktList )
        {
            v->aud_pkt_queue.first = pktList->next;

            if( !v->aud_pkt_queue.first ) {
                v->aud_pkt_queue.last = NULL;
            }

            v->aud_pkt_queue.size -= ( pktList->pkt.size + sizeof(*pktList) );
            v->aud_pkt_queue.packets--;
            *retPkt = pktList->pkt;
            av_free( pktList );
            return 1;
        }
    }
    else if( stream == v->idx_video )
    {
        pktList = v->vid_pkt_queue.first;

        if( pktList )
        {
            v->vid_pkt_queue.first = pktList->next;

            if( !v->vid_pkt_queue.first ) {
                v->vid_pkt_queue.last = NULL;
            }

            v->vid_pkt_queue.size -= ( pktList->pkt.size + sizeof(*pktList) );
            v->vid_pkt_queue.packets--;
            *retPkt = pktList->pkt;
            av_free( pktList );
            return 1;
        }
    }

    return 0;
}



static void vineoNextPacketAudio( Vineo *v )
{
    AVPacket pkt;
    //double pts = 0.0f;

    //while( getNextPacket( stream->fmt_ctx, stream->index, &pkt ) )
    while( vineoNextPacket( v, v->idx_audio, &pkt ) )
    {
        //if( stream->index != pkt.stream_index )
        //{
        //    av_av_free_packet( &pkt );
        //    continue;
        //}

        /*
        if( pkt.pts != AV_NOPTS_VALUE )
        {
            //printf( "FROM PACKET DTS,   " );
            pts = pkt.pts;
        }
        else
        {
            //printf( "NO PTS,            " );
            pts = 0.0f;
        }

        pts *= av_q2d( stream->fmt_ctx->streams[stream->index]->time_base );
        pts -= g_start_timeOffset;

        if( pts >= g_timeCur )
        {
            printf( "getNextAudioPacket pts: %f, gpts: %f\n", pts, g_timeCur );
        */

            size_t idx = v->data_size;

            // Found the stream. Grow the input data buffer as needed to
            // hold the new packet's data. Additionally, some ffmpeg codecs
            // need some padding so they don't overread the allocated buffer
            if( idx + pkt.size > v->data_size_max )
            {
                void *tmp = av_realloc( v->data, idx + pkt.size + FF_INPUT_BUFFER_PADDING_SIZE );

                if( !tmp ) {
                    break;
                }

                v->data = tmp;
                v->data_size_max = idx + pkt.size;
            }

            // Copy the packet and av_free it
            memcpy( &v->data[idx], pkt.data, pkt.size );
            v->data_size += pkt.size;

            av_free_packet( &pkt );
            break;
        /*
        }

        av_free_packet( &pkt );
        */
    }
}



Vineo *vineoNew()
{
    Vineo *v = av_malloc( sizeof( Vineo ) );

    if( !v )
    {
        printf( "Error @ vineoNew() @ av_malloc()\n" );
        return NULL;
    }

    memset( v, 0, sizeof( Vineo ) );

    v->aud_pkt_queue.max_size = MAX_AUDIOQ_SIZE;
    v->vid_pkt_queue.max_size = MAX_VIDEOQ_SIZE;

    v->idx_audio = -1;
    v->idx_video = -1;

    // generate OpenGL texture
    glGenTextures( 1, &v->tex_gl );

    // generate OpenAL source and buffers and set parameters so mono sources won't distance attenuate
    alGenSources( 1, &v->aud_src_al );
    alGenBuffers( NUM_BUFFERS, v->aud_buf_al );
    alSourcei( v->aud_src_al, AL_SOURCE_RELATIVE, AL_TRUE );
    alSourcei( v->aud_src_al, AL_ROLLOFF_FACTOR, 0 );

    return v;
}



void vineoOpen( Vineo *v, char *file )
{
    // NOTE FIXME als er iets mis gaat in deze functie, netjes unloaden


    // NOTE av_register_all heeft zelf al een check of het al is registered.
    av_register_all();


    // open container file
    if( av_open_input_file( &v->fmt_ctx, file, NULL, 0, NULL ) != 0 )
    {
        printf( "Error @ vineoOpen() @ av_open_input_file()\n" );
        return;
    }

    if( av_find_stream_info( v->fmt_ctx ) < 0 )
    {
        printf( "Error @ vineoOpen() @ av_find_stream_info()\n" );
        return;
    }


    //dump_format( v->fmt_ctx, 0, file, 0 );


    // doe we have a start time offset?
    if( v->fmt_ctx->start_time != AV_NOPTS_VALUE ) {
		v->start_time = v->fmt_ctx->start_time;
	}


    // find streams
    int i;

    for( i = 0; i < v->fmt_ctx->nb_streams; i++ )
    {
        if( v->fmt_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO && v->idx_video < 0 ) {
            v->idx_video = i;
        }

         if( v->fmt_ctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && v->idx_audio < 0) {
            v->idx_audio = i;
        }
    }


    // open video codec
    if( v->idx_video >= 0 )
    {
        v->vid_codec_ctx = v->fmt_ctx->streams[v->idx_video]->codec;
        v->vid_codec = avcodec_find_decoder( v->vid_codec_ctx->codec_id );

        if( !v->vid_codec )
        {
            printf( "Error @ vineoOpen() @ avcodec_find_decoder()\n" );
            v->idx_video = -1;
        }
        else if( avcodec_open( v->vid_codec_ctx, v->vid_codec ) < 0 )
        {
            printf( "Error @ vineoOpen() @ avcodec_open()\n" );
            v->idx_video = -1;
        }

        v->sws = sws_getContext(
            v->vid_codec_ctx->width,
            v->vid_codec_ctx->height,
            v->vid_codec_ctx->pix_fmt,
            v->vid_codec_ctx->width,
            v->vid_codec_ctx->height,
            PIX_FMT_RGBA32,
            SWS_FAST_BILINEAR,
            NULL,
            NULL,
            NULL
        );
    }


    // open audio codec
    if( v->idx_audio >= 0 )
    {
        v->aud_codec_ctx = v->fmt_ctx->streams[v->idx_audio]->codec;
        v->aud_codec = avcodec_find_decoder( v->aud_codec_ctx->codec_id );

        if( !v->aud_codec )
        {
            printf( "Error @ vineoOpen() @ avcodec_find_decoder()\n" );
            v->idx_audio = -1;
        }
        else if( avcodec_open( v->aud_codec_ctx, v->aud_codec ) < 0 )
        {
            printf( "Error @ vineoOpen() @ avcodec_open()\n" );
            v->idx_audio = -1;
        }
    }


    // allocate video frames for decoding
    if( v->idx_video >= 0 )
    {
        v->frame = avcodec_alloc_frame();
        v->frame_rgba = avcodec_alloc_frame();

        if( !v->frame_rgba )
        {
            printf( "Error @ vineoOpen() @ avcodec_alloc_frame()\n" );
            return;
        }

        int b = avpicture_get_size( PIX_FMT_RGBA32, v->vid_codec_ctx->width, v->vid_codec_ctx->height );
        v->frame_buffer = av_malloc( b * sizeof(unsigned char) );

        if( !v->frame_buffer )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }

        avpicture_fill( (AVPicture *)v->frame_rgba, v->frame_buffer, PIX_FMT_RGBA32, v->vid_codec_ctx->width, v->vid_codec_ctx->height );
    }


    // init audio
    if( v->idx_audio >= 0 )
    {
        v->aud_rate = v->aud_codec_ctx->sample_rate;
        v->aud_channels = v->aud_codec_ctx->channels;
        v->aud_bits = 16; // NOTE ffmpeg gebruikt altijd 16 bits
        v->aud_format = 0;

        if( v->aud_channels == 1 ) {
            v->aud_format = AL_FORMAT_MONO16;
        }

        if( v->aud_channels == 2 ) {
            v->aud_format = AL_FORMAT_STEREO16;
        }

        if( alIsExtensionPresent("AL_EXT_MCFORMATS") )
        {
            if( v->aud_channels == 4 ) {
                v->aud_format = alGetEnumValue("AL_FORMAT_QUAD16");
            }

            if( v->aud_channels == 6 ) {
                v->aud_format = alGetEnumValue("AL_FORMAT_51CHN16");
            }
        }

        if( v->aud_format == 0 )
        {
            printf( "Error @ vineoOpen() @ Unhandled format (%d channels, %d bits)\n", v->aud_channels, v->aud_bits );
            return;
        }


        v->buffer_playing = 0;
        v->data = NULL;
        v->data_size = 0;
        v->data_size_max = 0;
        v->dec_data = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
        v->dec_data_size = 0;

        if( !v->dec_data )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }

        v->data_tmp = av_malloc( BUFFER_SIZE );

        if( !v->data_tmp )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }
    }
}


/*
VineoVideoPicture *vineoPicture( Vineo *v )
{
    VineoVideoPicture *vp = v->vid_buffer.first;

    while( vp )
    {
        if( !vp->next ) {
            return vp;
        }

        if( v->time < vp->next->pts ) {
            return vp;
        }

        vp = vp->next;
    }

    return NULL;
}
*/


void vineoPlay( Vineo *v )
{
    if( !v ) {
        return;
    }


    vineoFillPacketQueue( v );

    // pre buffer audio
    if( v->idx_audio >= 0 )
    {
        int i, count = 0;

        for( i = 0; i < NUM_BUFFERS; i++ )
        {
            count = vineoNextDataAudio( v, v->data_tmp, BUFFER_SIZE );

            if( count <= 0 ) {
                break;
            }

            alBufferData( v->aud_buf_al[i], v->aud_format, v->data_tmp, count, v->aud_rate );
            alSourceQueueBuffers( v->aud_src_al, 1, &v->aud_buf_al[i] );
            v->buffer_playing += count;
        }

        alSourcePlay( v->aud_src_al );
    }

    v->time_offset = av_gettime();
    v->time = 0;
    v->is_playing = 1;
}



void vineoVolume( Vineo *v, ALfloat vol )
{
    alSourcef( v->aud_src_al, AL_GAIN, vol );
}
