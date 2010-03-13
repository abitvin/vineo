# Automatically generated by configure - do not modify!
FFMPEG_CONFIGURATION=--disable-ffmpeg --disable-ffplay --disable-ffserver --disable-vhook --enable-w32threads --enable-memalign-hack --disable-decoders --enable-decoder=aac --enable-decoder=flv --enable-decoder=gif --enable-decoder=h264 --enable-decoder=mjpeg --enable-decoder=mp3 --enable-decoder=mpeg4 --enable-decoder=png --disable-encoders --disable-muxers --disable-demuxers --enable-demuxer=aac --enable-demuxer=flv --enable-demuxer=h264 --enable-demuxer=image2 --enable-demuxer=mjpeg --enable-demuxer=mov --enable-demuxer=mp3 --disable-parsers --enable-parser=aac --enable-parser=h264 --enable-parser=mpeg4video --disable-bsfs --disable-protocols --enable-protocol=file --enable-protocol=http --enable-protocol=tcp --disable-filters --disable-devices --arch=i686 --disable-debug
prefix=/usr/local
LIBDIR=$(DESTDIR)${prefix}/lib
SHLIBDIR=$(DESTDIR)${prefix}/lib
INCDIR=$(DESTDIR)${prefix}/include
BINDIR=$(DESTDIR)${prefix}/bin
DATADIR=$(DESTDIR)${prefix}/share/ffmpeg
MANDIR=$(DESTDIR)${prefix}/share/man
CC=gcc
YASM=yasm
AR=ar
RANLIB=ranlib
LN_S=ln -sf
STRIP=strip
OPTFLAGS= -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112 -std=c99 -fomit-frame-pointer -Wdeclaration-after-statement -Wall -Wno-switch -Wdisabled-optimization -Wpointer-arith -Wredundant-decls -Wno-pointer-sign -Wcast-qual -Wwrite-strings -Wtype-limits -Wundef -O3 -fno-math-errno -fno-signed-zeros
VHOOKCFLAGS=-fPIC
LDFLAGS=  -Wl,--warn-common -Wl,--as-needed -Wl,-rpath-link,$(BUILD_ROOT)/libpostproc -Wl,-rpath-link,$(BUILD_ROOT)/libswscale -Wl,-rpath-link,$(BUILD_ROOT)/libavfilter -Wl,-rpath-link,$(BUILD_ROOT)/libavdevice -Wl,-rpath-link,$(BUILD_ROOT)/libavformat -Wl,-rpath-link,$(BUILD_ROOT)/libavcodec -Wl,-rpath-link,$(BUILD_ROOT)/libavutil -Wl,-Bsymbolic
FFSERVERLDFLAGS=-Wl,-E
SHFLAGS=-shared -Wl,-soname,$$(@F)
YASMFLAGS=-f elf -DARCH_X86_32
VHOOKSHFLAGS=$(SHFLAGS)
VHOOKLIBS=
LIBOBJFLAGS=
BUILD_STATIC=yes
BUILDSUF=
FULLNAME=$(NAME)$(BUILDSUF)
LIBPREF=lib
LIBSUF=.a
LIBNAME=$(LIBPREF)$(FULLNAME)$(LIBSUF)
SLIBPREF=lib
SLIBSUF=.so
EXESUF=
EXTRA_VERSION=
DEPEND_CMD=$(CC) $(CFLAGS) -MM $< | sed -e "/^\#.*/d" -e "s,^[[:space:]]*$(*F)\\.o,$(@D)/$(*F).o,"
HOSTCC=gcc
HOSTCFLAGS=-O3 -g -Wall
HOSTLDFLAGS=
HOSTLIBS=-lm
TARGET_EXEC=
TARGET_PATH=.
libswscale_VERSION=0.7.1
libswscale_VERSION_MAJOR=0
libpostproc_VERSION=51.2.0
libpostproc_VERSION_MAJOR=51
libavcodec_VERSION=52.20.0
libavcodec_VERSION_MAJOR=52
libavdevice_VERSION=52.1.0
libavdevice_VERSION_MAJOR=52
libavformat_VERSION=52.31.0
libavformat_VERSION_MAJOR=52
libavutil_VERSION=49.15.0
libavutil_VERSION_MAJOR=49
libavfilter_VERSION=0.4.0
libavfilter_VERSION_MAJOR=0
LIB_INSTALL_EXTRA_CMD=$(RANLIB) "$(LIBDIR)/$(LIBNAME)"
EXTRALIBS=  -lz -lm
ARCH=x86
ARCH_X86=yes
ARCH_X86_32=yes
HAVE_AMD3DNOW=yes
HAVE_AMD3DNOWEXT=yes
HAVE_MMX=yes
HAVE_MMX2=yes
HAVE_SSE=yes
HAVE_SSSE3=yes
HAVE_W32THREADS=yes
HAVE_ARPA_INET_H=yes
HAVE_BSWAP=yes
HAVE_DLFCN_H=yes
HAVE_DLOPEN=yes
HAVE_EBP_AVAILABLE=yes
HAVE_EBX_AVAILABLE=yes
HAVE_FAST_UNALIGNED=yes
HAVE_FORK=yes
HAVE_GETRUSAGE=yes
HAVE_INET_ATON=yes
HAVE_INLINE_ASM=yes
HAVE_LLRINT=yes
HAVE_LRINT=yes
HAVE_LRINTF=yes
HAVE_MALLOC_H=yes
HAVE_MEMALIGN=yes
HAVE_MKSTEMP=yes
HAVE_POSIX_MEMALIGN=yes
HAVE_ROUND=yes
HAVE_ROUNDF=yes
HAVE_SOCKLEN_T=yes
HAVE_POLL_H=yes
HAVE_SYS_MMAN_H=yes
HAVE_SYS_RESOURCE_H=yes
HAVE_SYS_SELECT_H=yes
HAVE_SYS_SOUNDCARD_H=yes
HAVE_TEN_OPERANDS=yes
HAVE_TERMIOS_H=yes
HAVE_THREADS=yes
HAVE_TRUNCF=yes
CONFIG_DECODERS=yes
CONFIG_DEMUXERS=yes
CONFIG_PROTOCOLS=yes
CONFIG_FFT=yes
CONFIG_GOLOMB=yes
CONFIG_IPV6=yes
CONFIG_MDCT=yes
CONFIG_MEMALIGN_HACK=yes
CONFIG_MPEGAUDIO_HP=yes
CONFIG_NETWORK=yes
CONFIG_STATIC=yes
CONFIG_ZLIB=yes
CONFIG_OLDSCALER=yes
CONFIG_FLV_DECODER=yes
CONFIG_GIF_DECODER=yes
CONFIG_H264_DECODER=yes
CONFIG_MJPEG_DECODER=yes
CONFIG_MPEG4_DECODER=yes
CONFIG_PNG_DECODER=yes
CONFIG_AAC_DECODER=yes
CONFIG_MP3_DECODER=yes
CONFIG_AAC_PARSER=yes
CONFIG_H264_PARSER=yes
CONFIG_MPEG4VIDEO_PARSER=yes
CONFIG_AAC_DEMUXER=yes
CONFIG_FLV_DEMUXER=yes
CONFIG_H264_DEMUXER=yes
CONFIG_IMAGE2_DEMUXER=yes
CONFIG_MJPEG_DEMUXER=yes
CONFIG_MOV_DEMUXER=yes
CONFIG_FILE_PROTOCOL=yes
CONFIG_HTTP_PROTOCOL=yes
CONFIG_TCP_PROTOCOL=yes
SRC_PATH="/home/vin777/work/vincent/development/vineo/stuff/ffmpeg"
SRC_PATH_BARE=/home/vin777/work/vincent/development/vineo/stuff/ffmpeg
BUILD_ROOT="/home/vin777/work/vincent/development/vineo/stuff/ffmpeg/build"
