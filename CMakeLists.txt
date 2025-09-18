cmake_minimum_required(VERSION 3.8)

# Suppress CMake policies warning for newer CMake versions
cmake_policy(SET CMP0144 NEW) # Handles BOOST_ROOT behavior
cmake_policy(SET CMP0167 NEW) # Handles modern FindBoost usage

project(fuegoX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_VERSION_MAJOR 10)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")

configure_file(
  "${PROJECT_SOURCE_DIR}/src/version.h.in"
  "${PROJECT_BINARY_DIR}/version/version.h"
)
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/version")

#
# Platform-specific compiler flags
#
option(ARCH "Choose architecture" "x86-64")

set(ARM 0) # Default to NOT ARM
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  # We are on an ARM processor, but it could be an emulated x86 build (Rosetta)
  # Check if the compiler supports ARM-specific flags to confirm a native ARM build
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-march=armv8-a+fp+simd" CXX_SUPPORTS_ARMV8)
  if(CXX_SUPPORTS_ARMV8)
    message(STATUS "Native ARM architecture detected.")
    set(ARM 1)
  else()
    message(STATUS "System processor is ARM, but compiler does not support ARM flags. Assuming x86 emulation.")
  endif()
endif()

if(ARM)
  include(arm)
endif()

# Set common compiler flags
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHsc")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=unused-function -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-error=type-limits -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized -Wno-error=unused-result -Wno-deprecated-builtins -Wno-error=unused-but-set-variable -Wno-reorder -Wno-missing-field-initializers")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=unused-function -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-error=type-limits -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized -Wno-error=unused-result -Wno-deprecated-builtins -Wno-error=unused-but-set-variable -Waggregate-return -Wnested-externs -Wold-style-definition -Wstrict-prototypes")
endif()

if(NOT ARM AND NOT MSVC)
  include(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-maes" C_SUPPORTS_MAES)
  if(C_SUPPORTS_MAES)
    message(STATUS "AES-NI hardware acceleration is available. Enabling -maes flag.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
  else()
     message(STATUS "AES-NI hardware acceleration is not available.")
  endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

#
# End of platform-specific compiler flags
#

# Suppress CMake policies warning
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

if(APPLE)
  include_directories(SYSTEM /usr/include/malloc)
  # brew install boost
  set(BOOST_ROOT "/opt/homebrew")
  set(Boost_NO_SYSTEM_PATHS ON)
endif()

if(UNIX AND NOT APPLE)
  # sudo apt-get install libboost-all-dev
  set(Boost_USE_STATIC_LIBS ON)
endif()

if(WIN32)
  set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

# Find Boost
find_package(Boost 1.74 REQUIRED COMPONENTS
  filesystem
  thread
  date_time
  chrono
  regex
  serialization
  program_options
)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Could not find Boost. Please install Boost 1.74 or newer.")
else()
  message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

# Additional include directories
include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/external
)

# Set platform-specific sources
if(UNIX OR APPLE)
  set(PLATFORM_DIR "Posix")
else()
  set(PLATFORM_DIR "Windows")
endif()
include_directories(${PROJECT_SOURCE_DIR}/src/Platform/${PLATFORM_DIR})

# External dependencies
add_subdirectory(external)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Set build output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/src)

option(STATIC "Build static libraries" ON)
set(STATICLIB ${STATIC})