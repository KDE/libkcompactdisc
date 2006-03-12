kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = $(all_includes) $(ARTSC_INCLUDE)
#
#noinst_LTLIBRARIES = libworkmanaudio.la
#
#libworkmanaudio_la_SOURCES = audio.c audio_alsa.c audio_arts.c audio_sun.c
