#############################
# CMake - ROS Bridge
#############################

cmake_minimum_required(VERSION 3.5.1)

project("ROS_Bridge")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(VERSION_TYPE "beta" CACHE STRING "version type" FORCE)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
site_name(VERSION_HOST) # read hostname to VERSION_HOST
set(VERSION_HOST "${VERSION_HOST}" CACHE STRING "host of build" FORCE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
add_compile_options(-fvisibility=hidden -Os -flto)
add_compile_options(-Wno-psabi)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.67.0 REQUIRED COMPONENTS thread)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_package(RapidJSON REQUIRED)

find_package(amm_std REQUIRED)

add_subdirectory(src)

file(COPY config DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

message(STATUS "")
message(STATUS "    == Final overview for ${PROJECT_NAME} ==")
message(STATUS "Version:              ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} ${VERSION_TYPE} @ ${VERSION_HOST}")
message(STATUS "Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Output:               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Compiler:             ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE:     ${CMAKE_BUILD_TYPE}")
message(STATUS "")

include(Packing)
