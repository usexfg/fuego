# DMWDA Test Suite
add_executable(DMWDA_TestSuite DMWDA_TestSuite.cpp)

target_link_libraries(DMWDA_TestSuite
    CryptoNoteCore
    Common
    Crypto
    Logging
)

target_include_directories(DMWDA_TestSuite PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME DMWDA_TestSuite COMMAND DMWDA_TestSuite)