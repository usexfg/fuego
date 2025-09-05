add_definitions(-DSTATICLIB)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ../version)

file(GLOB_RECURSE CoreTests CoreTests/*)
file(GLOB_RECURSE IntegrationTestLibrary IntegrationTestLib/*)
file(GLOB_RECURSE IntegrationTests IntegrationTests/*)
file(GLOB_RECURSE FunctionalTests FunctionalTests/*)
file(GLOB_RECURSE NodeRpcProxyTests NodeRpcProxyTests/*)
file(GLOB_RECURSE PerformanceTests PerformanceTests/*)
file(GLOB_RECURSE SystemTests System/*)
file(GLOB_RECURSE TestGenerator TestGenerator/*)
file(GLOB_RECURSE CryptoNoteProtocol ../src/CryptoNoteProtocol/*)
file(GLOB_RECURSE P2p ../src/P2p/*)

source_group("" FILES ${CoreTests} ${CryptoTests} ${FunctionalTests} ${NodeRpcProxyTests} ${PerformanceTests} ${SystemTests} ${TestGenerator} )
source_group("" FILES ${CryptoNoteProtocol} ${P2p})

add_library(IntegrationTestLibrary ${IntegrationTestLibrary})
add_library(TestGenerator ${TestGenerator})

add_executable(CoreTests ${CoreTests})
add_executable(IntegrationTests ${IntegrationTests})
add_executable(NodeRpcProxyTests ${NodeRpcProxyTests})
add_executable(PerformanceTests ${PerformanceTests})
add_executable(SystemTests ${SystemTests})
add_executable(DifficultyTests Difficulty/Difficulty.cpp)

# Dynamic Supply Tests
add_executable(DynamicMoneySupplyTest UnitTests/TestDynamicMoneySupply.cpp)
add_executable(DynamicSupplyIntegrationTest UnitTests/TestDynamicSupplyIntegration.cpp)
add_executable(DynamicSupplyPerformanceTest UnitTests/TestDynamicSupplyPerformance.cpp)

target_link_libraries(CoreTests TestGenerator CryptoNoteCore Serialization System Logging Common Crypto BlockchainExplorer ${Boost_LIBRARIES})
target_link_libraries(IntegrationTests IntegrationTestLibrary Wallet NodeRpcProxy InProcessNode P2P Rpc Http Transfers Serialization System CryptoNoteCore Logging Common Crypto BlockchainExplorer gtest upnpc-static ${Boost_LIBRARIES})
target_link_libraries(NodeRpcProxyTests NodeRpcProxy CryptoNoteCore Rpc Http Serialization System Logging Common Crypto ${Boost_LIBRARIES})
target_link_libraries(PerformanceTests CryptoNoteCore Serialization Logging Common Crypto ${Boost_LIBRARIES})
target_link_libraries(SystemTests System gtest_main)

# Dynamic Supply Test Libraries
target_link_libraries(DynamicMoneySupplyTest CryptoNoteCore Serialization System Logging Common Crypto gtest ${Boost_LIBRARIES})
target_link_libraries(DynamicSupplyIntegrationTest CryptoNoteCore Serialization System Logging Common Crypto gtest ${Boost_LIBRARIES})
target_link_libraries(DynamicSupplyPerformanceTest CryptoNoteCore Serialization System Logging Common Crypto gtest ${Boost_LIBRARIES})
if (MSVC)
  target_link_libraries(SystemTests ws2_32)
  target_link_libraries(NodeRpcProxyTests ws2_32)
  target_link_libraries(CoreTests ws2_32)
endif ()
target_link_libraries(DifficultyTests CryptoNoteCore Serialization Crypto Logging Common ${Boost_LIBRARIES})


add_custom_target(tests DEPENDS NodeRpcProxyTests PerformanceTests SystemTests DifficultyTests DynamicMoneySupplyTest DynamicSupplyIntegrationTest DynamicSupplyPerformanceTest )

set_property(TARGET
  tests
  CoreTests
  IntegrationTestLibrary
  TestGenerator
  IntegrationTests
  NodeRpcProxyTests
  PerformanceTests
  SystemTests
  DifficultyTests
  DynamicMoneySupplyTest
  DynamicSupplyIntegrationTest
  DynamicSupplyPerformanceTest

PROPERTY FOLDER "tests")

set_property(TARGET CoreTests PROPERTY OUTPUT_NAME "core_tests")
set_property(TARGET IntegrationTests PROPERTY OUTPUT_NAME "integration_tests")
set_property(TARGET NodeRpcProxyTests PROPERTY OUTPUT_NAME "node_rpc_proxy_tests")
set_property(TARGET PerformanceTests PROPERTY OUTPUT_NAME "performance_tests")
set_property(TARGET SystemTests PROPERTY OUTPUT_NAME "system_tests")
set_property(TARGET DifficultyTests PROPERTY OUTPUT_NAME "difficulty_tests")
set_property(TARGET DynamicMoneySupplyTest PROPERTY OUTPUT_NAME "dynamic_money_supply_test")
set_property(TARGET DynamicSupplyIntegrationTest PROPERTY OUTPUT_NAME "dynamic_supply_integration_test")
set_property(TARGET DynamicSupplyPerformanceTest PROPERTY OUTPUT_NAME "dynamic_supply_performance_test")
