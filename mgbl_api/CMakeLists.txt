cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)

project(mgbl_api
        VERSION ${CMAKE_PROJECT_VERSION}
        LANGUAGES C CXX
        )

find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET protobuf grpc++)

include(GNUInstallDirs)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(third_party/gnmi)
configure_file(cmake/mgbl_api.pc.in ${PROJECT_BINARY_DIR}/mgbl_api.pc @ONLY)
add_library(mgbl_api STATIC)

target_include_directories(mgbl_api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/>
)

target_sources(mgbl_api
    PRIVATE
        src/mgbl_api.cpp
        src/logger/logger.cpp
        src/gnmi/mgbl_gnmi_helper.cpp
        src/pbr/mgbl_pbr.cpp
)

target_link_libraries(mgbl_api PRIVATE
    PkgConfig::gRPC
    pthread
    nlohmann_json::nlohmann_json
    fmt::fmt
    rpc_a
)

set(MGBL_API_HEADERS include/mgbl_api.h
    include/pbr/mgbl_pbr.h
    include/rpc/mgbl_rpc.h
    include/gnmi/mgbl_gnmi_client.h
    include/gnmi/mgbl_gnmi_connection.h
    src/gnmi/mgbl_gnmi_helper.h
    src/logger/logger.h
    src/mgbl_api_impl.h
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/gnmi/generated/gnmi.grpc.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/gnmi/generated/gnmi.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/third_party/gnmi/generated/gnmi_ext.pb.h
)

target_sources(mgbl_api PUBLIC FILE_SET HEADERS
    BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/third_party/gnmi/generated/
    FILES
        ${MGBL_API_HEADERS}
)

set_target_properties(mgbl_api PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(MGBL_API_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/mgbl_api)
set(MGBL_API_PKGCONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
set(version_config ${PROJECT_BINARY_DIR}/mgbl_api-config-version.cmake)
set(project_config ${PROJECT_BINARY_DIR}/mgbl_api-config.cmake)
set(pkgconfig ${PROJECT_BINARY_DIR}/cmake/mgbl_api.pc)
set(targets_export_name mgbl_api-targets)

write_basic_package_version_file(
    ${version_config}
    VERSION ${FMT_VERSION}
    COMPATIBILITY AnyNewerVersion)

configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/mgbl_api.pc.in"
    "${pkgconfig}"
    @ONLY)
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/mgbl_api-config.cmake.in
    ${project_config}
    INSTALL_DESTINATION ${MGBL_API_CMAKE_DIR})

set(INSTALL_TARGETS mgbl_api rpc_a)


set(INSTALL_FILE_SET FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mgbl_api")

# Install the library and headers.
install(TARGETS ${INSTALL_TARGETS} EXPORT ${targets_export_name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mgbl_api"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ${INSTALL_FILE_SET})

# Use a namespace because CMake provides better diagnostics for namespaced
# imported targets.
export(TARGETS ${INSTALL_TARGETS} NAMESPACE mgbl_api::
        FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

# Install version, config and target files.
install(
    FILES ${project_config} ${version_config}
    DESTINATION ${MGBL_API_CMAKE_DIR})
install(EXPORT ${targets_export_name} DESTINATION ${MGBL_API_CMAKE_DIR}
        NAMESPACE mgbl_api::)

install(FILES "${pkgconfig}" DESTINATION "${MGBL_API_PKGCONFIG_DIR}")
