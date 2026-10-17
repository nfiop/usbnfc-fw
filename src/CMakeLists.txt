cmake_minimum_required(VERSION 3.20)

# Initialize platform SDK and any dependecies before declaring the
# project.
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/sdk.cmake)

project(usbnfc_fw C CXX ASM)

# Define ${PLATFORM_SOURCES} variable, possibly C and C++ standards as well
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/platform.cmake)

add_executable(usbnfc_fw
    ${PLATFORM_SOURCES}
)

# Define libraries, dependecies, flags for the platform, after the project is
# declared...
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/platform_extras.cmake)
