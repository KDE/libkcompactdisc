#
# Makefile.am for libworkman. Based on the example Makefile.am for a
# shared library.
#

subdirs = include
SUBDIRS = audio
INCLUDES = $(all_includes) 

lib_LTLIBRARIES  = libworkman.la

libworkman_la_LDFLAGS = -version-info 6:0:0 $(all_libraries)

libworkman_la_SOURCES = cddb.c cdinfo.c cdrom.c wm_helpers.c cdtext.c\
database.c index.c scsi.c cdda.c\
plat_aix.c plat_bsd386.c plat_freebsd.c plat_hpux.c plat_irix.c \
plat_linux.c plat_svr4.c plat_ultrix.c plat_news.c plat_openbsd.c \
plat_osf1.c plat_sun.c plat_scor5.c \
drv_sony.c drv_toshiba.c

libworkmanincludedir = $(includedir)/libwm
libworkmaninclude_HEADERS = include/wm_cdrom.h include/wm_cdtext.h

if libwm_with_cdda
CDDA_SLAVE = cddaslave
endif

bin_PROGRAMS = $(CDDA_SLAVE)
cddaslave_SOURCES = cddaslave.c plat_linux_cdda.c plat_sun_cdda.c

cddaslave_LDADD = audio/libworkmanaudio.a libworkman.la $(ARTSC_LIBS) $(ALSA_LIBS)
