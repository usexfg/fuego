# Dynamic Supply Simulation Test
add_executable(dynamic_supply_simulation
    dynamic_supply_simulation.cpp
)

target_link_libraries(dynamic_supply_simulation
    CryptoNoteCore
    Common
    Crypto
    Logging
)

# Set include directories
target_include_directories(dynamic_supply_simulation PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Set compile definitions
target_compile_definitions(dynamic_supply_simulation PRIVATE
    NDEBUG
    LOGGING_LEVEL=2
)
