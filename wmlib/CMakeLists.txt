set(wmlib_audio_SRCS
	audio/audio.c
	audio/audio_arts.c
	audio/audio_alsa.c
	audio/audio_sun.c
)

set(wmlib_SRCS
	cdda.c
	cddb.c
	cdrom.c
	wm_helpers.c
	cdtext.c
	scsi.c
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

kde4_add_library(wmlib STATIC ${wmlib_SRCS} ${wmlib_audio_SRCS})

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
 SET_TARGET_PROPERTIES(wmlib PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
