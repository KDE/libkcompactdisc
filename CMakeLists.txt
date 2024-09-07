cmake_minimum_required(VERSION 3.16)

project(KCompactDisc VERSION 5.0.0)

set(QT_MIN_VERSION "6.7")
set(KF_MIN_VERSION "6.4")

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
include(ECMGenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMDeprecationSettings)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})


# Qt6, KF6 and Phonon Packages

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Solid
    I18n
)
find_package(Phonon4Qt6 4.8.0 CONFIG REQUIRED)

# Sources
ecm_set_disabled_deprecation_versions(
    QT 6.7
    KF 6.4
)

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ki18n_install(po)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
