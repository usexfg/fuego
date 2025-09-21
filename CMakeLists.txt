cmake_minimum_required(VERSION 3.16)

project(fuegoX)

# --------------------------
# Version generation (must happen BEFORE add_subdirectory(src))
# --------------------------
set(VERSION "0.2")
set(COMMIT 72946d9)
set(REFS " (HEAD -> master)")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
configure_file("${CMAKE_SOURCE_DIR}/src/version.h.in" "${CMAKE_BINARY_DIR}/version/version.h")
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

message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")

# --------------------------
# CPU instruction set flags (AES-NI, SSE2, SSE4.1)
# --------------------------
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

set(AES_FLAGS "-maes" "-msse2" "-msse4.1")

# Check and add for C
foreach(flag ${AES_FLAGS})
    string(REPLACE "-" "" flag_var ${flag})
    check_c_compiler_flag(${flag} HAVE_C_${flag_var})
    if(HAVE_C_${flag_var})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
    endif()
endforeach()

# Check and add for C++
foreach(flag ${AES_FLAGS})
    string(REPLACE "-" "" flag_var ${flag})
    check_cxx_compiler_flag(${flag} HAVE_CXX_${flag_var})
    if(HAVE_CXX_${flag_var})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
    endif()
endforeach()

message(STATUS "Final C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Final CXX Flags: ${CMAKE_CXX_FLAGS}")

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

# --------------------------
# ICU handling
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
        message(FATAL_ERROR "ICU libraries not found on macOS. Install icu4c (Homebrew).")
    endif()
else()
    find_package(ICU REQUIRED COMPONENTS i18n uc data)
    if(ICU_FOUND)
        include_directories(${ICU_INCLUDE_DIRS})
        message(STATUS "Found ICU: ${ICU_LIBRARIES}")
    else()
        message(FATAL_ERROR "ICU not found on non-Apple platform.")
    endif()
endif()

# --------------------------
# Subdirectories
# --------------------------
add_subdirectory(external)
add_subdirectory(src)

# --------------------------
# ICU support
# --------------------------
find_package(ICU REQUIRED COMPONENTS uc i18n data)

if(APPLE)
    foreach(exec_target Daemon SimpleWallet Optimizer BurnDepositValidationService)
        if(TARGET ${exec_target})
            message(STATUS "Fixing ICU linkage for ${exec_target}")
            target_link_libraries(${exec_target} PRIVATE ICU::uc ICU::i18n ICU::data)
        endif()
    endforeach()
endif()
