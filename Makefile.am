# Example Makefile.am for a shared library.  It makes a library
#  called "example" as libexample.so.2.1.2
# This Makefile.am was taken from the kdelibs distribution and modified 
#  to serve as an example.
#
# David Sweet
#

INCLUDES=  $(all_includes) 

lib_LTLIBRARIES  = libWorkMan.la

# Note:  If you specify a:b:c as the version in the next line,
#  the library that is made has version (a-c).c.b.  In this
#  example, the version is 2.1.2.
libWorkMan_la_LDFLAGS = -version-info 5:2:4 $(all_libraries)

include_HEADERS = 

# Which headers shouldn't be installed when a   make install  is done?
#noinst_HEADERS =

libWorkMan_la_SOURCES = cddb.c cdinfo.c cdrom.c wm_helpers.c \
database.c drv_sony.c drv_toshiba.c index.c plat_aix.c plat_bsd386.c \
plat_freebsd.c plat_hpux.c plat_irix.c plat_linux.c plat_linux_audio.c \
plat_linux_cdda.c plat_news.c plat_openbsd.c plat_osf1.c plat_sun.c \
plat_sun_audio.c plat_sun_cdda.c plat_svr4.c plat_ultrix.c scsi.c 


