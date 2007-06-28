project(libkcompactdisc)

add_subdirectory(wmlib)

set(kcompactdisc_LIB_SRCS kcompactdisc.cpp kcompactdisc_p.cpp phonon_interface.cpp wmlib_interface.cpp)

kde4_automoc(${kcompactdisc_LIB_SRCS})

kde4_add_library(kcompactdisc SHARED ${kcompactdisc_LIB_SRCS})

target_link_libraries(kcompactdisc wmlib ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS})
if (HAVE_LIBASOUND2)
    target_link_libraries(kcompactdisc ${ASOUND_LIBRARY})
endif (HAVE_LIBASOUND2)

macro_display_feature_log()

set_target_properties(kcompactdisc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kcompactdisc  DESTINATION ${LIB_INSTALL_DIR} )

########### install files ###############

install( FILES kcompactdisc.h  DESTINATION ${INCLUDE_INSTALL_DIR}/libkcompactdisc)

