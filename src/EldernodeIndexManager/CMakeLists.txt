set(ELDERNODE_INDEX_MANAGER_SOURCES
    EldernodeIndexManager.cpp
    EldernodeIndexUtils.cpp
    ElderfierDepositManager.cpp
    EldernodeRandomSelector.cpp
)

add_library(EldernodeIndexManager STATIC ${ELDERNODE_INDEX_MANAGER_SOURCES})

target_include_directories(EldernodeIndexManager PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(EldernodeIndexManager
    CryptoNoteCore
    Common
    Crypto
    Logging
)
