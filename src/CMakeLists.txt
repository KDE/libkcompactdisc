## Compiler flags

add_definitions(-DTRANSLATION_DOMAIN=\"libkcompactdisc\")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")   ## ALSA no longer compiles with -std=c90, see https://bugzilla.novell.com/show_bug.cgi?id=817077
endif()

find_package(ALSA)
set_package_properties(ALSA PROPERTIES
                       URL "https://www.alsa-project.org/"
                       DESCRIPTION "ALSA provides audio and MIDI functionality"
                       TYPE OPTIONAL
                       PURPOSE "Play back audio CDs via ALSA")
set(HAVE_ALSA ${ALSA_FOUND})

set(KCOMPACTDISC_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KCompactDisc6")
set(KCOMPACTDISC_CMAKECONFIG_NAME "KCompactDisc6")
set(LIBRARYFILE_NAME "KCompactDisc6")
set(TARGET_EXPORT_NAME "KCompactDisc6")
set(NAMESPACE_NAME "")

# Version Setup

ecm_setup_version(${KCompactDisc_VERSION}
    VARIABLE_PREFIX KCOMPACTDISC
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    PACKAGE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KCOMPACTDISC_CMAKECONFIG_NAME}ConfigVersion.cmake
    SOVERSION 5
)

configure_file(config-alsa.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

add_library(KCompactDisc SHARED)
set_target_properties(KCompactDisc PROPERTIES
    VERSION ${KCOMPACTDISC_VERSION}
    SOVERSION ${KCOMPACTDISC_SOVERSION}
    OUTPUT_NAME ${LIBRARYFILE_NAME}
    EXPORT_NAME ${TARGET_EXPORT_NAME}
)

set(_generate_export_header_version_args USE_VERSION_HEADER)

ecm_generate_export_header(KCompactDisc
    BASE_NAME KCompactDisc
    VERSION ${KCOMPACTDISC_VERSION}
    DEPRECATED_BASE_VERSION 0
    ${_generate_export_header_version_args}
)

target_sources(KCompactDisc PRIVATE
    kcompactdisc.cpp kcompactdisc.h
    kcompactdisc_p.cpp kcompactdisc_p.h
    phonon_interface.cpp phonon_interface.h
)

target_link_libraries(KCompactDisc
    PUBLIC
        Qt::Core
    PRIVATE
        Qt::DBus
        KF6::Solid
        KF6::I18n
        Phonon::phonon4qt6
)

if (HAVE_ALSA)
    target_link_libraries(KCompactDisc PRIVATE ALSA::ALSA)
endif()

target_include_directories(KCompactDisc
    INTERFACE
        "$<INSTALL_INTERFACE:${KCOMPACTDISC_INSTALL_INCLUDEDIR}>"
)

ecm_generate_headers(KCompactDisc_HEADERS
    HEADER_NAMES 
    KCompactDisc
    REQUIRED_HEADERS KCompactDisc_HEADERS
)

install(TARGETS KCompactDisc EXPORT KCompactDiscTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_export.h
    ${KCompactDisc_HEADERS}
    DESTINATION ${KCOMPACTDISC_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})


set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/${KCOMPACTDISC_CMAKECONFIG_NAME})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/KF5CompactDiscConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${KCOMPACTDISC_CMAKECONFIG_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${KCOMPACTDISC_CMAKECONFIG_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${KCOMPACTDISC_CMAKECONFIG_NAME}ConfigVersion.cmake
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT
    KCompactDiscTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE ${KCOMPACTDISC_CMAKECONFIG_NAME}Targets.cmake
    NAMESPACE ${NAMESPACE_NAME}
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    DESTINATION "${KCOMPACTDISC_INSTALL_INCLUDEDIR}"
    COMPONENT Devel
)

ecm_generate_pri_file(
    BASE_NAME KCompactDisc
    LIB_NAME ${KCOMPACTDISC_CMAKECONFIG_NAME}
    DEPS "core dbus phonon4qt5 KCoreAddons KI18n Solid"
    FILENAME_VAR PRI_FILENAME
    INCLUDE_INSTALL_DIR ${KCOMPACTDISC_INSTALL_INCLUDEDIR}
)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
