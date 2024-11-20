cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)

project(unit_tests LANGUAGES C CXX)

include(CTest)
enable_testing()
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(mgbl_api REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET protobuf grpc++)
pkg_check_modules(GTest REQUIRED IMPORTED_TARGET gtest gtest_main gmock)

add_executable(mgbl_api_test
    mgbl_api_test.cpp
    mgbl_api_test_edge_cases.cpp
    gnmi/mgbl_gnmi_client_test.cpp
    gnmi/mgbl_gnmi_helper_test.cpp
    gnmi/mgbl_gnmi_helper_test_edge_cases.cpp
    pbr/mgbl_pbr_test.cpp
    pbr/mgbl_pbr_test_edge_cases.cpp
)

target_include_directories(mgbl_api_test PRIVATE test/unit)
target_link_libraries(
    mgbl_api_test
    mgbl_api::mgbl_api
    PkgConfig::gRPC
    nlohmann_json::nlohmann_json
    fmt::fmt
    PkgConfig::GTest ${CMAKE_THREAD_LIBS_INIT}
)

include(GoogleTest)
gtest_discover_tests(mgbl_api_test)
