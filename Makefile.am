#
# Makefile.am for libworkman. Based on the example Makefile.am for a
# shared library.
#

INCLUDES=  $(all_includes) 

lib_LTLIBRARIES  = libworkman.la

libworkman_la_LDFLAGS = -version-info 5:2:4 $(all_libraries)

#
# Headers come later...
#
include_HEADERS = 

# Which headers shouldn't be installed when a   make install  is done?
#noinst_HEADERS =

libworkman_la_SOURCES = cddb.c cdinfo.c cdrom.c wm_helpers.c \
database.c drv_sony.c drv_toshiba.c index.c plat_aix.c plat_bsd386.c \
plat_freebsd.c plat_hpux.c plat_irix.c plat_linux.c plat_linux_audio.c \
plat_linux_cdda.c plat_news.c plat_openbsd.c plat_osf1.c plat_sun.c \
plat_sun_audio.c plat_sun_cdda.c plat_svr4.c plat_ultrix.c scsi.c 


