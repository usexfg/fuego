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

# Create the version target
configure_file(
  "${PROJECT_SOURCE_DIR}/src/version.h.in"
  "${PROJECT_BINARY_DIR}/version/version.h"
)
add_custom_target(version DEPENDS "${PROJECT_BINARY_DIR}/version/version.h")
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/version")

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
    else()
        message(FATAL_ERROR "ICU not found, install via Homebrew: brew install icu4c")
    endif()
endif()

# Find pthreads
find_package(Threads REQUIRED)

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

# --------------------------
# Subdirectories
# --------------------------
add_subdirectory(external)
add_subdirectory(src)

# --------------------------
# Tests
# --------------------------
# Expose a toggle to optionally build the test suite. This preserves all
# existing behavior when disabled and allows CI to turn tests on explicitly.
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    set(DO_TESTS ON)
    enable_testing()
    add_subdirectory(tests)
    
if(APPLE)
    foreach(exec_target Daemon SimpleWallet PaymentGateService Optimizer BurnDepositValidationService)
        if(TARGET ${exec_target})
            message(STATUS "Force-linking ICU to ${exec_target}")
            
            # Use specific ICU library paths if available, otherwise fall back to ICU_LIBRARIES
            if(ICU_DATA_LIBRARY AND ICU_I18N_LIBRARY AND ICU_UC_LIBRARY)
                target_link_libraries(${exec_target} PRIVATE 
                    ${ICU_DATA_LIBRARY}
                    ${ICU_I18N_LIBRARY}
                    ${ICU_UC_LIBRARY}
                )
                message(STATUS "Using specific ICU library paths for ${exec_target}")
            else()
                target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
                message(STATUS "Using ICU_LIBRARIES for ${exec_target}")
            endif()
            
            set_target_properties(${exec_target} PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "${ICU_LIBRARY_DIRS}"
                LINK_FLAGS "-L${ICU_LIBRARY_DIRS}"
            )
            # Add ICU library directory to link directories
            target_link_directories(${exec_target} PRIVATE ${ICU_LIBRARY_DIRS})
        endif()
    endforeach()
    endif()
endif()
# Intentionally avoid extra macOS ICU link manipulation here. Handled in src/CMakeLists.txt
