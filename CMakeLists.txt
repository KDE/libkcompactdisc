project(libkcompactdisc)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

find_package(Alsa)
alsa_configure_file(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

include_directories(${KDE4_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})


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

if (APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL GNU)
	set(USE_WMLIB false)
else (APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL GNU)
	set(USE_WMLIB true)
endif (APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL GNU)

set(kcompactdisc_LIB_SRCS kcompactdisc.cpp kcompactdisc_p.cpp phonon_interface.cpp)

if (USE_WMLIB)
	set(kcompactdisc_LIB_SRCS ${kcompactdisc_LIB_SRCS} wmlib_interface.cpp ${wmlib_SRCS} ${wmlib_audio_SRCS})
	add_definitions(-DUSE_WMLIB=1)
endif (USE_WMLIB)

kde4_add_library(kcompactdisc SHARED ${kcompactdisc_LIB_SRCS})

target_link_libraries(kcompactdisc ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS})
if (HAVE_LIBASOUND2)
    target_link_libraries(kcompactdisc ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

set_target_properties(kcompactdisc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kcompactdisc EXPORT LibkcompactdiscTargets ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kcompactdisc.h  DESTINATION ${INCLUDE_INSTALL_DIR}/libkcompactdisc COMPONENT Devel)

# Figure out the relative path from the installed Config.cmake file to the install prefix (which may be at
# runtime different from the chosen CMAKE_INSTALL_PREFIX if under Windows the package was installed anywhere)
# This relative path will be configured into LibkcompactdiscConfig.cmake
file(RELATIVE_PATH relInstallDir ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/cmake/libkcompactdisc ${CMAKE_INSTALL_PREFIX})

configure_file(LibkcompactdiscConfig.cmake.in ${CMAKE_BINARY_DIR}/LibkcompactdiscConfig.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/LibkcompactdiscConfig.cmake DESTINATION ${LIB_INSTALL_DIR}/cmake/libkcompactdisc)
install(EXPORT LibkcompactdiscTargets DESTINATION ${LIB_INSTALL_DIR}/cmake/libkcompactdisc/ FILE LibkcompactdiscTargets.cmake NAMESPACE KDE::Libkcompactdisc::)
