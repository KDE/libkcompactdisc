#
# Makefile.am for libworkman. Based on the example Makefile.am for a
# shared library.
#

SUBDIRS =

INCLUDES = $(all_includes) @ARTSC_INCLUDE@

lib_LIBRARIES = libworkmanaudio.a

libworkmanaudio_a_SOURCES = audio.c audio_aix.c audio_alib.c audio_alsa.c\
audio_arts.c audio_esd.c audio_hp.c audio_mint.c audio_nas.c audio_os2.c \
audio_oss.c audio_sgi.c audio_sun.c audio_win32.c
