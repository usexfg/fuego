add_definitions(-DSTATICLIB)
include_directories(${CMAKE_SOURCE_DIR}/external/parallel_hashmap)

file(GLOB_RECURSE BlockchainExplorer BlockchainExplorer/*)
file(GLOB_RECURSE Common Common/*)
file(GLOB_RECURSE Crypto crypto/*)
file(GLOB_RECURSE CryptoNoteCore CryptoNoteCore/* CryptoNoteConfig.h DynamicRingSize.cpp OSPEADDecoySelection.cpp)
file(GLOB_RECURSE CryptoNoteProtocol CryptoNoteProtocol/*)
file(GLOB_RECURSE Daemon Daemon/*)
file(GLOB_RECURSE Http HTTP/*)
file(GLOB_RECURSE InProcessNode InProcessNode/*)
file(GLOB_RECURSE Logging Logging/*)
# Optimizer sources
file(GLOB_RECURSE NodeRpcProxy NodeRpcProxy/*)
file(GLOB_RECURSE P2p P2p/*)
file(GLOB_RECURSE Rpc Rpc/*)
file(GLOB_RECURSE Serialization Serialization/*)
# Source files defined directly in add_executable calls

file(GLOB_RECURSE Transfers Transfers/*)
file(GLOB_RECURSE Wallet Wallet/* BurnTransactionHandler.cpp)
file(GLOB_RECURSE WalletLegacy WalletLegacy/*)
file(GLOB_RECURSE JsonRpcServer JsonRpcServer/*)
file(GLOB_RECURSE PaymentGate PaymentGate/*)
file(GLOB_RECURSE PaymentGateService PaymentGateService/*)

# Eldernode services
add_subdirectory(EldernodeIndexManager)
add_subdirectory(BurnDepositValidationService)

# System files
if(WIN32)
  file(GLOB_RECURSE System System/* Platform/Windows/System/*)
elseif(APPLE)
  file(GLOB_RECURSE System System/* Platform/OSX/System/*)
else()
  file(GLOB_RECURSE System System/* Platform/Linux/System/*)
endif()

source_group("" FILES ${Common} ${Crypto} ${CryptoNoteCore} ${CryptoNoteProtocol} ${Daemon} ${JsonRpcServer} ${Http} ${Logging} ${NodeRpcProxy} ${P2p} ${Rpc} ${Serialization} ${SimpleWallet} ${System} ${Transfers} ${Wallet} ${WalletLegacy})

# --------------------------
# Libraries
# --------------------------
add_library(BlockchainExplorer STATIC ${BlockchainExplorer})
add_library(Common STATIC ${Common})
add_library(Crypto STATIC ${Crypto})
add_library(CryptoNoteCore STATIC ${CryptoNoteCore})
add_library(Http STATIC ${Http})
add_library(InProcessNode STATIC ${InProcessNode})
add_library(Logging STATIC ${Logging})
add_library(NodeRpcProxy STATIC ${NodeRpcProxy})
add_library(Rpc STATIC ${Rpc})
add_library(P2P STATIC ${CryptoNoteProtocol} ${P2p})

if(TARGET upnpc-static)
    add_dependencies(P2P upnpc-static)
    target_link_libraries(P2P PUBLIC upnpc-static)
    target_include_directories(P2P PUBLIC 
        ${CMAKE_SOURCE_DIR}/external/miniupnpc
        ${CMAKE_BINARY_DIR}/external/miniupnpc
    )
else()
    message(FATAL_ERROR "upnpc-static target not found. MiniUPnPc static library is required for UPnP support.")
endif()
add_library(Serialization STATIC ${Serialization})
add_library(System STATIC ${System})
add_library(Transfers STATIC ${Transfers})
add_library(Wallet STATIC ${Wallet} ${WalletLegacy})
add_library(PaymentGate STATIC ${PaymentGate})
add_library(JsonRpcServer STATIC ${JsonRpcServer})

# --------------------------
# Apply AES/SSE compile options to Crypto library only (x86/x64 non-Android)
# --------------------------
if ((CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    AND NOT ANDROID
    AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i[3-6]86|amd64|x86_64"))
    target_compile_options(Crypto PRIVATE -maes -msse2 -msse4.1)
endif()

# Force AES support for Apple Silicon (Rosetta) builds
if (CMAKE_SYSTEM_NAME MATCHES "Darwin" AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
    # For Rosetta builds, disable hardware AES and use software AES
    target_compile_definitions(Crypto PRIVATE NO_AES)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_AES")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_AES")
else()
    # Enable hardware AES for native x86_64 builds
    target_compile_options(Crypto PRIVATE -maes -msse2 -msse4.1)
endif()

# --------------------------
# Executables
# --------------------------
# Work around CMake 4.1.1 bug by specifying all sources in add_executable
add_executable(Daemon
    "${CMAKE_CURRENT_SOURCE_DIR}/Daemon/Daemon.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Daemon/DaemonCommandsHandler.cpp"
)

add_executable(SimpleWallet
    "${CMAKE_CURRENT_SOURCE_DIR}/SimpleWallet/SimpleWallet.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SimpleWallet/PasswordContainer.cpp"
)

add_executable(PaymentGateService
    "${CMAKE_CURRENT_SOURCE_DIR}/PaymentGateService/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PaymentGateService/PaymentGateService.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PaymentGateService/ConfigurationManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PaymentGateService/PaymentServiceConfiguration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PaymentGateService/RpcNodeConfiguration.cpp"
)
add_executable(Optimizer "${CMAKE_CURRENT_SOURCE_DIR}/Optimizer/Optimizer.cpp")

# --------------------------
# System library for Windows
# --------------------------
if (MSVC)
  target_link_libraries(System PRIVATE ws2_32)
endif ()

# --------------------------
# Link libraries (keyword form)
# --------------------------
target_link_libraries(Daemon PRIVATE
    CryptoNoteCore
    P2P
    Rpc
    System
    Http
    Logging
    Common
    Crypto
    BlockchainExplorer
    ${Boost_LIBRARIES}
    Serialization
)

target_link_libraries(SimpleWallet PRIVATE
    Wallet
    NodeRpcProxy
    Transfers
    Rpc
    Http
    CryptoNoteCore
    System
    Logging
    Common
    Crypto
    BlockchainExplorer
    ${Boost_LIBRARIES}
    Serialization
)

target_link_libraries(PaymentGateService PRIVATE
    PaymentGate
    JsonRpcServer
    Wallet
    NodeRpcProxy
    Transfers
    CryptoNoteCore
    Crypto
    P2P
    Rpc
    Http
    System
    Logging
    Common
    InProcessNode
    BlockchainExplorer
    ${Boost_LIBRARIES}
    Serialization
)

target_link_libraries(Optimizer PRIVATE
    PaymentGate
    Rpc
    Http
    System
    Logging
    Common
    Crypto
    ${Boost_LIBRARIES}
    Serialization
)

# --------------------------
# ICU linking is handled in root CMakeLists.txt
# --------------------------

# --------------------------
# Additional Linux linking
# --------------------------
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR APPLE AND NOT ANDROID)
  target_link_libraries(SimpleWallet PRIVATE -lresolv)
  target_link_libraries(Daemon PRIVATE -lresolv)
  target_link_libraries(PaymentGateService PRIVATE -lresolv)
  target_link_libraries(Optimizer PRIVATE -lresolv)
endif ()

# --------------------------
# Dependencies
# --------------------------
add_dependencies(Rpc version)
add_dependencies(Daemon version)
add_dependencies(SimpleWallet version)
add_dependencies(PaymentGateService version)
add_dependencies(Optimizer version)
add_dependencies(P2P version)

# --------------------------
# Output names
# --------------------------
set_property(TARGET SimpleWallet PROPERTY OUTPUT_NAME "fuego-wallet-cli")
set_property(TARGET PaymentGateService PROPERTY OUTPUT_NAME "walletd")
set_property(TARGET Daemon PROPERTY OUTPUT_NAME "fuegod")
set_property(TARGET Optimizer PROPERTY OUTPUT_NAME "fuego-optimizer")
