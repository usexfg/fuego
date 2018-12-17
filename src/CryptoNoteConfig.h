// {DRGL} Kills White Walkers
//
// 2018 {DRÆGONGLASS}
// <https://www.ZirtysPerzys.org>
// Copyright (c) 2012-2016, The CryptoNote developers, 
// Copyright (c) 2012-2016, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2018-2018, The Ryo Currency developers
// Copyright (c) 2017-2018, The Dragonglass developers

// This file is part of Bytecoin.
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>

namespace CryptoNote {
namespace parameters {


const uint64_t DIFFICULTY_TARGET                             = 81;//sec
const uint64_t CRYPTONOTE_MAX_BLOCK_NUMBER                   = 500000000;
const size_t   CRYPTONOTE_MAX_BLOCK_BLOB_SIZE                = 500000000;
const size_t   CRYPTONOTE_MAX_TX_SIZE                        = 1000000000;
const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX       = 679385; // addresses start with "dRGL"
const size_t   CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW          = 60;
  
const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT            = 60 * 60 * 2;
const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1         = DIFFICULTY_TARGET * 6;
const size_t   BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             = 60; 
const size_t   BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1          = 11;


const uint64_t MONEY_SUPPLY                                  = UINT64_C(80000088000008);
const size_t   CRYPTONOTE_DISPLAY_DECIMAL_POINT              = 7;
const size_t   CRYPTONOTE_COIN_VERSION                       = 1;
const unsigned EMISSION_SPEED_FACTOR                         = 18;
static_assert(EMISSION_SPEED_FACTOR <= 8 * sizeof(uint64_t), "Bad EMISSION_SPEED_FACTOR");

const size_t   CRYPTONOTE_REWARD_BLOCKS_WINDOW               = 100;
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE     = 800000; //size of block (bytes) after which reward for block calculated using block size
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2  = 800000;
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1  = 20000;
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_CURRENT = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
const size_t   CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE        = 600;

const uint64_t EXPECTED_NUMBER_OF_BLOCKS_PER_DAY             = 24 * 60 * 60 / DIFFICULTY_TARGET;
const size_t   DIFFICULTY_WINDOW                             = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY; // blocks
const size_t   DIFFICULTY_WINDOW_V2                          = 18;  // blocks
const size_t   DIFFICULTY_WINDOW_V3                          = 60; // blocks

const uint64_t MINIMUM_FEE                                   = UINT64_C(800000);
const uint64_t DEFAULT_DUST_THRESHOLD                        = UINT64_C(8000);
const uint64_t MAX_TX_MIXIN_SIZE                             = 18;

const size_t   DIFFICULTY_CUT                                = 60;  // timestamps to cut after sorting
const size_t   DIFFICULTY_LAG                                = 15;  // !!!
static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Bad DIFFICULTY_WINDOW or DIFFICULTY_CUT");

static constexpr uint64_t POISSON_CHECK_TRIGGER = 10; // Reorg size that triggers poisson timestamp check
static constexpr uint64_t POISSON_CHECK_DEPTH = 60;   // Main-chain depth of the poisson check. The attacker will have to tamper 50% of those blocks
static constexpr double POISSON_LOG_P_REJECT = -75.0; // Reject reorg if the probablity that the timestamps are genuine is below e^x, -75 = 10^-33

const size_t   MAX_BLOCK_SIZE_INITIAL                        = 800000;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR         = 100 * 1024;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR       = 365 * 24 * 60 * 60 / DIFFICULTY_TARGET;

const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS     = 1;
const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS    = DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;

const uint64_t CRYPTONOTE_MEMPOOL_TX_LIVETIME                = 60 * 60 * 24;     //seconds, one day
const uint64_t CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME = 60 * 60 * 24 * 7; //seconds, one week
const uint64_t CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL = 7;  // CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL * CRYPTONOTE_MEMPOOL_TX_LIVETIME = time to forget tx

const size_t   FUSION_TX_MAX_SIZE                            = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 * 30 / 100;
const size_t   FUSION_TX_MIN_INPUT_COUNT                     = 12;
const size_t   FUSION_TX_MIN_IN_OUT_COUNT_RATIO              = 4;
  
const uint32_t UPGRADE_HEIGHT_V2                             = 147958; //{Hardhome}
const uint32_t UPGRADE_HEIGHT_V3                             = 154321; //{Longclaw}
const uint32_t UPGRADE_HEIGHT_V4                             = 300000; //{Dracarys}
const uint32_t UPGRADE_HEIGHT_V5                             = 324819; //{Ironborn} CNv7
  
const unsigned UPGRADE_VOTING_THRESHOLD                      = 90; // percent
const uint32_t UPGRADE_VOTING_WINDOW                         = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;  // blocks
const uint32_t UPGRADE_WINDOW                                = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;  // blocks
static_assert(0 < UPGRADE_VOTING_THRESHOLD && UPGRADE_VOTING_THRESHOLD <= 100, "Bad UPGRADE_VOTING_THRESHOLD");
static_assert(UPGRADE_VOTING_WINDOW > 1, "Bad UPGRADE_VOTING_WINDOW");

const char     CRYPTONOTE_BLOCKS_FILENAME[]                  = "blocks.dat";
const char     CRYPTONOTE_BLOCKINDEXES_FILENAME[]            = "blockindexes.dat";
const char     CRYPTONOTE_BLOCKSCACHE_FILENAME[]             = "blockscache.dat";
const char     CRYPTONOTE_POOLDATA_FILENAME[]                = "poolstate.bin";
const char     P2P_NET_DATA_FILENAME[]                       = "p2pstate.bin";
const char     CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME[]      = "blockchainindices.dat";
const char     MINER_CONFIG_FILE_NAME[]                      = "miner_conf.json";
} // parameters

const char     CRYPTONOTE_NAME[]                             = "dragonglass";
const char     GENESIS_COINBASE_TX_HEX[]                     = "013c01ff0001b4bcc29101029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd0880712101bd4e0bf284c04d004fd016a21405046e8267ef81328cabf3017c4c24b273b25a";

const uint8_t  CURRENT_TRANSACTION_VERSION                   =  1;
const uint8_t  BLOCK_MAJOR_VERSION_1                         =  1;
const uint8_t  BLOCK_MAJOR_VERSION_2                         =  2;
const uint8_t  BLOCK_MAJOR_VERSION_3                         =  3;
const uint8_t  BLOCK_MAJOR_VERSION_4                         =  4;
const uint8_t  BLOCK_MAJOR_VERSION_5                         =  5;
const uint8_t  BLOCK_MINOR_VERSION_0                         =  0;
const uint8_t  BLOCK_MINOR_VERSION_1                         =  1;

const size_t   BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT        =  10000;  //by default, blocks ids count in synchronizing
const size_t   BLOCKS_SYNCHRONIZING_DEFAULT_COUNT            =  128;    //by default, blocks count in blocks downloading
const size_t   COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT         =  1000;

const int      P2P_DEFAULT_PORT                              =  10818;
const int      RPC_DEFAULT_PORT                              =  18180;

const size_t   P2P_LOCAL_WHITE_PEERLIST_LIMIT                =  1000;
const size_t   P2P_LOCAL_GRAY_PEERLIST_LIMIT                 =  5000;

const size_t   P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE          = 64 * 1024 * 1024; // 64 MB
const uint32_t P2P_DEFAULT_CONNECTIONS_COUNT                 = 8;
const size_t   P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT     = 70;
const uint32_t P2P_DEFAULT_HANDSHAKE_INTERVAL                = 60;            // seconds
const uint32_t P2P_DEFAULT_PACKET_MAX_SIZE                   = 50000000;      // 50000000 bytes maximum packet size
const uint32_t P2P_DEFAULT_PEERS_IN_HANDSHAKE                = 250;
const uint32_t P2P_DEFAULT_CONNECTION_TIMEOUT                = 5000;          // 5 seconds
const uint32_t P2P_DEFAULT_PING_CONNECTION_TIMEOUT           = 2000;          // 2 seconds
const uint64_t P2P_DEFAULT_INVOKE_TIMEOUT                    = 60 * 2 * 1000; // 2 minutes
const size_t   P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT          = 5000;          // 5 seconds
const uint32_t P2P_FAILED_ADDR_FORGET_SECONDS                = (60 * 60);     // 1 hour
const uint32_t P2P_IP_BLOCKTIME                              = (60 * 60 * 24);// 24 hour
const uint32_t P2P_IP_FAILS_BEFORE_BLOCK                     =  90;
const uint32_t P2P_IDLE_CONNECTION_KILL_INTERVAL             = (30 * 60);     // 30 minutes
const char     P2P_STAT_TRUSTED_PUB_KEY[]                    = "";

const char* const SEED_NODES[] = {
"ice.zirtysperzys.info:10818",
"fire.zirtysperzys.online:10818",
"mine.drgl.online:10818"
};

struct CheckpointData {
  uint32_t height;
  const char* blockId;
};

const std::initializer_list<CheckpointData> CHECKPOINTS = {
{ 800,    "c1c64f752f6f5f6f69671b3794f741af0707c71b35302ea4fc96b0befdce8ce9" },
{ 8008,   "299702f163995cd790b5c45362c78ad596f8717d749ff9016ce27eaa625b8a5e" },
{ 18008,  "46baf8aea2b9472a9f127ad7cdcb01a871ecf20d710e9e0d3a2b13176a452112" },
{ 63312,  "57c815dd1480b6a1de7037f85aa510ff7c784b91808f3777451c030d40614ddb" },
{ 80008,  "19e65aec81a283e756c9b55a884927bcbffa4639c9fe21fd4894ef211e0e8472" },
{ 108801, "0cb48287678f9df42a63c6c344f448ddce5316f9c5c03548e77d9a1193ebf5fd" },
{ 147959, "cecc0692782cd1956fb12bf170c4ebd6c7b6bb5c12e7071ef2d98e7c940f1961" },
{ 148000, "bd318f33b5f1804bc648ce847d4214cff8cfd7498483461db660a87e342eb0e9" },
{ 154322, "73232b04d18cdc9cc6430194298166c6e775a55ff0f48e2f819f8ed5fd873df7" },
{ 155433, "89be8af3d0a62454e95cf71cf7c17df9480ac337b4b5a294e0d75400b8989700" },
{ 158000, "153b22f4912d1a6db9f235de40ae2be3a178eb44cbde8e2a4fe0c7727037ab34" },
{ 180018, "3c0c6fd2f6c2805280f2079f50f772433957fae495ad81e305835bdb935fd21e" },
{ 200000, "4c4555f73e54b43f62fe26950d3c7f877e35c448a1e865b5ea07aa09d971e0e5" },
{ 222222, "801d187ca11851d0379c0fa4a790d26aa24e76835d26bf7e54f4b858bfd7ad53" },
{ 250000, "1a2cfc1c53a62038468feff7f22a150a95ba65090842d09fadd97f789e1e00fc" },
{ 260000, "968fc54cd727b5d70c4ccc1f9fe144c58bd909acc97cd27c491c4f6fc1b97087" },
{ 280000, "fa6016236d07c8a5ab660f5ddd788f2f002bd518146e2bc379dd66d1bc7f94a8" },
{ 300001, "ba7e401c03a9f5b2111ef402d8715761990ff53e31069c413f5c78c7cd819de9" },
{ 320000, "2c42f527960ce443ffa645b0af85d85bdf10cf9df8625d900b4edd0b29b68735" }
};

}

#define ALLOW_DEBUG_COMMANDS
//Knowledge has made you powerful but there is still so much you don't know.
