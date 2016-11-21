cmake_minimum_required(VERSION 2.8.12)

project(libkcompactdisc)

set(REQUIRED_QT_VERSION 5.4.0)
set(PROJECT_VERSION "16.08.0")
set(KF5_VERSION ${PROJECT_VERSION})
set(KF5_DEP_VERSION "5.24.0") # handled by release scripts

# Dependencies
include(FeatureSummary)
find_package(ECM 5.24.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMGenerateHeaders)
include(ECMSetupVersion)
include(ECMGeneratePriFile)
include(GenerateExportHeader)
include(ECMPackageConfigHelpers)

# Version Setup

ecm_setup_version(
    ${KF5_VERSION}
    VARIABLE_PREFIX KCOMPACTDISC
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    PACKAGE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfigVersion.cmake
    SOVERSION 16
)

# Qt5, KF5 and Phonon Packages

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS CoreAddons Solid I18n)
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

ecm_configure_package_config_file(
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

