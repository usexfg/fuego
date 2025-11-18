# Tor Integration CMakeLists.txt

# Add Tor integration source files
set(TOR_INTEGRATION_SOURCES
    TorIntegration.cpp
)

# Add Tor integration header files
set(TOR_INTEGRATION_HEADERS
    ../../include/TorIntegration.h
)

# Create Tor integration library
add_library(TorIntegration STATIC ${TOR_INTEGRATION_SOURCES} ${TOR_INTEGRATION_HEADERS})

# Set include directories
target_include_directories(TorIntegration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link required libraries
if(WIN32)
    target_link_libraries(TorIntegration ws2_32)
else()
    target_link_libraries(TorIntegration pthread)
endif()

# Set C++ standard
target_compile_features(TorIntegration PRIVATE cxx_std_17)

# Compiler-specific options
if(MSVC)
    target_compile_options(TorIntegration PRIVATE /W4)
else()
    target_compile_options(TorIntegration PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add Tor integration to parent CMakeLists.txt
set(TOR_INTEGRATION_LIBRARY TorIntegration PARENT_SCOPE)
