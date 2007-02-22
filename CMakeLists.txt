project(libkcompactdisc)

########### next target ###############

set(audio_SRCS
        audio/audio.c
        audio/audio_arts.c
        audio/audio_alsa.c
        audio/audio_sun.c
        audio/audio_phonon.cpp
)

set(libwm_SRCS ${audio_SRCS}
	cddb.c
	cdrom.c
	wm_helpers.c
	cdtext.c
	scsi.c
	cdda.c
	plat_linux_cdda.c
	plat_sun_cdda.c
	plat_aix.c
	plat_bsd386.c
	plat_freebsd.c
	plat_hpux.c
	plat_irix.c
	plat_linux.c
	plat_svr4.c
	plat_ultrix.c
	plat_news.c
	plat_openbsd.c
	plat_osf1.c
	plat_sun.c
	plat_scor5.c
	drv_sony.c
	drv_toshiba.c
)

set(kcompactdisc_LIB_SRCS ${libwm_SRCS} kcompactdisc.cpp)

kde4_automoc(${kcompactdisc_LIB_SRCS} audio/audio_phonon.cpp)

kde4_add_library(kcompactdisc SHARED ${kcompactdisc_LIB_SRCS})

target_link_libraries(kcompactdisc ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KIO_LIBS} phononcore)
if (HAVE_LIBASOUND2)
    target_link_libraries(kcompactdisc ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

macro_display_feature_log()

set_target_properties(kcompactdisc PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kcompactdisc  DESTINATION ${LIB_INSTALL_DIR} )

########### install files ###############

install( FILES kcompactdisc.h  DESTINATION ${INCLUDE_INSTALL_DIR}/libkcompactdisc)

