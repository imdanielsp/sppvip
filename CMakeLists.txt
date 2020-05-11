cmake_minimum_required (VERSION 2.8)
project(sppvip)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_LIBRARY_DIR ${PROJECT_SOURCE_DIR}/lib)
set(LIBRARY_OUTPUT_PATH ${CMAKE_LIBRARY_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXE_LINKER_FLAGS "-framework AudioToolBox -framework CoreFoundation")

# BUILD TYPE
message("A ${CMAKE_BUILD_TYPE} build configuration is detected")

find_package(Boost 1.71.0)
include_directories("${Boost_INCLUDE_DIRS}")

file(GLOB SOURCE_FILES "./*")

# Removes client_test.cpp because it is an executable
list(REMOVE_ITEM SOURCE_FILES "client_test.cpp")

add_executable("sppvip" ${SOURCE_FILES})

add_executable("client_test" "client_test.cpp")