#include "vineo.h"



static void vineoFillPacketQueue( Vineo *v );
static void vineoFreeData( Vineo *v );
static int vineoNextDataAudio( Vineo *v, void *data, int length );
static int vineoNextPacket( Vineo *v, int stream, AVPacket *retPkt );
static void vineoNextPacketAudio( Vineo *v );



void vineoClose( Vineo *v )
{
    if( !v ) {
        return;
    }


    // clear video packets
    AVPacketList *tmp, *pktList = v->audPktQueue.first;

    while( pktList )
    {
        tmp = pktList;
        pktList = tmp->next;

        av_free_packet( &tmp->pkt );
        av_free( tmp );
    }


    // clear audio packets
    pktList = v->vidPktQueue.first;

    while( pktList )
    {
        tmp = pktList;
        pktList = tmp->next;

        av_free_packet( &tmp->pkt );
        av_free( tmp );
    }


    // clear video frames
    VineoVideoPicture *next;
    VineoVideoPicture *cur = v->vidBuffer.first;

    while( cur )
    {
        next = cur->next;
        av_free( cur->data );
        av_free( cur );
        cur = next;
    }


    // clear OpenAL en OpenGL resources
    glDeleteTextures( 1, &v->texGL );
    alSourceStop( v->audSrcAL );                // stop the source or else the buffer won't be freed
    alSourcei( v->audSrcAL, AL_BUFFER, 0 );     // free bufferdata that still exists
    alDeleteBuffers( NUM_BUFFERS, v->audBufAL );
    alDeleteSources( 1, &v->audSrcAL );


    // clear ffmpeg video stream
    if( v->idxVideo >= 0 )
    {
        sws_freeContext( v->sws );
        av_free( v->frameBuffer );
        av_free( v->frameRGB );
        av_free( v->frame );
        avcodec_close( v->vidCodecCtx );
    }


    // clear ffmpeg audio stream
    if( v->idxAudio >= 0 )
    {
        av_free( v->decData );
        av_free( v->data );
        av_free( v->dataTmp );
        avcodec_close( v->audCodecCtx );
    }


    // close ffmpeg context
    if( v->fmtCtx ) {
        av_close_input_file( v->fmtCtx );
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
    v->time = av_gettime() - v->timeOffset + v->startTime;



    double pts = 0.0f;
    int64_t pts64 = 0;
    AVPacket pkt;
    int frameFinished = 0;
    VineoVideoPicture *vp;


    // fill the video buffer
    while( v->vidBuffer.size < v->vidBuffer.maxSize )
    {
        if( !vineoNextPacket( v, v->idxVideo, &pkt ) ) {
            break;
        }

        //v->vidCodecCtx->reordered_opaque = packet.pts;

        avcodec_decode_video( v->vidCodecCtx, v->frame, &frameFinished, pkt.data, pkt.size );

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

        pts *= av_q2d( v->fmtCtx->streams[v->idxVideo]->time_base );
        pts64 = pts * AV_TIME_BASE;

        if( frameFinished && ( pts64 >= v->time || v->vidBuffer.size == 0 ) )
        {
            sws_scale(
                v->sws,
                v->frame->data,
                v->frame->linesize,
                0,
                v->vidCodecCtx->height,
                v->frameRGB->data,
                v->frameRGB->linesize
            );

            vp = av_malloc( sizeof(VineoVideoPicture) );

            if( !vp )
            {
                printf( "Error @ av_malloc()\n" );
                break;
            }

            vp->next = NULL;
            vp->prev = NULL;
            vp->pts = pts64;
            vp->width = v->vidCodecCtx->width;
            vp->height = v->vidCodecCtx->height;
            vp->dataSize = vp->width * vp->height * 4; // alleen voor RGBA
            vp->data = av_malloc( vp->dataSize );

            if( !vp->data )
            {
                printf( "Error @ av_malloc()\n" );
                break;
            }

            memcpy( vp->data, v->frameRGB->data[0], vp->dataSize );


            // FIXME swap red <-> blue, waarom moeten we swappen bij PIX_FMT_RGBA32 -> GL_RGBA???
            char *a = vp->data;
            char *b = a + 2;
            char *end = &vp->data[vp->dataSize];

            for( ; a < end; a += 4, b += 4 )
            {
                *a ^= *b;
                *b ^= *a;
                *a ^= *b;
            }


            if( !v->vidBuffer.last ) {
                v->vidBuffer.first = vp;
            }
            else {
                v->vidBuffer.last->next = vp;
            }

            v->vidBuffer.last = vp;
            v->vidBuffer.size++;
        }

        av_free_packet( &pkt );
    }


    // update video picture
    vp = vineoPicture( v );

    if( vp != NULL && vp != v->curVP )
    {
        // NOTE is er een snellere method voor draw-too-texture, glTexSubImage2D
        glBindTexture( GL_TEXTURE_2D, v->texGL );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, vp->width, vp->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, vp->data );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

        v->curVP = vp;

        // NOTE FIXME glDrawPixels werkt niet? http://www.glprogramming.com/red/chapter08.html#name6
        /*
        glShadeModel( GL_FLAT );
        glRasterPos2i( 0, 0 );
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
        glDrawPixels( vpp->width, vpp->height, GL_RGB, GL_UNSIGNED_BYTE, vpp->data );
        glFlush();
        */
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
        alGetSourcei( v->audSrcAL, AL_BUFFERS_PROCESSED, &processed );

        if( processed == 0 )
        {
            alGetSourcei( v->audSrcAL, AL_SOURCE_STATE, &state );

            if( alGetError() != AL_NO_ERROR )
            {
                printf( "Error checking source state...\n" );
                break;
            }

            if( state != AL_PLAYING )
            {
                alSourcePlay( v->audSrcAL );

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

            count = vineoNextDataAudio( v, v->dataTmp, BUFFER_SIZE );

            v->bufferPlaying += count;
            clock = (float)v->bufferPlaying / (float)( ( v->audBits / 8 ) * v->audChannels * v->audRate ) * AV_TIME_BASE;
        }
        while( v->time > clock && count > 0 );



        if( count > 0 )
        {
            alSourceUnqueueBuffers( v->audSrcAL, 1, &buf );

            if( buf != 0 )
            {
                if( fixAudioSync )
                {
                    //double timeFix = clock - g_timeCur;
                    double timeFix = clock - v->time;
                    count = timeFix * ( (float)( ( v->audBits / 8 ) * v->audChannels * v->audRate ) );
                    // round to nice buffer length
                    count /= ( ( v->audBits / 8 ) * v->audChannels );
                    count *= ( ( v->audBits / 8 ) * v->audChannels );
                    //count = BUFFER_SIZE - count;
                    printf( "- fix audio syncing, count: %i, time: %f\n", count, timeFix );
                }

                alBufferData( buf, v->audFormat, v->dataTmp, count, v->audRate );
                alSourceQueueBuffers( v->audSrcAL, 1, &buf );
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


    // free packets A/V data that we don't need anymore
    vineoFreeData( v );
}



static void vineoFillPacketQueue( Vineo *v )
{
    // NOTE FIXME lelijk code! kan vast anders
    if( v->idxAudio > -1 && v->idxVideo > -1 &&
        v->audPktQueue.size > v->audPktQueue.maxSize &&
        v->vidPktQueue.size > v->vidPktQueue.maxSize ) {
            return;
    }
    else if( v->idxAudio > -1 && v->idxVideo < 0 &&
             v->audPktQueue.size > v->audPktQueue.maxSize ) {
                return;
    }
    else if( v->idxVideo > -1 && v->idxAudio < 0 &&
             v->vidPktQueue.size > v->vidPktQueue.maxSize ) {
                return;
    }
    else if( v->idxVideo < 0 && v->idxAudio < 0 ) {
        return;
    }

    AVPacket pkt;
    AVPacketList *pktList;

    while( av_read_frame( v->fmtCtx, &pkt ) >= 0 )
    {
        if( pkt.stream_index == v->idxAudio )
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

                if( !v->audPktQueue.last ) {
                    v->audPktQueue.first = pktList;
                }
                else {
                    v->audPktQueue.last->next = pktList;
                }

                v->audPktQueue.last = pktList;
                v->audPktQueue.size += pkt.size + sizeof(*pktList);
                v->audPktQueue.packets++;
            }
        }
        else if( pkt.stream_index == v->idxVideo )
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

                if( !v->vidPktQueue.last ) {
                    v->vidPktQueue.first = pktList;
                }
                else {
                    v->vidPktQueue.last->next = pktList;
                }

                v->vidPktQueue.last = pktList;
                v->vidPktQueue.size += pkt.size + sizeof(*pktList);
                v->vidPktQueue.packets++;
            }
        }
        else
        {
            av_free_packet( &pkt );
        }

        // NOTE zie FIXME boven
        if( v->idxAudio > -1 && v->idxVideo > -1 &&
            v->audPktQueue.size > v->audPktQueue.maxSize &&
            v->vidPktQueue.size > v->vidPktQueue.maxSize ) {
                return;
        }
        else if( v->idxAudio > -1 && v->idxVideo < 0 &&
                 v->audPktQueue.size > v->audPktQueue.maxSize ) {
                    return;
        }
        else if( v->idxVideo > -1 && v->idxAudio < 0 &&
                 v->vidPktQueue.size > v->vidPktQueue.maxSize ) {
                    return;
        }
        else if( v->idxVideo < 0 && v->idxAudio < 0 ) {
            return;
        }
    }
}



static void vineoFreeData( Vineo *v )
{
    VineoVideoPicture *next;
    VineoVideoPicture *cur = v->vidBuffer.first;

    while( cur )
    {
        if( !cur->next ) {
            return;
        }

        if( v->time < cur->next->pts ) {
            return;
        }

        next = cur->next;
        av_free( cur->data );
        av_free( cur );
        v->vidBuffer.first = next;
        v->vidBuffer.size--;

        cur = next;
    }
}



static int vineoNextDataAudio( Vineo *v, void *data, int length )
{
    int dec = 0;

    while( dec < length )
    {
        // If there's any pending decoded data, deal with it first
        if( v->decDataSize > 0 )
        {
            // Get the amount of bytes remaining to be written, and clamp to
            // the amount of decoded data we have
            size_t rem = length - dec;

            if( rem > v->decDataSize ) {
                rem = v->decDataSize;
            }

            // Copy the data to the app's buffer and increment
            memcpy( data, v->decData, rem );
            data = (char*)data + rem;
            dec += rem;

            // If there's any decoded data left, move it to the front of the
            // buffer for next time
            if( rem < v->decDataSize ) {
                memmove( v->decData, &v->decData[rem], v->decDataSize - rem );
            }

            v->decDataSize -= rem;
        }

        // Check if we need to get more decoded data
        if( v->decDataSize == 0 )
        {
            size_t insize = v->dataSize;
            int size, len;

            if( insize == 0 )
            {
                vineoNextPacketAudio( v );

                // If there's no more input data, break and return what we have
                if( v->dataSize == 0 ) {
                    break;
                }

                insize = v->dataSize;
                memset( &v->data[insize], 0, FF_INPUT_BUFFER_PADDING_SIZE );
            }

            // Clear the input padding bits
            // Decode some data, and check for errors
            size = AVCODEC_MAX_AUDIO_FRAME_SIZE;

            while( ( len = avcodec_decode_audio2( v->audCodecCtx, (int16_t*)v->decData, &size, (uint8_t*)v->data, insize ) ) == 0 )
            {
                if( size > 0 ) {
                    break;
                }

                vineoNextPacketAudio( v );

                if( insize == v->dataSize ) {
                    break;
                }

                insize = v->dataSize;
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

                v->dataSize = rem;
            }

            // Set the output buffer size
            v->decDataSize = size;
        }
    }

    // Return the number of bytes we were able to get
    return dec;
}



static int vineoNextPacket( Vineo *v, int stream, AVPacket *retPkt )
{
    AVPacketList *pktList;

    if( stream == v->idxAudio )
    {
        pktList = v->audPktQueue.first;

        if( pktList )
        {
            v->audPktQueue.first = pktList->next;

            if( !v->audPktQueue.first ) {
                v->audPktQueue.last = NULL;
            }

            v->audPktQueue.size -= ( pktList->pkt.size + sizeof(*pktList) );
            v->audPktQueue.packets--;
            *retPkt = pktList->pkt;
            av_free( pktList );
            return 1;
        }
    }
    else if( stream == v->idxVideo )
    {
        pktList = v->vidPktQueue.first;

        if( pktList )
        {
            v->vidPktQueue.first = pktList->next;

            if( !v->vidPktQueue.first ) {
                v->vidPktQueue.last = NULL;
            }

            v->vidPktQueue.size -= ( pktList->pkt.size + sizeof(*pktList) );
            v->vidPktQueue.packets--;
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

    //while( getNextPacket( stream->fmtCtx, stream->index, &pkt ) )
    while( vineoNextPacket( v, v->idxAudio, &pkt ) )
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

        pts *= av_q2d( stream->fmtCtx->streams[stream->index]->time_base );
        pts -= g_startTimeOffset;

        if( pts >= g_timeCur )
        {
            printf( "getNextAudioPacket pts: %f, gpts: %f\n", pts, g_timeCur );
        */

            size_t idx = v->dataSize;

            // Found the stream. Grow the input data buffer as needed to
            // hold the new packet's data. Additionally, some ffmpeg codecs
            // need some padding so they don't overread the allocated buffer
            if( idx + pkt.size > v->dataSizeMax )
            {
                void *tmp = av_realloc( v->data, idx + pkt.size + FF_INPUT_BUFFER_PADDING_SIZE );

                if( !tmp ) {
                    break;
                }

                v->data = tmp;
                v->dataSizeMax = idx + pkt.size;
            }

            // Copy the packet and av_free it
            memcpy( &v->data[idx], pkt.data, pkt.size );
            v->dataSize += pkt.size;

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

    v->audPktQueue.maxSize = MAX_AUDIOQ_SIZE;
    v->vidPktQueue.maxSize = MAX_VIDEOQ_SIZE;
    v->vidBuffer.maxSize = VID_BUFFER_SIZE;

    v->idxAudio = -1;
    v->idxVideo = -1;

    // generate OpenGL texture
    glGenTextures( 1, &v->texGL );

    // generate OpenAL source and buffers and set parameters so mono sources won't distance attenuate
    alGenSources( 1, &v->audSrcAL );
    alGenBuffers( NUM_BUFFERS, v->audBufAL );
    alSourcei( v->audSrcAL, AL_SOURCE_RELATIVE, AL_TRUE );
    alSourcei( v->audSrcAL, AL_ROLLOFF_FACTOR, 0 );

    return v;
}



void vineoOpen( Vineo *v, char *file )
{
    // NOTE FIXME als er iets mis gaat in deze functie, netjes unloaden


    // NOTE av_register_all heeft zelf al een check of het al is registered.
    av_register_all();


    // open container file
    if( av_open_input_file( &v->fmtCtx, file, NULL, 0, NULL ) != 0 )
    {
        printf( "Error @ vineoOpen() @ av_open_input_file()\n" );
        return;
    }

    if( av_find_stream_info( v->fmtCtx ) < 0 )
    {
        printf( "Error @ vineoOpen() @ av_find_stream_info()\n" );
        return;
    }


    //dump_format( v->fmtCtx, 0, file, 0 );


    // doe we have a start time offset?
    if( v->fmtCtx->start_time != AV_NOPTS_VALUE ) {
		v->startTime = v->fmtCtx->start_time;
	}


    // find streams
    int i;

    for( i = 0; i < v->fmtCtx->nb_streams; i++ )
    {
        if( v->fmtCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO && v->idxVideo < 0 ) {
            v->idxVideo = i;
        }

         if( v->fmtCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && v->idxAudio < 0) {
            v->idxAudio = i;
        }
    }


    // open video codec
    if( v->idxVideo >= 0 )
    {
        v->vidCodecCtx = v->fmtCtx->streams[v->idxVideo]->codec;
        v->vidCodec = avcodec_find_decoder( v->vidCodecCtx->codec_id );

        if( !v->vidCodec )
        {
            printf( "Error @ vineoOpen() @ avcodec_find_decoder()\n" );
            v->idxVideo = -1;
        }
        else if( avcodec_open( v->vidCodecCtx, v->vidCodec ) < 0 )
        {
            printf( "Error @ vineoOpen() @ avcodec_open()\n" );
            v->idxVideo = -1;
        }

        v->sws = sws_getContext(
            v->vidCodecCtx->width,
            v->vidCodecCtx->height,
            v->vidCodecCtx->pix_fmt,
            v->vidCodecCtx->width,
            v->vidCodecCtx->height,
            PIX_FMT_RGBA32,
            SWS_FAST_BILINEAR,
            NULL,
            NULL,
            NULL
        );
    }


    // open audio codec
    if( v->idxAudio >= 0 )
    {
        v->audCodecCtx = v->fmtCtx->streams[v->idxAudio]->codec;
        v->audCodec = avcodec_find_decoder( v->audCodecCtx->codec_id );

        if( !v->audCodec )
        {
            printf( "Error @ vineoOpen() @ avcodec_find_decoder()\n" );
            v->idxAudio = -1;
        }
        else if( avcodec_open( v->audCodecCtx, v->audCodec ) < 0 )
        {
            printf( "Error @ vineoOpen() @ avcodec_open()\n" );
            v->idxAudio = -1;
        }
    }


    // allocate video frames for decoding
    if( v->idxVideo >= 0 )
    {
        v->frame = avcodec_alloc_frame();
        v->frameRGB = avcodec_alloc_frame();

        if( !v->frameRGB )
        {
            printf( "Error @ vineoOpen() @ avcodec_alloc_frame()\n" );
            return;
        }

        int b = avpicture_get_size( PIX_FMT_RGBA32, v->vidCodecCtx->width, v->vidCodecCtx->height );
        v->frameBuffer = av_malloc( b * sizeof(uint8_t) );

        if( !v->frameBuffer )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }

        avpicture_fill( (AVPicture *)v->frameRGB, v->frameBuffer, PIX_FMT_RGBA32, v->vidCodecCtx->width, v->vidCodecCtx->height );
    }


    // init audio
    if( v->idxAudio >= 0 )
    {
        v->audRate = v->audCodecCtx->sample_rate;
        v->audChannels = v->audCodecCtx->channels;
        v->audBits = 16; // NOTE ffmpeg gebruikt altijd 16 bits
        v->audFormat = 0;

        if( v->audChannels == 1 ) {
            v->audFormat = AL_FORMAT_MONO16;
        }

        if( v->audChannels == 2 ) {
            v->audFormat = AL_FORMAT_STEREO16;
        }

        if( alIsExtensionPresent("AL_EXT_MCFORMATS") )
        {
            if( v->audChannels == 4 ) {
                v->audFormat = alGetEnumValue("AL_FORMAT_QUAD16");
            }

            if( v->audChannels == 6 ) {
                v->audFormat = alGetEnumValue("AL_FORMAT_51CHN16");
            }
        }

        if( v->audFormat == 0 )
        {
            printf( "Error @ vineoOpen() @ Unhandled format (%d channels, %d bits)\n", v->audChannels, v->audBits );
            return;
        }


        v->bufferPlaying = 0;
        v->data = NULL;
        v->dataSize = 0;
        v->dataSizeMax = 0;
        v->decData = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
        v->decDataSize = 0;

        if( !v->decData )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }

        v->dataTmp = av_malloc( BUFFER_SIZE );

        if( !v->dataTmp )
        {
            printf( "Error @ vineoOpen() @ av_malloc()\n" );
            return;
        }
    }
}



VineoVideoPicture *vineoPicture( Vineo *v )
{
    VineoVideoPicture *vp = v->vidBuffer.first;

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



void vineoPlay( Vineo *v )
{
    if( !v ) {
        return;
    }


    vineoFillPacketQueue( v );

    // pre buffer audio
    if( v->idxAudio >= 0 )
    {
        int i, count = 0;

        for( i = 0; i < NUM_BUFFERS; i++ )
        {
            count = vineoNextDataAudio( v, v->dataTmp, BUFFER_SIZE );

            if( count <= 0 ) {
                break;
            }

            alBufferData( v->audBufAL[i], v->audFormat, v->dataTmp, count, v->audRate );
            alSourceQueueBuffers( v->audSrcAL, 1, &v->audBufAL[i] );
            v->bufferPlaying += count;
        }

        alSourcePlay( v->audSrcAL );
    }

    v->timeOffset = av_gettime();
    v->time = 0;
    v->isPlaying = 1;
}



void vineoVolume( Vineo *v, ALfloat vol )
{
    alSourcef( v->audSrcAL, AL_GAIN, vol );
}
