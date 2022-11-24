cmake_minimum_required(VERSION 3.16)

project(KCompactDisc VERSION 5.0.0)

set(QT_MIN_VERSION "5.15.2")
set(KF_MIN_VERSION "5.92.0")

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
include(ECMDeprecationSettings)

# Qt5, KF5 and Phonon Packages

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Solid
    I18n
)
find_package(Phonon4Qt${QT_MAJOR_VERSION} 4.8.0 CONFIG REQUIRED)

# Sources
ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.100.0
    )

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
