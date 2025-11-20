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
# Removed CMAKE_SKIP_INSTALL_RULES to allow installation rules
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -maes -mpclmul")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes -mpclmul")
    message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler with C++17 and AES-NI support")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
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
    include_directories(src/Platform/Windows)
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
