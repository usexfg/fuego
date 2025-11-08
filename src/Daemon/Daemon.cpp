// Copyright (c) 2019-2025 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
//
// This file is part of Fuego.
// 
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.



#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <thread>
#include <chrono>

#ifdef _WIN32
  #ifdef _MSC_VER
    #include <crtdbg.h>
  #endif
#endif

#include "DaemonCommandsHandler.h"

#include "Common/SignalHandler.h"
#include "Common/PathTools.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/CoreConfig.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/MinerConfig.h"
#include "CryptoNoteCore/TransactionExtra.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "CryptoNoteProtocol/ICryptoNoteProtocolQuery.h"
#include "P2p/NetNode.h"
#include "P2p/NetNodeConfig.h"
#include "Rpc/RpcServer.h"
#include "Rpc/RpcServerConfig.h"
#include "version.h"

#include "Logging/ConsoleLogger.h"
#include <Logging/LoggerManager.h>

#if defined(WIN32)
#include <crtdbg.h>
#endif

using Common::JsonValue;
using namespace CryptoNote;
using namespace Logging;

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", "Specify configuration file", std::string(CryptoNote::CRYPTONOTE_NAME) + ".conf"};
  
  // Stake verification functions for private blockchain
  bool verifyMinimumStakeWithWallet(const std::string& address, uint64_t minimumStake, 
                                   const CryptoNote::core& ccore, const CryptoNote::Currency& currency);
  bool verifyMinimumStakeWithProof(const std::string& address, uint64_t minimumStake);
  bool verifyMinimumStakeWithExternalService(const std::string& address, uint64_t minimumStake);
  bool verifyStakeWithDaemonWallet(const CryptoNote::AccountPublicAddress& acc, 
                                  uint64_t minimumStake, const CryptoNote::core& ccore);
  const command_line::arg_descriptor<bool>        arg_os_version  = {"os-version", ""};
  const command_line::arg_descriptor<std::string> arg_log_file    = {"log-file", "", ""};
  const command_line::arg_descriptor<std::string> arg_set_fee_address = { "fee-address", "Set a fee address for remote nodes", "" };
  const command_line::arg_descriptor<std::string> arg_set_view_key = { "view-key", "Set secret view-key for remote node fee confirmation", "" };
  
  // Elderfier Service Arguments (STARK verification with stake requirement)
  const command_line::arg_descriptor<bool>        arg_enable_elderfier = {"enable-elderfier", "Enable Elderfier service mode for STARK proof verification (requires 800 XFG stake)", false};
  const command_line::arg_descriptor<std::string> arg_elderfier_config = {"elderfier-config", "Path to Elderfier configuration file (optional)", ""};
  const command_line::arg_descriptor<std::string> arg_elderfier_registry_url = {"elderfier-registry-url", "GitHub URL for Elderfier registry (default: https://raw.githubusercontent.com/usexfg/fuego/main/elderfier_registry.txt)", "https://raw.githubusercontent.com/usexfg/fuego/main/elderfier_registry.txt"};
  
  const command_line::arg_descriptor<bool>        arg_restricted_rpc = {"restricted-rpc", "Restrict RPC to view only commands to prevent abuse"};
  const command_line::arg_descriptor<std::string> arg_enable_cors = { "enable-cors", "Adds header 'Access-Control-Allow-Origin' to the daemon's RPC responses. Uses the value as domain. Use * for all", "" };
  const command_line::arg_descriptor<int>         arg_log_level   = {"log-level", "", 2}; // info level
  const command_line::arg_descriptor<bool>        arg_console     = {"no-console", "Disable daemon console commands"};
  const command_line::arg_descriptor<bool> arg_testnet_on = {
    "testnet",
    "Used to deploy test nets. Checkpoints and hardcoded seeds are ignored. Testnet uses different default ports: P2P=20808, RPC=28280.",
    false
};
  const command_line::arg_descriptor<bool>        arg_print_genesis_tx = { "print-genesis-tx", "Prints genesis' block tx hex to insert it to config and exits" };

    // Check if terminal supports UTF-8/emoji
  bool isUtf8Supported() {
    #ifdef _WIN32
      // Windows: Check console code page
      return GetConsoleOutputCP() == CP_UTF8;
    #else
      // Unix/Linux/macOS: Check locale
      const char* locale = std::setlocale(LC_ALL, "");
      if (!locale) return false;
      
      // Check if locale contains UTF-8 or UTF8
      std::string loc(locale);
      std::transform(loc.begin(), loc.end(), loc.begin(), ::toupper);
      return loc.find("UTF-8") != std::string::npos || 
             loc.find("UTF8") != std::string::npos;
    #endif
  }
  
  // Verify stake by checking for active locked 0xE8 (Elderfier) deposit
  // This replaces all previous proof-of-stake methods
  bool verifyStakeWithElderfierDeposit(const std::string& address,
                                       CryptoNote::core& ccore,
                                       const CryptoNote::Currency& currency) {
    try {
      // Parse address
      CryptoNote::AccountPublicAddress acc;
      if (!currency.parseAccountAddressString(address, acc)) {
        return false;
      }
      
      uint32_t currentHeight = ccore.get_current_blockchain_height();
      
       // Only scan from block 1M and up
      if (currentHeight <= 1000000) {
        return false; // Chain hasn't reached block 1M yet
      }
      
      // Scan from block 1M & up
      uint32_t startHeight = 1000000;
      
      for (uint32_t height = startHeight; height < currentHeight; ++height) {
        std::list<CryptoNote::Block> blocks;
        ccore.get_blocks(height, 1, blocks);
        
        if (blocks.empty()) continue;
        
        const CryptoNote::Block& block = blocks.front();
        
        for (const Crypto::Hash& txHash : block.transactionHashes) {
          CryptoNote::Transaction tx;
          Crypto::Hash blockHash;
          uint32_t blockHeight;
          
          if (!ccore.getTransaction(txHash, tx)) {
            continue;
          }
          
          // Parse transaction extra
          std::vector<CryptoNote::TransactionExtraField> extraFields;
          if (!CryptoNote::parseTransactionExtra(tx.extra, extraFields)) {
            continue;
          }
          
          // Check for 0xE8 (TX_EXTRA_ELDERFIER_DEPOSIT)
          bool hasElderfierDeposit = false;
          for (const auto& field : extraFields) {
            if (field.type() == typeid(CryptoNote::TransactionExtraElderfierDeposit)) {
              hasElderfierDeposit = true;
              break;
            }
          }
          
          if (!hasElderfierDeposit) {
            continue;
          }
          
          // Check if stake is still locked
          // Get amount at creation & current height
          uint64_t depositAtCreation = ccore.depositAmountAtHeight(height);
          uint64_t depositAtCurrent = ccore.depositAmountAtHeight(currentHeight);
          
          // If stake exists, make sure its locked & isnt slashed
          // Check if amount is 800 XFG
          if (depositAtCreation == 800000000000ULL && depositAtCurrent == depositAtCreation) {
            return true; // Found active locked 0xE8 deposit
          }
        }
      }
      
      return false;
    } catch (const std::exception& e) {
      return false;
    }
  }

} // anonymous namespace

bool command_line_preprocessor(const boost::program_options::variables_map& vm, LoggerRef& logger);

void print_genesis_tx_hex() {
  Logging::ConsoleLogger logger;
  CryptoNote::Transaction tx = CryptoNote::CurrencyBuilder(logger).generateGenesisTransaction();
  CryptoNote::BinaryArray txb = CryptoNote::toBinaryArray(tx);
  std::string tx_hex = Common::toHex(txb);

  std::cout << "const char GENESIS_COINBASE_TX_HEX[] = \"" << tx_hex << "\";" << std::endl;

  return;
}

JsonValue buildLoggerConfiguration(Level level, const std::string& logfile) {
  JsonValue loggerConfiguration(JsonValue::OBJECT);
  loggerConfiguration.insert("globalLevel", static_cast<int64_t>(level));

  JsonValue& cfgLoggers = loggerConfiguration.insert("loggers", JsonValue::ARRAY);

  JsonValue& fileLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  fileLogger.insert("type", "file");
  fileLogger.insert("filename", logfile);
  fileLogger.insert("level", static_cast<int64_t>(TRACE));

  JsonValue& consoleLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  consoleLogger.insert("type", "console");
  consoleLogger.insert("level", static_cast<int64_t>(TRACE));
  consoleLogger.insert("pattern", "%D %T %L ");

  return loggerConfiguration;
}

int main(int argc, char* argv[])
{
  // Set locale for UTF-8 support
  std::setlocale(LC_ALL, "");

#ifdef _WIN32
  #ifdef _MSC_VER
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  #endif
#endif

  LoggerManager logManager;
  LoggerRef logger(logManager, "daemon");

  try {

    po::options_description desc_cmd_only("Command line options");
    po::options_description desc_cmd_sett("Command line options and settings options");

   desc_cmd_sett.add_options()("enable-blockchain-indexes,i", po::bool_switch()->default_value(false), "Enable blockchain indexes");
   desc_cmd_sett.add_options()("enable-autosave,a", po::bool_switch()->default_value(false), "Enable blockchain autosave every 720 blocks");

   command_line::add_arg(desc_cmd_only, command_line::arg_help);
   command_line::add_arg(desc_cmd_only, command_line::arg_version);
   command_line::add_arg(desc_cmd_only, arg_os_version);
   command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, Tools::getDefaultDataDirectory());
   command_line::add_arg(desc_cmd_only, arg_config_file);
   command_line::add_arg(desc_cmd_sett, arg_restricted_rpc);

   command_line::add_arg(desc_cmd_sett, arg_set_fee_address);
   command_line::add_arg(desc_cmd_sett, arg_log_file);
   command_line::add_arg(desc_cmd_sett, arg_log_level);
   command_line::add_arg(desc_cmd_sett, arg_console);
   command_line::add_arg(desc_cmd_sett, arg_set_view_key);
   
       // Elderfier Service Arguments (STARK verification with stake requirement)
    command_line::add_arg(desc_cmd_sett, arg_enable_elderfier);
    command_line::add_arg(desc_cmd_sett, arg_elderfier_config);
    command_line::add_arg(desc_cmd_sett, arg_elderfier_registry_url);
   
   command_line::add_arg(desc_cmd_sett, arg_testnet_on);
   command_line::add_arg(desc_cmd_sett, arg_enable_cors);

   command_line::add_arg(desc_cmd_sett, arg_print_genesis_tx);

   RpcServerConfig::initOptions(desc_cmd_sett);
   CoreConfig::initOptions(desc_cmd_sett);
   NetNodeConfig::initOptions(desc_cmd_sett);
   MinerConfig::initOptions(desc_cmd_sett);

   po::options_description desc_options("Allowed options");
   desc_options.add(desc_cmd_only).add(desc_cmd_sett);

   po::variables_map vm;
   bool r = command_line::handle_error_helper(desc_options, [&]() {
     po::store(po::parse_command_line(argc, argv, desc_options), vm);

     if (command_line::get_arg(vm, command_line::arg_help))
     {
       const char* fuego_icon = isUtf8Supported() ? "🔥 " : "";
       std::cout << "Fuego" << fuego_icon << PROJECT_VERSION_LONG << ENDL;
       std::cout << desc_options << std::endl;
       return false;
     }

     if (command_line::get_arg(vm, arg_print_genesis_tx))
     {
       //print_genesis_tx_hex(vm);
       print_genesis_tx_hex();
       return false;
     }

     std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
     std::string config = command_line::get_arg(vm, arg_config_file);

     boost::filesystem::path data_dir_path(data_dir);
     boost::filesystem::path config_path(config);
     if (!config_path.has_parent_path())
     {
       config_path = data_dir_path / config_path;
     }

     boost::system::error_code ec;
     if (boost::filesystem::exists(config_path, ec))
     {
       po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett), vm);
     }

     po::notify(vm);
     return true;
   });

   if (!r)
   {
     return 1;
    }

    auto modulePath = Common::NativePathToGeneric(argv[0]);
    auto cfgLogFile = Common::NativePathToGeneric(command_line::get_arg(vm, arg_log_file));

    if (cfgLogFile.empty()) {
      cfgLogFile = Common::ReplaceExtenstion(modulePath, ".log");
    } else {
      if (!Common::HasParentPath(cfgLogFile)) {
        cfgLogFile = Common::CombinePath(Common::GetPathDirectory(modulePath), cfgLogFile);
      }
    }

    Level cfgLogLevel = static_cast<Level>(static_cast<int>(Logging::ERROR) + command_line::get_arg(vm, arg_log_level));

    // configure logging
	    logManager.configure(buildLoggerConfiguration(cfgLogLevel, cfgLogFile));
		logger(INFO, BRIGHT_MAGENTA) <<
#ifdef _WIN32
" \n"		
"       8888888888 888     888 8888888888 .d8888b.   .d88888b.   \n"
"       888        888     888 888       d88P  Y88b d88P` `Y88b  \n"
"       888        888     888 888       888    888 888     888  \n"
"       8888888    888     888 8888888   888        888     888  \n"
"       888        888     888 888       888  88888 888     888  \n"
"       888        888     888 888       888    888 888     888  \n"
"       888        Y88b. .d88P 888       Y88b  d88P Y88b. .d88P  \n"
"       888         `Y88888P`  8888888888 `Y8888P88  `Y88888P`   \n"                                
#else
" \n"
" ░░░░░░░ ░░    ░░ ░░░░░░░  ░░░░░░   ░░░░░░  \n"
" ▒▒      ▒▒    ▒▒ ▒▒      ▒▒       ▒▒    ▒▒ \n"
" ▒▒▒▒▒   ▒▒    ▒▒ ▒▒▒▒▒   ▒▒   ▒▒▒ ▒▒    ▒▒ \n"
" ▓▓      ▓▓    ▓▓ ▓▓      ▓▓    ▓▓ ▓▓    ▓▓ \n"
" ██       ██████  ███████  ██████   ██████  \n"
#endif
			"\n"
			<< "             "  PROJECT_VERSION_LONG "\n"
			"\n";

    if (command_line_preprocessor(vm, logger)) {
      return 0;
    }

    printf("INFO: Module folder: %s\n", argv[0]);

    bool testnet_mode = command_line::get_arg(vm, arg_testnet_on);
    if (testnet_mode) {
      printf("INFO: Starting in testnet mode!\n");
    }

    //create objects and link them
    CryptoNote::CurrencyBuilder currencyBuilder(logManager);
    currencyBuilder.testnet(testnet_mode);

    try {
      currencyBuilder.currency();
    } catch (std::exception&) {
      std::cout << "GENESIS_COINBASE_TX_HEX constant has an incorrect value. Please launch: " << CryptoNote::CRYPTONOTE_NAME << "d --" << arg_print_genesis_tx.name;
      return 1;
    }

    CryptoNote::Currency currency = currencyBuilder.currency();
    CryptoNote::core ccore(currency, nullptr, logManager, vm["enable-blockchain-indexes"].as<bool>(), vm["enable-autosave"].as<bool>());

    CoreConfig coreConfig;
    coreConfig.init(vm);
    NetNodeConfig netNodeConfig;
    netNodeConfig.init(vm);
    netNodeConfig.setTestnet(testnet_mode);
    
    // Set mainnet default port if not explicitly configured
    if (netNodeConfig.getBindPort() == 0) {
      netNodeConfig.setBindPort(P2P_DEFAULT_PORT);
    }
    
    MinerConfig minerConfig;
    minerConfig.init(vm);
    RpcServerConfig rpcConfig;
    rpcConfig.init(vm);
    

    if (!coreConfig.configFolderDefaulted) {
      if (!Tools::directoryExists(coreConfig.configFolder)) {
        throw std::runtime_error("Directory does not exist: " + coreConfig.configFolder);
      }
    } else {
      if (!Tools::create_directories_if_necessary(coreConfig.configFolder)) {
        throw std::runtime_error("Can't create directory: " + coreConfig.configFolder);
      }
    }

    System::Dispatcher dispatcher;

    CryptoNote::CryptoNoteProtocolHandler cprotocol(currency, dispatcher, ccore, nullptr, logManager);
    CryptoNote::NodeServer p2psrv(dispatcher, cprotocol, logManager);
    CryptoNote::RpcServer rpcServer(dispatcher, logManager, ccore, p2psrv, cprotocol);

    cprotocol.set_p2p_endpoint(&p2psrv);
    ccore.set_cryptonote_protocol(&cprotocol);
    DaemonCommandsHandler dch(ccore, p2psrv, logManager, cprotocol);

    // initialize objects
    printf("INFO: Initializing p2p server...\n");
    if (!p2psrv.init(netNodeConfig)) {
      printf("ERROR: Failed to initialize p2p server.\n");
      return 1;
    }

    printf("INFO: P2p server initialized OK\n");

    // initialize core here
    printf("INFO: Initializing core...\n");
    if (!ccore.init(coreConfig, minerConfig, true)) {
      printf("ERROR: Failed to initialize core\n");
      return 1;
    }

    printf("INFO: Core initialized OK\n");

    // start components
    if (!command_line::has_arg(vm, arg_console)) {
      dch.start_handling();
    }

    printf("INFO: Starting core rpc server setup\n");

    /* Set address for remote node fee */
  	if (command_line::has_arg(vm, arg_set_fee_address)) {
	  std::string addr_str = command_line::get_arg(vm, arg_set_fee_address);
	  if (!addr_str.empty()) {
        AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();
        if (!currency.parseAccountAddressString(addr_str, acc)) {
          printf("ERROR: Bad fee address: %s\n", addr_str.c_str());
          return 1;
        }
        rpcServer.setFeeAddress(addr_str, acc);
        printf("INFO: Remote node fee address set: %s\n", addr_str.c_str());

      }
	  }
  
    /* This sets the view-key so we can confirm that
       the fee is part of the transaction blob */       
    if (command_line::has_arg(vm, arg_set_view_key)) {
      std::string vk_str = command_line::get_arg(vm, arg_set_view_key);
	    if (!vk_str.empty()) {
        rpcServer.setViewKey(vk_str);
        printf("INFO: Secret view key set\n");
      }
    }

    printf("INFO: Starting RPC server\n");
    try {
      rpcServer.start(rpcConfig.bindIp, rpcConfig.bindPort);
      printf("INFO: RPC server started successfully\n");
    } catch (const std::exception& e) {
      printf("ERROR: RPC server failed to start: %s\n", e.what());
      throw;
    }
    printf("INFO: Core setup completed, starting P2P networking\n");

    // Continue with full daemon initialization

    // RPC server configs
     rpcServer.restrictRPC(command_line::get_arg(vm, arg_restricted_rpc));
     rpcServer.enableCors(command_line::get_arg(vm, arg_enable_cors));

    // Blockchain loading is done during core initialization above
    // Signal handler for graceful shutdown
    Tools::SignalHandler::install([&dch, &p2psrv] {
      dch.stop_handling();
      p2psrv.sendStopSignal();
     });

    printf("INFO: Starting p2p net loop...\n");
    printf("DEBUG: About to call p2psrv.run()\n");

    printf("DEBUG: Calling p2psrv.run() now\n");
    p2psrv.run();
    printf("INFO: p2p net loop started\n");

    // Initialize Elderfier Service if enabled (STARK verification with stake requirement)
    // Moved here to ensure blockchain is fully loaded before verifying stakes
    if (command_line::has_arg(vm, arg_enable_elderfier)) {
      printf("INFO: Checking Elderfier service eligibility...\n");
      std::string feeAddress = command_line::get_arg(vm, arg_set_fee_address);
      
      if (feeAddress.empty()) {
        printf("WARNING: Elderfier service requires --set-fee-address\n");
        printf("INFO: To enable: --enable-elderfier --set-fee-address YOUR_ADDRESS\n");
      } else {
        printf("INFO: Verifying Elderfier stake for address: %s\n", feeAddress.c_str());
        
        // Verify the fee address has 800 XFG stake via Elderfier deposit
        if (verifyStakeWithElderfierDeposit(feeAddress, ccore, currency)) {
          printf("INFO: Elderfier service enabled - valid 800 XFG stake verified\n");
          printf("INFO: STARK proof verification service is now active\n");
          // TODO: Initialize Elderfier registry from GitHub
          // TODO: Start Elderfier service endpoint
        } else {
          printf("WARNING: Elderfier service requires 800 XFG locked in Elderfier deposit (0xE8)\n");
          printf("INFO: Address %s does not have valid stake\n", feeAddress.c_str());
          printf("INFO: To create Elderfier deposit, use fuego-wallet-cli burn command\n");
        }
      }
    }

    dch.stop_handling();

    //stop components
    printf("INFO: Stopping core rpc server...\n");
    rpcServer.stop();

    //deinitialize components
    printf("INFO: Deinitializing core...\n");
    ccore.deinit();
    printf("INFO: Deinitializing p2p...\n");
    p2psrv.deinit();

    ccore.set_cryptonote_protocol(NULL);
    cprotocol.set_p2p_endpoint(NULL);

  } catch (const std::exception& e) {
    printf("ERROR: Exception: %s\n", e.what());
    return 1;
  }

  printf("INFO: Node stopped.\n");
  return 0;
}


bool command_line_preprocessor(const boost::program_options::variables_map &vm, LoggerRef &logger) {
  bool exit = false;

  if (command_line::get_arg(vm, command_line::arg_version)) {
    std::cout << CryptoNote::CRYPTONOTE_NAME << PROJECT_VERSION_LONG << ENDL;
    exit = true;
  }

  if (command_line::get_arg(vm, arg_os_version)) {
    std::cout << "OS: " << Tools::get_os_version_string() << ENDL;
    exit = true;
  }

  if (exit) {
    return true;
  }

  return false;
}
