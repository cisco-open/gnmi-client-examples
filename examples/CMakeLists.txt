cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)

project(examples LANGUAGES C CXX)

find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET protobuf grpc++)
find_package(mgbl_api REQUIRED)

add_executable(mgbl_api_pbr_once_tutorial mgbl_api_pbr_once_tutorial.cpp)
target_link_libraries(mgbl_api_pbr_once_tutorial PRIVATE
    PkgConfig::gRPC
    nlohmann_json::nlohmann_json
    fmt::fmt
    mgbl_api::mgbl_api
)

install(TARGETS mgbl_api_pbr_once_tutorial
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_executable(mgbl_api_pbr_stream_tutorial mgbl_api_pbr_stream_tutorial.cpp)
target_link_libraries(mgbl_api_pbr_stream_tutorial PRIVATE
    PkgConfig::gRPC
    nlohmann_json::nlohmann_json
    fmt::fmt
    mgbl_api::mgbl_api
)

install(TARGETS mgbl_api_pbr_stream_tutorial
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_custom_target(clean-examples
    COMMAND ${CMAKE_COMMAND} -E remove mgbl_api_pbr_once_tutorial
    COMMAND ${CMAKE_COMMAND} -E remove mgbl_api_pbr_stream_tutorial
    COMMENT "Cleaning example binaries"
)