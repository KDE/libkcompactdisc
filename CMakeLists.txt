cmake_minimum_required(VERSION 3.0.2)
project(libkcompactdisc VERSION 5.0.0)

set(QT5_REQUIRED_VERSION 5.4.0)
set(KF5_REQUIRED_VERSION 5.13.0)

find_package(ECM ${KF5_REQUIRED_VERSION} NO_MODULE REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(CMakePackageConfigHelpers)
include(ECMGenerateHeaders)
include(ECMSetupVersion)
include(ECMGeneratePriFile)
include(GenerateExportHeader)
include(FeatureSummary)

# Version Setup

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KCOMPACTDISC
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    PACKAGE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfigVersion.cmake
    SOVERSION 5
)

# Qt5, KF5 and Phonon Packages

find_package(Qt5 ${QT5_REQUIRED_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF5 ${KF5_REQUIRED_VERSION} REQUIRED COMPONENTS CoreAddons KDELibs4Support Solid I18n)
find_package(Phonon4Qt5 4.8.0 CONFIG REQUIRED)

# Sources

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(src)
add_subdirectory(tests)

# Additional

set(CMAKECONFIG_INSTALL_DIR ${CMAKECONFIG_INSTALL_PREFIX}/KF5CompactDisc)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/KF5CompactDiscConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfigVersion.cmake
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT
    KF5CompactDiscTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF5CompactDiscTargets.cmake
    NAMESPACE KF5::
    COMPONENT Devel
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    DESTINATION "${KF5_INCLUDE_INSTALL_DIR}"
    COMPONENT Devel
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
