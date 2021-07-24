cmake_minimum_required(VERSION 3.18.4)

project(transport_handbook)

set(CMAKE_CXX_STANDARD 17)

#add_compile_options(-Wall -Wextra -g -O3 -fsanitize=address -fno-omit-frame-pointer)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto graph.proto transport_router.proto)

set(SOURCES ${PROTO_SRCS} ${PROTO_HDRS} transport_catalogue.proto domain.h domain.cpp geo.h geo.cpp graph.h json.h json.cpp json_builder.h json_builder.cpp json_reader.h json_reader.cpp map_renderer.h map_renderer.cpp ranges.h request_handler.h request_handler.cpp router.h svg.h svg.cpp transport_catalogue.h transport_catalogue.cpp transport_router.h transport_router.cpp serialization.h serialization.cpp map_renderer.proto svg.proto graph.proto transport_router.proto)

add_executable(transport_catalogue main.cpp ${SOURCES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)