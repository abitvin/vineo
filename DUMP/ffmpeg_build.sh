#!/bin/sh

#../configure --help
#../configure --list-decoders
#../configure --list-demuxers
#../configure --list-parsers
#../configure --list-bsfs
#../configure --list-protocols
#../configure --list-filters


# --enable-w32threads    --enable-pthreads    --target-os=mingw32
a=" --disable-static --enable-shared --disable-ffmpeg --disable-ffplay --disable-ffserver --disable-vhook --enable-memalign-hack"

deco="--disable-decoders --enable-decoder=aac --enable-decoder=flv --enable-decoder=gif --enable-decoder=h264 --enable-decoder=mjpeg --enable-decoder=mp3 --enable-decoder=mpeg4 --enable-decoder=png"

enco="--disable-encoders"

muxe="--disable-muxers"

demu="--disable-demuxers --enable-demuxer=aac --enable-demuxer=flv --enable-demuxer=h264 --enable-demuxer=image2  --enable-demuxer=mjpeg --enable-demuxer=mov --enable-demuxer=mp3"

pars="--disable-parsers --enable-parser=aac --enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio"

bsfs="--disable-bsfs"

prot="--disable-protocols --enable-protocol=file --enable-protocol=http --enable-protocol=tcp"

filt="--disable-filters"

devi="--disable-devices"

comp="--arch=amd64" 	# i386, i686, x86, x86_64, etc

debu="--disable-debug"

#echo $a $deco $enco $muxe $demu $pars $bsfs $prot $filt $devi $comp $debu

../configure $a $deco $enco $muxe $demu $pars $bsfs $prot $filt $devi $comp $deb