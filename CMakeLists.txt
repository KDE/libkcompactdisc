cmake_minimum_required(VERSION 3.16)

project(KCompactDisc VERSION 5.0.0)

set(QT_MIN_VERSION "5.15.0")
set(KF_MIN_VERSION "5.80.0")

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
    ${KCompactDisc_VERSION}
    VARIABLE_PREFIX KCOMPACTDISC
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kcompactdisc_version.h
    PACKAGE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/KF5CompactDiscConfigVersion.cmake
    SOVERSION 5
)

# Qt5, KF5 and Phonon Packages

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Solid
    I18n
)
find_package(Phonon4Qt5 4.8.0 CONFIG REQUIRED)

# Sources

add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050F00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055000
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
    -DQT_NO_FOREACH
    -DQT_NO_KEYWORDS
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Additional

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5CompactDisc)

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
    DESTINATION "${KDE_INSTALL_INCLUDEDIR_KF5}"
    COMPONENT Devel
)

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
