#!/bin/sh

#../configure --help
#../configure --list-decoders
#../configure --list-demuxers
#../configure --list-parsers
#../configure --list-bsfs
#../configure --list-protocols
#../configure --list-filters


# MEMALIGN-HACK INFO
# The --enable-memalign-hack option is necessary for FFmpeg to run MMX and SSE-optimized code on Windows.



# WINDOWS
# a="--disable-ffmpeg --disable-ffplay --disable-ffserver --disable-vhook --enable-w32threads --enable-memalign-hack"

# LINUX
a="--disable-ffmpeg --disable-ffplay --disable-ffserver --disable-vhook --enable-pthreads"



deco="--disable-decoders --enable-decoder=aac --enable-decoder=flv --enable-decoder=gif --enable-decoder=h264 --enable-decoder=mjpeg --enable-decoder=mp3 --enable-decoder=mpeg4 --enable-decoder=png"

enco="--disable-encoders"

muxe="--disable-muxers"

demu="--disable-demuxers --enable-demuxer=aac --enable-demuxer=flv --enable-demuxer=h264 --enable-demuxer=image2  --enable-demuxer=mjpeg --enable-demuxer=mov --enable-demuxer=mp3"

pars="--disable-parsers --enable-parser=aac --enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio"

bsfs="--disable-bsfs"

prot="--disable-protocols --enable-protocol=file --enable-protocol=http --enable-protocol=tcp"

filt="--disable-filters"

devi="--disable-devices"

comp="--arch=amd64 --cpu=core2" 	# i386, i686, amd64, x86_64, etc

debu="--disable-debug"



#echo $a $deco $enco $muxe $demu $pars $bsfs $prot $filt $devi $comp $debu
../configure $a $deco $enco $muxe $demu $pars $bsfs $prot $filt $devi $comp $debu