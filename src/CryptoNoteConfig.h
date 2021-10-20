// Copyright (c) 2019-2021 Fango Developers
// Copyright (c) 2018-2021 Fandom Gold Society
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2018 The Monero project
// Copyright (c) 2014-2018 The Forknote developers
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
// Copyright (c) 2018-2019 The Ryo Currency developers
//
// This file is part of Fango.
//
// Fango is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fango includes elements written 
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fango. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <initializer_list>

namespace CryptoNote
{
	namespace parameters
	{
		const uint64_t DIFFICULTY_TARGET = 480;	
		const uint64_t CRYPTONOTE_MAX_BLOCK_NUMBER = 500000000;
		const size_t CRYPTONOTE_MAX_BLOCK_BLOB_SIZE = 500000000;
		const size_t CRYPTONOTE_MAX_TX_SIZE = 1000000000;
		const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 1075740; /* TEST address prefix */
		const size_t CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW = 60;	
		const uint64_t DIFFICULTY_TARGET_DRGL = 81;
		const unsigned EMISSION_SPEED_FACTOR = 18;
		const unsigned EMISSION_SPEED_FACTOR_FANGO = 19;
		const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT  = 60 * 60 * 2;
		const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1 = DIFFICULTY_TARGET_DRGL * 6;
		const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V2 = DIFFICULTY_TARGET * 2;
		const uint64_t CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10;
		const size_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW = 60;
		const size_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1 = 11; /* LWMA3 */

		const uint64_t MONEY_SUPPLY = UINT64_C(80000088000008); /* max supply: 8M8 */
		const uint64_t COIN = UINT64_C(10000000);			  
		const uint64_t MINIMUM_FEE_V1 = UINT64_C(800000);
		const uint64_t MINIMUM_FEE_V2 = UINT64_C(80000);	/* 0.008 XFG */
		const uint64_t MINIMUM_FEE = MINIMUM_FEE_V2;		
		const uint64_t MINIMUM_FEE_BANKING = UINT64_C(80000);  /* 0.008 XFG */
		const uint64_t DEFAULT_DUST_THRESHOLD = UINT64_C(20000); /* < 0.002 XFG */
		const size_t   MINIMUM_MIXIN = 2;

		const size_t   CRYPTONOTE_COIN_VERSION                       = 1;
		const size_t   CRYPTONOTE_DISPLAY_DECIMAL_POINT 	     = 7;
		const size_t   CRYPTONOTE_REWARD_BLOCKS_WINDOW               = 100;
		const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE     = 800000; //size of block (bytes) after reward for block is calculated in block-size
		const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2  = 800000;
		const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1  = 20000;
		const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_CURRENT = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
		const size_t   CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE        = 600;

		const uint64_t EXPECTED_NUMBER_OF_BLOCKS_PER_DAY             = 24 * 60 * 60 / DIFFICULTY_TARGET;
		const size_t   DIFFICULTY_CUT                                = 60;  // v0
		const size_t   DIFFICULTY_LAG                                = 15;  // v0
		const size_t   DIFFICULTY_WINDOW                             = 1067; // blocks  Original CryptoNote
		const size_t   DIFFICULTY_WINDOW_V2                          = 18;  // blocks  Zawy v1.0
		const size_t   DIFFICULTY_WINDOW_V3                          = 60;  // blocks  Zawy-LWMA1
		const size_t   DIFFICULTY_WINDOW_V4                          = 45;  // blocks  Zawy-LWMA1 Fango

		const uint64_t MIN_TX_MIXIN_SIZE                             = 2;
		const uint64_t MAX_TX_MIXIN_SIZE                             = 18;
		static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Bad DIFFICULTY_WINDOW or DIFFICULTY_CUT");

		const uint64_t DEPOSIT_MIN_AMOUNT = 8 * COIN;
		const uint32_t DEPOSIT_MIN_TERM = 8;  //test term 		 /* one month=5480 ( 3 months (16440) for release ) OverviewFrame::depositParamsChanged */ 
		const uint32_t DEPOSIT_MAX_TERM = 1 * 12 * 80;  		 /* one year | use 3 month min/max */

		static_assert(DEPOSIT_MIN_TERM > 0, "Bad DEPOSIT_MIN_TERM");
		static_assert(DEPOSIT_MIN_TERM <= DEPOSIT_MAX_TERM, "Bad DEPOSIT_MAX_TERM");

		static constexpr uint64_t POISSON_CHECK_TRIGGER = 10; // Reorg size that triggers poisson timestamp check
		static constexpr uint64_t POISSON_CHECK_DEPTH = 60;   // Main-chain depth of poisson check. The attacker will have to tamper 50% of those blocks
		static constexpr double POISSON_LOG_P_REJECT = -75.0; // Reject reorg if probability of timestamps being genuine is less than e^x, -75 = 10^-33

		const size_t   MAX_BLOCK_SIZE_INITIAL                        = 800000;  
		const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR         = 100 * 1024;
		const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR       = 365 * 24 * 60 * 60 / DIFFICULTY_TARGET;

		const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS     = 1;
		const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS    = DIFFICULTY_TARGET_DRGL * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;
		const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2 = DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;		   

		const size_t CRYPTONOTE_MAX_TX_SIZE_LIMIT = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_CURRENT - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE; /* maximum transaction size */
		const size_t CRYPTONOTE_OPTIMIZE_SIZE=  100;		/* proportional to CRYPTONOTE_MAX_TX_SIZE_LIMIT */

		const uint64_t CRYPTONOTE_MEMPOOL_TX_LIVETIME = (60 * 60 * 12);					/* 1 hour in seconds */
		const uint64_t CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME = (60 * 60 * 12);	/* 24 hours in seconds */
		const uint64_t CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL = 7; /* CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL * CRYPTONOTE_MEMPOOL_TX_LIVETIME  = time to forget tx */

		const size_t FUSION_TX_MAX_SIZE = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 30 / 100;
		const size_t FUSION_TX_MIN_INPUT_COUNT = 12;
		const size_t FUSION_TX_MIN_IN_OUT_COUNT_RATIO = 4;

		const uint32_t UPGRADE_HEIGHT_V2                             = 2; //{Hardhome}
		const uint32_t UPGRADE_HEIGHT_V3                             = 3; //{Longclaw}
		const uint32_t UPGRADE_HEIGHT_V4                             = 4; //{Dracarys}
		const uint32_t UPGRADE_HEIGHT_V5                             = 5; //{Ironborn}  CN7  (variant1) 
		const uint32_t UPGRADE_HEIGHT_V6                             = 6; //{Ice&fire}  CN8  (variant2)
		const uint32_t UPGRADE_HEIGHT_V7                             = 7; //Fandomgold
		const uint32_t UPGRADE_HEIGHT_V8                             = 5439; //Dragonbourne (emission|deposits)
		const unsigned UPGRADE_VOTING_THRESHOLD = 90; // percent
		const size_t UPGRADE_VOTING_WINDOW = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;
		const size_t UPGRADE_WINDOW = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;

		static_assert(0 < UPGRADE_VOTING_THRESHOLD && UPGRADE_VOTING_THRESHOLD <= 100, "Bad UPGRADE_VOTING_THRESHOLD");
		static_assert(UPGRADE_VOTING_WINDOW > 1, "Bad UPGRADE_VOTING_WINDOW");

		const char CRYPTONOTE_BLOCKS_FILENAME[] = "testnetblocks.dat";
		const char CRYPTONOTE_BLOCKINDEXES_FILENAME[] = "testnetblockindexes.dat";
		const char CRYPTONOTE_BLOCKSCACHE_FILENAME[] = "testnetblockscache.dat";
		const char CRYPTONOTE_POOLDATA_FILENAME[] = "testnetpoolstate.bin";
		const char P2P_NET_DATA_FILENAME[] = "testnetp2pstate.bin";
		const char CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME[] = "testnetblockchainindices.dat";
		const char MINER_CONFIG_FILE_NAME[] = "testnetminer_conf.json";

	} // namespace parameters

	const char CRYPTONOTE_NAME[] = "XFG_TESTNET";
	const char GENESIS_COINBASE_TX_HEX[] = "013c01ff0001b4bcc29101029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd0880712101aeba4ab2c89f2fa87a73f28f8002bdb9a86309f0a2e7465e18cbf9c58625347d";

	const uint8_t  TRANSACTION_VERSION_1                         =  1;
	const uint8_t  TRANSACTION_VERSION_2                         =  2;
	const uint8_t  BLOCK_MAJOR_VERSION_1                         =  1;
	const uint8_t  BLOCK_MAJOR_VERSION_2                         =  2;
	const uint8_t  BLOCK_MAJOR_VERSION_3                         =  3;
	const uint8_t  BLOCK_MAJOR_VERSION_4                         =  4;
	const uint8_t  BLOCK_MAJOR_VERSION_5                         =  5;
	const uint8_t  BLOCK_MAJOR_VERSION_6                         =  6;
	const uint8_t  BLOCK_MAJOR_VERSION_7                         =  7;
	const uint8_t  BLOCK_MAJOR_VERSION_8                         =  8; 
	const uint8_t  BLOCK_MINOR_VERSION_0 			     =  0;
	const uint8_t  BLOCK_MINOR_VERSION_1 			     =  1;

	const size_t BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT = 10000; // by default, blocks ids count in synchronizing
	const size_t BLOCKS_SYNCHRONIZING_DEFAULT_COUNT = 128;		 // by default, blocks count in blocks downloading
	const size_t COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT = 1000;

	const int P2P_DEFAULT_PORT = 28282;
	const int RPC_DEFAULT_PORT = 38383;

	/* P2P Network Configuration Section - This defines our current P2P network version
	and the minimum version for communication between nodes */
	const uint8_t P2P_VERSION_1 = 1;
	const uint8_t P2P_VERSION_2 = 2;
	const uint8_t P2P_CURRENT_VERSION = 1;
	const uint8_t P2P_MINIMUM_VERSION = 1;
	const uint8_t P2P_UPGRADE_WINDOW = 2;

	// This defines the minimum P2P version required for lite blocks propogation
	const uint8_t P2P_LITE_BLOCKS_PROPOGATION_VERSION = 3;

	const size_t P2P_LOCAL_WHITE_PEERLIST_LIMIT = 1000;
	const size_t P2P_LOCAL_GRAY_PEERLIST_LIMIT = 5000;

	const size_t P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE = 64 * 1024 * 1024; // 64MB
	const uint32_t P2P_DEFAULT_CONNECTIONS_COUNT = 8;
	const size_t P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT = 2;
	const size_t P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT = 70; // percent
	const uint32_t P2P_DEFAULT_HANDSHAKE_INTERVAL = 60;			 // seconds
	const uint32_t P2P_DEFAULT_PACKET_MAX_SIZE = 50000000;		 // 50000000 bytes maximum packet size
	const uint32_t P2P_DEFAULT_PEERS_IN_HANDSHAKE = 250;
	const uint32_t P2P_DEFAULT_CONNECTION_TIMEOUT = 5000;	   // 5 seconds
	const uint32_t P2P_DEFAULT_PING_CONNECTION_TIMEOUT = 2000; // 2 seconds
	const uint64_t P2P_DEFAULT_INVOKE_TIMEOUT = 60 * 2 * 1000; // 2 minutes
	const size_t P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT = 5000;  // 5 seconds
        const uint32_t P2P_IP_BLOCKTIME         = (60 * 60 * 24);  // 24 hr
        const uint32_t P2P_IP_FAILS_BEFORE_BLOCK  =  45;
	const char P2P_STAT_TRUSTED_PUB_KEY[] = "";

	// Seed Nodes
	const std::initializer_list<const char *> SEED_NODES = {
		"104.236.0.16:28282",
		"188.226.177.187:28282",
		"fango.money:28282",
		"fangotango.hopto.org:28282"
	};

	struct CheckpointData
	{
		uint32_t height;
		const char *blockId;
	};

#ifdef __GNUC__
	__attribute__((unused))
#endif
	// Blockchain Checkpoints:
	// {<block height>, "<block hash>"},
	const std::initializer_list<CheckpointData>
		CHECKPOINTS = {	};

} // namespace CryptoNote

#define ALLOW_DEBUG_COMMANDS
//Knowledge has made you powerful but there is still so much you don't know.
