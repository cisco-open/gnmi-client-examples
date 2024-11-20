cmake_minimum_required(VERSION 3.23)
project(mgbl_api
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "A gNMI library to retrieve counter statistics data with open-config models."
)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})


option(ENABLE_UNIT_TESTS "Build and run unit test for this project" OFF)
option(ENABLE_FUNC_TESTS "Build and run functional test for this project" OFF)
option(ENABLE_SPHINX_DOC "Build the Sphinx documentation for this project" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(CTEST_COMMAND ctest)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CTEST_COMMAND ctest --output-on-failure)
endif()

# Superbuild variables for sub projects
include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE ${CMAKE_BINARY_DIR}/subprojects)
set(STAGED_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/stage)
set(DEPENDENCIES_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/subprojects/Install)
include(GNUInstallDirs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# ------------------------
# Install dependencies
# ------------------------

add_subdirectory(third_party)

# ------------------------
# Build Library
# ------------------------

unset(CMAKE_CACHE_ARGS_EXTRA)
if (CMAKE_PROJECT_INCLUDE AND NOT CMAKE_PROJECT_INCLUDE STREQUAL "")
    set(CMAKE_CACHE_ARGS_EXTRA "-DCMAKE_PROJECT_INCLUDE:PATH=${CMAKE_PROJECT_INCLUDE}")
endif()

ExternalProject_Add(mgbl_api
    SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/mgbl_api"
    DEPENDS
        nlohmann_json_dependency
        fmt_dependency
        grpc_dependency
        googletest_dependency
    CMAKE_ARGS
        -DPKG_CONFIG_EXECUTABLE:PATH=${PKG_CONFIG_EXECUTABLE}
    CMAKE_CACHE_ARGS
        -DCMAKE_PROJECT_VERSION:STRING=${CMAKE_PROJECT_VERSION}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_FIND_ROOT_PATH:PATH=${CMAKE_FIND_ROOT_PATH}
        -DCMAKE_IGNORE_PATH:PATH=${CMAKE_IGNORE_PATH}
        -DCMAKE_SYSROOT:PATH=${CMAKE_SYSROOT}
        -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
        -DCMAKE_C_COMPILER_TARGET:STRING=${CMAKE_C_COMPILER_TARGET}
        -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_COMPILER_TARGET:STRING=${CMAKE_CXX_COMPILER_TARGET}
        -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
        -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
        -DCMAKE_INSTALL_PREFIX:PATH=${STAGED_INSTALL_PREFIX}
        -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_INSTALL_INCLUDEDIR:PATH=${CMAKE_INSTALL_INCLUDEDIR}
        -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
        -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
        -DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
        -DROOT_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        ${CMAKE_CACHE_ARGS_EXTRA}
    BUILD_ALWAYS
        1
    )

# ------------------------
# Add the install directory to the CMake search path
# ------------------------

include(GNUInstallDirs)
set(CMAKE_PREFIX_PATH ${STAGED_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}
        CACHE PATH "append mgbl_api library into the search path"
        FORCE)

# ------------------------
# Apply clang-format target
# ------------------------

#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClangFormat.cmake)
#add_clamp_format_target(format-code)

# ------------------------
# Build Unit tests
# ------------------------

if (ENABLE_UNIT_TESTS)
    ExternalProject_Add(unit_tests
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/test/unit"
        DEPENDS
            mgbl_api
        CMAKE_CACHE_ARGS
            -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_BINDIR:PATH=${CMAKE_INSTALL_BINDIR}
            -DSTAGED_INSTALL_PREFIX:PATH=${STAGED_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
        TEST_BEFORE_INSTALL
            1
        INSTALL_COMMAND
            ""
        TEST_COMMAND
            ${CTEST_COMMAND}
    )
endif ()

# ------------------------------
# Build Integration tests
# ------------------------------

# Run the functional tests
if (ENABLE_FUNC_TESTS)
    ExternalProject_Add(integration_tests
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/test/integration"
        DEPENDS
            mgbl_api
        CMAKE_CACHE_ARGS
            -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_BINDIR:PATH=${CMAKE_INSTALL_BINDIR}
            -DSTAGED_INSTALL_PREFIX:PATH=${STAGED_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
        TEST_BEFORE_INSTALL
            1
        INSTALL_COMMAND
            ""
        TEST_COMMAND
            ${CTEST_COMMAND}
    )
endif ()

#add_subdirectory(test/integration)

# ------------------------------
# Build Examples
# ------------------------------

ExternalProject_Add(examples
    SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/examples"
    DEPENDS
        mgbl_api
    CMAKE_CACHE_ARGS
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
)

# ------------------------------
# Build Documentation
# ------------------------------

if(ENABLE_SPHINX_DOC)
    ExternalProject_Add(documentation
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/docs"
        DEPENDS
            mgbl_api
        CMAKE_CACHE_ARGS
            -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_BINDIR:PATH=${CMAKE_INSTALL_BINDIR}
            -DSTAGED_INSTALL_PREFIX:PATH=${STAGED_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
            -DCMAKE_INSTALL_DOCDIR:PATH=${CMAKE_INSTALL_PREFIX}/docs
    )
endif()

# ------------------------------
# Build Testing tool
# ------------------------------

#add_subdirectory(test/testing_tool)
ExternalProject_Add(testing_tool
    SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/test/testing_tool"
    DEPENDS
        mgbl_api
    CMAKE_CACHE_ARGS
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
)

# ------------------------------
# Install
# ------------------------------

# Install the project artifacts from the staged directory
include(GNUInstallDirs)
install(DIRECTORY
            ${STAGED_INSTALL_PREFIX}/
        DESTINATION
            .
        USE_SOURCE_PERMISSIONS
)
install(FILES
            LICENSE README.md CONTRIBUTING.md CODE_OF_CONDUCT.md
        DESTINATION
            ${CMAKE_INSTALL_DOCDIR}
)

# ------------------------------
# Packaging
# ------------------------------

# Set up package from this project
set(CPACK_PACKAGE_NAME "mgbl_api")
set(CPACK_PACKAGE_CONTACT "Cisco Systems, Inc.")
set(CPACK_PACKAGE_VENDOR ${CPACK_PACKAGE_CONTACT})
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A gNMI library to retrieve counter statistics data with open-config models.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_RPM_PACKAGE_RELEASE "1%{?dist}")
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP "")
set(CPACK_RPM_PACKAGE_URL "http://github.com/cisco-open/gnmi-client-examples")
set(CPACK_RPM_PACKAGE_DESCRIPTION "A gNMI library to retrieve counter statistics data with open-config models.")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)