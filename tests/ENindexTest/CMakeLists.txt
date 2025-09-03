add_executable(EldernodeIndexTest EldernodeIndexTest.cpp)

target_include_directories(EldernodeIndexTest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(EldernodeIndexTest
    EldernodeIndexManager
    CryptoNoteCore
    Common
    Crypto
    Logging
)
