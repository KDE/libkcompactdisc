include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDES})

## Compiler flags

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_C_FLAGS "-std=c99")   ## ALSA no longer compiles with -std=c90, see https://bugzilla.novell.com/show_bug.cgi?id=817077
endif()

find_package(ALSA)

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

# Disable WMLib temporarily

set(USE_WMLIB false)

set(KCD_LIB_SRCS
    kcompactdisc.cpp
    kcompactdisc_p.cpp
    phonon_interface.cpp
)

if (USE_WMLIB)
        set(KCD_LIB_SRCS ${KCD_LIB_SRCS} wmlib_interface.cpp ${wmlib_SRCS} ${wmlib_audio_SRCS})
        add_definitions(-DUSE_WMLIB=1)
endif (USE_WMLIB)

add_library(KF5CompactDisc SHARED ${KCD_LIB_SRCS})
add_library(KF5::CompactDisc ALIAS KF5CompactDisc)
set_target_properties(KF5CompactDisc PROPERTIES
    VERSION ${KCOMPACTDISC_VERSION}
    SOVERSION ${KCOMPACTDISC_SOVERSION}
)

target_link_libraries(KF5CompactDisc
    Qt5::DBus
    KF5::CoreAddons
    KF5::Solid
    KF5::I18n
    Phonon::phonon4qt5
)

if (HAVE_LIBASOUND2)
    target_link_libraries(KF5CompactDisc ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

if (USE_WMLIB)
    find_package(Threads)
    target_link_libraries(KF5CompactDisc ${CMAKE_THREAD_LIBS_INIT})
endif (USE_WMLIB)

target_include_directories(KF5CompactDisc PUBLIC $<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc>)
ecm_generate_headers(KCOMPACTDISC_HEADERS
    HEADER_NAMES KCompactDisc
    PREFIX KCompactDisc
    REQUIRED_HEADERS KCOMPACTDISC_REQ_HEADERS
)

ecm_generate_pri_file(
    BASE_NAME KCompactDisc
    LIB_NAME KF5CompactDisc
    DEPS "core dbus phonon4qt5 KCoreAddons KI18n Solid"
    FILENAME_VAR PRI_FILENAME
)

install(TARGETS KF5CompactDisc EXPORT KF5CompactDiscTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${KCOMPACTDISC_HEADERS} DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc/KCompactDisc COMPONENT Devel)
install(FILES ${KCOMPACTDISC_REQ_HEADERS} DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc/kcompactdisc COMPONENT Devel)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
