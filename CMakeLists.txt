cmake_minimum_required(VERSION 3.16)
project(fuegoX)

# --------------------------
# Version and commit info
# --------------------------
set(VERSION "0.2")
set(COMMIT 72946d9)
set(REFS " (HEAD -> master)")

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

message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")

# --------------------------
# Build configuration
# --------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING INTERNAL)
set(CMAKE_SKIP_INSTALL_RULES ON)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set(CMAKE_SUPPRESS_REGENERATION ON)

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
# Architecture flags
# --------------------------
include(TestCXXAcceptsFlag)
include(CheckCCompilerFlag)

if(NOT ARCH)
    set(ARCH native CACHE STRING "CPU to build for: -march value or 'default' to not pass -march at all")
endif()

message(STATUS "Building on ${CMAKE_SYSTEM_PROCESSOR}")

set(ARCH_FLAG "")
if(NOT ARCH STREQUAL "default")
    set(ARCH_FLAG "-march=${ARCH}")
endif()

# --------------------------
# ARM / AES flags
# --------------------------
include(arm.cmake)

option(NO_AES "Explicitly disable AES support" ${NO_AES})
if(NO_AES)
    message(STATUS "AES support explicitly disabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_AES")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_AES")
elseif(NOT ARM)
    message(STATUS "AES support enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAG}")

# --------------------------
# Static linking option
# --------------------------
set(STATIC ${MSVC} CACHE BOOL "Link libraries statically")

if(MSVC)
    add_definitions("/bigobj /MP /W3 /GS- /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345 /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /DGTEST_HAS_TR1_TUPLE=0 /D_VARIADIC_MAX=8 /D__SSE4_1__")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10485760")
    if(STATIC)
        foreach(VAR CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE)
            string(REPLACE "/MD" "/MT" ${VAR} "${${VAR}}")
        endforeach()
    endif()
else()
    add_definitions("-D_DEFAULT_SOURCE -D_GNU_SOURCE")
    set(C_WARNINGS "-Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=unused-function -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized -Wno-error=unused-result -Wno-deprecated-builtins")
    set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 ${C_WARNINGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${CXX_WARNINGS} -pthread")
endif()

# --------------------------
# Boost
# --------------------------
if(STATIC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()

if(APPLE)
    if(EXISTS /opt/homebrew)
        set(BOOST_ROOT /opt/homebrew)
        set(Boost_INCLUDEDIR /opt/homebrew/include)
        set(Boost_LIBRARYDIR /opt/homebrew/lib)
    elseif(EXISTS /usr/local)
        set(BOOST_ROOT /usr/local)
        set(Boost_INCLUDEDIR /usr/local/include)
        set(Boost_LIBRARYDIR /usr/local/lib)
    endif()
endif()

find_package(Boost 1.74 REQUIRED COMPONENTS filesystem thread date_time chrono regex serialization program_options)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if(MINGW)
    set(Boost_LIBRARIES "${Boost_LIBRARIES};ws2_32;mswsock")
elseif(APPLE)
    set(Boost_LIBRARIES "${Boost_LIBRARIES}")
elseif(NOT MSVC)
    set(Boost_LIBRARIES "${Boost_LIBRARIES};rt")
endif()

# --------------------------
# ICU handling (macOS full paths)
# --------------------------
if(APPLE)
    find_path(ICU_INCLUDE_DIR icu.h HINTS /opt/homebrew/opt/icu4c/include /usr/local/opt/icu4c/include)
    find_library(ICU_DATA_LIBRARY NAMES libicudata.dylib icudata HINTS /opt/homebrew/opt/icu4c/lib /usr/local/opt/icu4c/lib)
    find_library(ICU_I18N_LIBRARY NAMES libicui18n.dylib icui18n HINTS /opt/homebrew/opt/icu4c/lib /usr/local/opt/icu4c/lib)
    find_library(ICU_UC_LIBRARY NAMES libicuuc.dylib icuuc HINTS /opt/homebrew/opt/icu4c/lib /usr/local/opt/icu4c/lib)

    if(ICU_INCLUDE_DIR AND ICU_DATA_LIBRARY AND ICU_I18N_LIBRARY AND ICU_UC_LIBRARY)
        include_directories(SYSTEM ${ICU_INCLUDE_DIR})
        set(ICU_LIBRARIES ${ICU_DATA_LIBRARY} ${ICU_I18N_LIBRARY} ${ICU_UC_LIBRARY})
        message(STATUS "Found ICU (macOS): ${ICU_LIBRARIES}")
    else()
        message(FATAL_ERROR "ICU libraries not found on macOS. Install icu4c via Homebrew.")
    endif()
else()
    find_package(ICU REQUIRED COMPONENTS i18n uc data)
    if(ICU_FOUND)
        include_directories(${ICU_INCLUDE_DIRS})
        set(ICU_LIBRARIES ${ICU_LIBRARIES})
        message(STATUS "Found ICU: ${ICU_LIBRARIES}")
    else()
        message(FATAL_ERROR "ICU not found on non-Apple platform.")
    endif()
endif()

# --------------------------
# Version generation
# --------------------------
# ... [keep your existing version generation code] ...

# --------------------------
# Subdirectories
# --------------------------
add_subdirectory(external)
add_subdirectory(src)

# --------------------------
# Fix ICU linking for executables (keyword form)
# --------------------------
if(APPLE)
    foreach(exec_target fuegod PaymentGateService SimpleWallet Optimizer BurnDepositValidationService)
        if(TARGET ${exec_target})
            message(STATUS "Fixing ICU linkage for ${exec_target}")
            target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
        endif()
    endforeach()
endif()

# --------------------------
# Tests
# --------------------------
if(BUILD_TESTS)
    add_subdirectory(tests)
    enable_testing()
endif()
