include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDES})

set(KCD_LIB_SRCS
    disc.cpp
    track.cpp
    audiofile.cpp
    kcompactdisc.cpp
)

add_library(KF5CompactDisc SHARED ${KCD_LIB_SRCS})
add_library(KF5::CompactDisc ALIAS KF5CompactDisc)
set_target_properties(KF5CompactDisc PROPERTIES
    VERSION ${KCOMPACTDISC_VERSION}
    SOVERSION ${KCOMPACTDISC_SOVERSION}
)

target_link_libraries(KF5CompactDisc
    KF5::CoreAddons
    KF5::Solid
    KF5::I18n
    ${CDIO_LIBRARIES}
)

target_include_directories(KF5CompactDisc PUBLIC $<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc>)
ecm_generate_headers(KCOMPACTDISC_HEADERS
    HEADER_NAMES Disc Track AudioFile KCompactDisc
    PREFIX KCompactDisc
    REQUIRED_HEADERS KCOMPACTDISC_REQ_HEADERS
)
generate_export_header(KF5CompactDisc BASE_NAME kcompactdisc)

ecm_generate_pri_file(
    BASE_NAME KCompactDisc
    LIB_NAME KF5CompactDisc
    DEPS "core KCoreAddons KI18n Solid"
    FILENAME_VAR PRI_FILENAME
)


install(TARGETS KF5CompactDisc EXPORT KF5CompactDiscTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${KCOMPACTDISC_HEADERS} DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc/KCompactDisc COMPONENT Devel)
install(FILES ${KCOMPACTDISC_REQ_HEADERS} DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc/kcompactdisc COMPONENT Devel)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_export.h DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCompactDisc/kcompactdisc COMPONENT Devel)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
