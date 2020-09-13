cmake_minimum_required(VERSION 3.5)

project(libkcompactdisc)

set(PROJECT_VERSION "16.08.0")

set(QT_MIN_VERSION "5.12.0")
set(KF_MIN_VERSION "5.68.0")

# Dependencies
include(FeatureSummary)
find_package(ECM ${KF_MIN_VERSION} NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMGenerateHeaders)
include(ECMSetupVersion)
include(ECMGeneratePriFile)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

# Version Setup

ecm_setup_version(
    "5.0.0"
    VARIABLE_PREFIX KCOMPACTDISC
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    PACKAGE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfigVersion.cmake
    SOVERSION 5
)

# Qt5, KF5 and Phonon Packages

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Solid I18n)
find_package(Phonon4Qt5 4.8.0 CONFIG REQUIRED)

# Sources

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)


add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

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

