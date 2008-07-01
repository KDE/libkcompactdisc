project(libkcompactdisc)


set(wmlib_audio_SRCS
        wmlib/audio/audio.c
        wmlib/audio/audio_arts.c
        wmlib/audio/audio_alsa.c
        wmlib/audio/audio_sun.c
)

set(wmlib_SRCS
        wmlib/cdda.c
        wmlib/cddb.c
        wmlib/cdrom.c
        wmlib/wm_helpers.c
        wmlib/cdtext.c
        wmlib/scsi.c
        wmlib/plat_aix.c
        wmlib/plat_bsd386.c
        wmlib/plat_freebsd.c
        wmlib/plat_hpux.c
        wmlib/plat_irix.c
        wmlib/plat_linux.c
        wmlib/plat_svr4.c
        wmlib/plat_ultrix.c
        wmlib/plat_news.c
        wmlib/plat_openbsd.c
        wmlib/plat_osf1.c
        wmlib/plat_sun.c
        wmlib/plat_scor5.c
        wmlib/drv_sony.c
        wmlib/drv_toshiba.c
)

if (APPLE OR WIN32)
	set(USE_WMLIB false)
else (APPLE OR WIN32)
	set(USE_WMLIB true)
endif (APPLE OR WIN32)

set(kcompactdisc_LIB_SRCS kcompactdisc.cpp kcompactdisc_p.cpp phonon_interface.cpp)

if (USE_WMLIB)
	set(kcompactdisc_LIB_SRCS ${kcompactdisc_LIB_SRCS} wmlib_interface.cpp ${wmlib_SRCS} ${wmlib_audio_SRCS})
	add_definitions(-DUSE_WMLIB=1)
endif (USE_WMLIB)

kde4_add_library(kcompactdisc SHARED ${kcompactdisc_LIB_SRCS})

target_link_libraries(kcompactdisc ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS})
if (HAVE_LIBASOUND2)
    target_link_libraries(kcompactdisc ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

set_target_properties(kcompactdisc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kcompactdisc  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kcompactdisc.h  DESTINATION ${INCLUDE_INSTALL_DIR}/libkcompactdisc COMPONENT Devel)

