cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

include(CheckCXXCompilerFlag)
set(VERSION "0.2")
# Packaged from main commits
set(COMMIT 72946d9)
set(REFS " (HEAD -> master)")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING INTERNAL)
set(CMAKE_SKIP_INSTALL_RULES ON)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set(CMAKE_SUPPRESS_REGENERATION ON)

project(fuegoX)

# --------------------------
# Version generation
# --------------------------
set(VERSION "0.2")
set(COMMIT 72946d9)
set(REFS " (HEAD -> master)")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
configure_file("src/version.h.in" "${CMAKE_BINARY_DIR}/version/version.h")
add_custom_target(version ALL)

# --------------------------
# C++ Standard
# --------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler with C++17")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    message(STATUS "Using MSVC compiler with C++17")
endif()

# Debug/Release flags
set(CMAKE_C_FLAGS_DEBUG   "-g3 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_C_FLAGS_RELEASE   "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")

# --------------------------
# Include directories
# --------------------------
include_directories(include src external "${CMAKE_BINARY_DIR}/version")

if(APPLE OR FREEBSD)
  include_directories(SYSTEM /usr/include/malloc)
  if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
  endif()
  enable_language(ASM)
endif()

if(MSVC)
  add_definitions("/bigobj /MP /W3 /GS- /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345 /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /DGTEST_HAS_TR1_TUPLE=0 /D__SSE4_1__")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10485760")
  if(STATIC)
    foreach(VAR CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE)
      string(REPLACE "/MD" "/MT" ${VAR} "${${VAR}}")
    endforeach()
  endif()
  include_directories(SYSTEM src/platform/msc)
  add_definitions(/DHAVE_ROTR)
else()
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # This option has no effect in glibc version less than 2.20.
    # Since glibc 2.20 _BSD_SOURCE is deprecated, this macro is recomended instead
    add_definitions("-D_DEFAULT_SOURCE -D_GNU_SOURCE")
	endif()

  set(WARNINGS "-Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=unused-function -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-error=type-limits -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized -Wno-error=unused-result")
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(WARNINGS "${WARNINGS} -Wno-error=mismatched-tags -Wno-error=null-conversion -Wno-overloaded-shift-op-parentheses -Wno-error=shift-count-overflow -Wno-error=tautological-constant-out-of-range-compare -Wno-error=unused-private-field -Wno-error=unneeded-internal-declaration -Wno-error=unused-function")
    add_definitions(/DHAVE_ROTR)
  else()
    set(WARNINGS "${WARNINGS} -Wlogical-op -Wno-error=maybe-uninitialized -Wno-error=clobbered -Wno-error=unused-but-set-variable")
  endif()
  if(MINGW)
    set(WARNINGS "${WARNINGS} -Wno-error=unused-value")
    set(MINGW_FLAG "-DWIN32_LEAN_AND_MEAN")
    include_directories(SYSTEM src/platform/mingw)
  else()
    set(MINGW_FLAG "")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_C_COMPILER_VERSION VERSION_LESS 5.1))
    set(WARNINGS "${WARNINGS} -Wno-error=odr")
    add_definitions(/DHAVE_ROTR)
  endif()
  set(C_WARNINGS "-Waggregate-return -Wnested-externs -Wold-style-definition -Wstrict-prototypes")
  set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 ${MINGW_FLAG} ${WARNINGS} ${C_WARNINGS}")
  if(NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${MINGW_FLAG} ${WARNINGS} ${CXX_WARNINGS}")
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_HAS_TR1_TUPLE=0")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8))
    set(DEBUG_FLAGS "-g3 -Og")
  else()
    set(DEBUG_FLAGS "-g3 -O0")
  endif()
  set(RELEASE_FLAGS "-g3 -O0")

endif()

if(APPLE)
	add_definitions(/DHAVE_ROTR)
endif()

if(STATIC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

if(APPLE)
  # Try to find Boost in Homebrew directories
  set(HOMEBREW_BOOST_ROOT "/opt/homebrew/opt/boost")
  if(EXISTS "${HOMEBREW_BOOST_ROOT}")
    set(Boost_ROOT "${HOMEBREW_BOOST_ROOT}")
    set(Boost_INCLUDE_DIR "${HOMEBREW_BOOST_ROOT}/include")
    set(Boost_LIBRARY_DIR "${HOMEBREW_BOOST_ROOT}/lib")
    # Set CMAKE_PREFIX_PATH to help CMake find Boost components
    set(CMAKE_PREFIX_PATH "${HOMEBREW_BOOST_ROOT}" ${CMAKE_PREFIX_PATH})
    # Force CMake to use the legacy FindBoost module
    set(Boost_NO_BOOST_CMAKE ON)
    # For Homebrew Boost, we need to manually set the libraries
    set(Boost_LIBRARIES "")
    # Find individual Boost libraries manually (skip system as it doesn't exist in Boost 1.89.0)
    find_library(BOOST_FILESYSTEM_LIB boost_filesystem PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_THREAD_LIB boost_thread PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_DATE_TIME_LIB boost_date_time PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_CHRONO_LIB boost_chrono PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_REGEX_LIB boost_regex PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_SERIALIZATION_LIB boost_serialization PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    find_library(BOOST_PROGRAM_OPTIONS_LIB boost_program_options PATHS "${HOMEBREW_BOOST_ROOT}/lib" NO_DEFAULT_PATH)
    
    if(BOOST_FILESYSTEM_LIB AND BOOST_THREAD_LIB AND BOOST_DATE_TIME_LIB AND BOOST_CHRONO_LIB AND BOOST_REGEX_LIB AND BOOST_SERIALIZATION_LIB AND BOOST_PROGRAM_OPTIONS_LIB)
      set(Boost_LIBRARIES "${BOOST_FILESYSTEM_LIB};${BOOST_THREAD_LIB};${BOOST_DATE_TIME_LIB};${BOOST_CHRONO_LIB};${BOOST_REGEX_LIB};${BOOST_SERIALIZATION_LIB};${BOOST_PROGRAM_OPTIONS_LIB}")
      set(Boost_FOUND TRUE)
      set(Boost_INCLUDE_DIRS "${HOMEBREW_BOOST_ROOT}/include")
      set(Boost_LIBRARY_DIRS "${HOMEBREW_BOOST_ROOT}/lib")
    endif()
  endif()
endif()

# Windows Boost configuration
if(WIN32)
  # Check for environment variable first
  if(DEFINED ENV{BOOST_ROOT})
    set(BOOST_ROOT "$ENV{BOOST_ROOT}")
    set(Boost_ROOT "$ENV{BOOST_ROOT}")
    set(Boost_INCLUDE_DIR "$ENV{BOOST_INCLUDEDIR}")
    set(Boost_LIBRARY_DIR "$ENV{BOOST_LIBRARYDIR}")
  else()
    # Fallback to common Boost installation paths
    set(BOOST_ROOT "C:/thirdparties/boost-1.83.0")
    set(Boost_ROOT "C:/thirdparties/boost-1.83.0")
    set(Boost_INCLUDE_DIR "C:/thirdparties/boost-1.83.0/boost")
    set(Boost_LIBRARY_DIR "C:/thirdparties/boost-1.83.0/stage/lib")
  endif()

  # Additional Windows-specific Boost configuration
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

# Set policy to use legacy FindBoost module for Boost 1.70+ compatibility
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

# Set policy to suppress CMP0074 warning about Boost_ROOT
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

#set(Boost_DEBUG on)
if(NOT Boost_FOUND)
  if(APPLE)
    # For macOS, skip system component as it doesn't exist in Boost 1.89.0
    find_package(Boost 1.55 REQUIRED COMPONENTS filesystem thread date_time chrono regex serialization program_options)
  else()
    find_package(Boost 1.55 REQUIRED COMPONENTS system filesystem thread date_time chrono regex serialization program_options)
  endif()
endif()
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# Platform-specific Boost library configurations
if(MINGW)
  set(Boost_LIBRARIES "${Boost_LIBRARIES};ws2_32;mswsock")
elseif(APPLE)
    include_directories(src/Platform/OSX)
else()
    include_directories(src/Platform/Linux)
endif()

# --------------------------
# Boost
# --------------------------
if(APPLE)
    if(EXISTS /opt/homebrew)
        set(BOOST_ROOT /opt/homebrew)
    elseif(EXISTS /usr/local)
        set(BOOST_ROOT /usr/local)
    endif()
endif()

# First try with all required components
# Note: boost_system is header-only in Boost 1.69+ so we don't require it as a component
set(BOOST_COMPONENTS filesystem thread date_time chrono regex serialization program_options)

# Try to find coroutine component (may not be available in all Boost versions)
find_package(Boost 1.74 COMPONENTS coroutine QUIET)
if(Boost_COROUTINE_FOUND)
    list(APPEND BOOST_COMPONENTS coroutine)
    message(STATUS "Boost coroutine component found, including in build")
else()
    message(WARNING "Boost coroutine component not found - using alternative context implementation")
endif()

# Find Boost with available components
find_package(Boost 1.74 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# --------------------------
# ICU (skipped on Android)
# --------------------------
if(ANDROID)
    message(STATUS "Android build detected: skipping ICU discovery")
else()
    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/icu4c" "/usr/local/opt/icu4c")
    endif()

    find_package(ICU REQUIRED COMPONENTS data i18n uc)

    if(ICU_FOUND)
        message(STATUS "Found ICU includes: ${ICU_INCLUDE_DIRS}")
        message(STATUS "Found ICU libraries: ${ICU_LIBRARIES}")
        include_directories(${ICU_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "ICU not found, install via Homebrew: brew install icu4c")
    endif()

    # On macOS, ensure we have the correct ICU library paths
    if(APPLE)
        if(ICU_DATA_LIBRARY)
            message(STATUS "ICU Data Library: ${ICU_DATA_LIBRARY}")
        endif()
        if(ICU_I18N_LIBRARY)
            message(STATUS "ICU I18N Library: ${ICU_I18N_LIBRARY}")
        endif()
        if(ICU_UC_LIBRARY)
            message(STATUS "ICU UC Library: ${ICU_UC_LIBRARY}")
        endif()
    endif()
endif()

# --------------------------
# Testing
# --------------------------
option(BUILD_TESTS "Build test executables" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --------------------------
# Subdirectories
# --------------------------
add_subdirectory(external)
add_subdirectory(src)

# --------------------------
# ICU linking for executables (macOS)
# --------------------------
if(APPLE AND ICU_FOUND)
    message(STATUS "Linking ICU libraries to executables")
    foreach(exec_target Daemon SimpleWallet PaymentGateService Optimizer)
        if(TARGET ${exec_target})
            message(STATUS "Linking ICU to ${exec_target}")
            target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
        endif()
    endforeach()
endif()
