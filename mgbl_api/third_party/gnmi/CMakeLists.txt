find_program(_PROTOBUF_PROTOC protoc HINTS ${gRPC_BINDIR})
message(STATUS "Looking for protoc ... ${_PROTOBUF_PROTOC}")
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin HINTS ${gRPC_BINDIR})
message(STATUS "Looking for grpc_cpp_plugin ... ${_GRPC_CPP_PLUGIN_EXECUTABLE}")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

get_filename_component(GNMI_PROTO "protos/gnmi.proto" ABSOLUTE)
get_filename_component(GNMI_PROTO_PATH "${GNMI_PROTO}" PATH)

add_custom_command(
        COMMAND
            ${_PROTOBUF_PROTOC}
        ARGS
            --proto_path "${GNMI_PROTO_PATH}"
            --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${GNMI_PROTO}"
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.grpc.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.grpc.pb.cc
        DEPENDS
            "${GNMI_PROTO}"
)

get_filename_component(GNMI_EXT_PROTO "protos/gnmi_ext.proto" ABSOLUTE)
get_filename_component(GNMI_EXT_PROTO_PATH "${GNMI_EXT_PROTO}" PATH)

add_custom_command(
        COMMAND
            ${_PROTOBUF_PROTOC}
        ARGS
            --proto_path "${GNMI_EXT_PROTO_PATH}"
            --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/generated"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${GNMI_EXT_PROTO}"
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.grpc.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.grpc.pb.cc
        DEPENDS
            "${GNMI_EXT_PROTO}"
)


# Create a static library, which contains the rpc stubs and clients.
add_library(rpc_a OBJECT)
target_include_directories(rpc_a PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:generated/>
)
target_sources(rpc_a
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi.grpc.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/generated/gnmi_ext.grpc.pb.cc
        INTERFACE
            $<TARGET_OBJECTS:rpc_a>
        )
target_link_libraries(rpc_a PRIVATE PkgConfig::gRPC)