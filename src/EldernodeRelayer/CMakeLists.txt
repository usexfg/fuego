# EldernodeRelayer module CMakeLists.txt

set(ELDERNODE_RELAYER_SOURCES
    EldernodeRelayer.cpp
)

set(ELDERNODE_RELAYER_HEADERS
    EldernodeRelayer.h
)

# Add the module to the parent project
add_library(EldernodeRelayer STATIC ${ELDERNODE_RELAYER_SOURCES})

# Link dependencies
target_link_libraries(EldernodeRelayer
    Common
    Crypto
    CryptoNoteCore
    BlockchainExplorer
    JsonRpcServer
)

# Include directories
target_include_directories(EldernodeRelayer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
