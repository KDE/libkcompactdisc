include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDES})

## Compiler flags

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_C_FLAGS "-std=c99")   ## ALSA no longer compiles with -std=c90, see https://bugzilla.novell.com/show_bug.cgi?id=817077
endif()

find_package(ALSA)
set_package_properties(ALSA PROPERTIES
                       URL "http://www.alsa-project.org/"
                       DESCRIPTION "ALSA provides audio and MIDI functionality"
                       TYPE OPTIONAL
                       PURPOSE "Play back audo CDs via ALSA")
set(HAVE_ALSA ${ALSA_FOUND})
configure_file(config-alsa.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

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
    EXPORT_NAME CompactDisc
)

target_link_libraries(KF5CompactDisc
    Qt5::DBus
    KF5::CoreAddons
    KF5::Solid
    KF5::I18n
    Phonon::phonon4qt5
)

if (HAVE_ALSA)
    target_link_libraries(KF5CompactDisc ${ALSA_LIBRARY})
endif ()

if (USE_WMLIB)
    find_package(Threads)
    target_link_libraries(KF5CompactDisc ${CMAKE_THREAD_LIBS_INIT})
endif (USE_WMLIB)

target_include_directories(KF5CompactDisc INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KCompactDisc>")
ecm_generate_headers(KCompactDisc_HEADERS
    HEADER_NAMES 
    KCompactDisc
    REQUIRED_HEADERS KCompactDisc_HEADERS
)
generate_export_header(KF5CompactDisc BASE_NAME KCompactDisc)

ecm_generate_pri_file(
    BASE_NAME KCompactDisc
    LIB_NAME KF5CompactDisc
    DEPS "core dbus phonon4qt5 KCoreAddons KI18n Solid"
    FILENAME_VAR PRI_FILENAME
    INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KCompactDisc
)


install(TARGETS KF5CompactDisc EXPORT KF5CompactDiscTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_export.h 
    ${KCompactDisc_HEADERS} 
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KCompactDisc COMPONENT Devel)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
