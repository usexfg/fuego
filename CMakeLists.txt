cmake_minimum_required(VERSION 3.16)

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

find_package(Boost 1.74 REQUIRED COMPONENTS filesystem thread date_time chrono regex serialization program_options)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# --------------------------
# ICU
# --------------------------
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

# --------------------------
# Subdirectories
# --------------------------
add_subdirectory(external)
add_subdirectory(src)

# --------------------------
# Link ICU to executables (macOS)
# --------------------------
if(APPLE)
    foreach(exec_target fuegod walletd optimizer BurnDepositValidationService)
        if(TARGET ${exec_target})
            message(STATUS "Force-linking ICU to ${exec_target}")
            target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
            set_target_properties(${exec_target} PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "${ICU_LIBRARY_DIRS}"
            )
        endif()
    endforeach()
endif()
