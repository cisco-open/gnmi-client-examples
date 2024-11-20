cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)

project(testing_tool LANGUAGES C CXX)

find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET protobuf grpc++)
find_package(mgbl_api REQUIRED)

add_executable(mgbl_api_pbr_testing_tool mgbl_api_pbr_testing_tool.cpp)

target_link_libraries(mgbl_api_pbr_testing_tool PRIVATE
    PkgConfig::gRPC
    nlohmann_json::nlohmann_json
    fmt::fmt
    mgbl_api::mgbl_api
)

install(TARGETS mgbl_api_pbr_testing_tool
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_custom_target(clean-testing-tool
    COMMAND ${CMAKE_COMMAND} -E remove mgbl_api_pbr_testing_tool
    COMMENT "Cleaning testing tool binaries"
)