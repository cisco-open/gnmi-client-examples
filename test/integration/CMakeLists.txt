cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)

project(integration_tests LANGUAGES C CXX)

include(CTest)
enable_testing()
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(mgbl_api REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET protobuf grpc++)
pkg_check_modules(GTest REQUIRED IMPORTED_TARGET gtest gtest_main gmock)

add_executable(
  gnmi_integration_test
  tests/gnmi_integration_tests.cpp
)
target_link_libraries(
  gnmi_integration_test
  mgbl_api::mgbl_api
  nlohmann_json::nlohmann_json
  fmt::fmt
  PkgConfig::gRPC
  PkgConfig::GTest ${CMAKE_THREAD_LIBS_INIT}
)

include(GoogleTest)
gtest_discover_tests(gnmi_integration_test)
# Generating Python bindings for mock server
option(ENABLE_PYTHON_PROTO_GEN "Enable Python proto generation" OFF)
message(STATUS "ENABLE_PYTHON_PROTO_GEN: ${ENABLE_PYTHON_PROTO_GEN}")

if(ENABLE_PYTHON_PROTO_GEN)
    set(PYTHON_PROTO_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/server/generated)
    file(MAKE_DIRECTORY ${PYTHON_PROTO_GEN_DIR})
    message(STATUS "PYTHON_PROTO_GEN_DIR: ${PYTHON_PROTO_GEN_DIR}")

    add_custom_command(
        OUTPUT ${VENV_DIR}/bin/activate
        COMMAND python3 -m venv ${CMAKE_BINARY_DIR}/venv
        COMMAND ${CMAKE_BINARY_DIR}/venv/bin/pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/server/requirements.txt
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/server/requirements.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating virtual environment and installing dependencies"
    )
    add_custom_target(
        CreateVirtualEnv ALL
        DEPENDS ${VENV_DIR}/bin/activate
    )
    foreach(proto_file ${PROTO_FILES})
        get_filename_component(proto_name ${proto_file} NAME_WE)
        message(STATUS "Processing proto file: ${proto_file}")
        add_custom_command(
            OUTPUT ${PYTHON_PROTO_GEN_DIR}/${proto_name}_pb2.py ${PYTHON_PROTO_GEN_DIR}/${proto_name}_pb2_grpc.py
            COMMAND ${CMAKE_BINARY_DIR}/venv/bin/python -m grpc_tools.protoc
            ARGS --proto_path=${GNMI_PROTO_DIR}
                 --python_out=${PYTHON_PROTO_GEN_DIR}
                 --grpc_python_out=${PYTHON_PROTO_GEN_DIR}
                 ${proto_file}
            DEPENDS ${proto_file} ${CMAKE_BINARY_DIR}/venv/bin/activate
        )
    endforeach()
    add_custom_target(GeneratePythonFiles ALL DEPENDS ${PYTHON_PROTO_GEN_DIR}/gnmi_pb2.py ${PYTHON_PROTO_GEN_DIR}/gnmi_pb2_grpc.py ${GNMI_GEN_DIR}/gnmi_ext_pb2.py ${PYTHON_PROTO_GEN_DIR}/gnmi_ext_pb2_grpc.py)
endif()