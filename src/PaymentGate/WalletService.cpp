#include "WalletService.h"
#include <System/EventLock.h>
#include "Wallet/WalletErrors.h"
#include <iomanip>
#include <CryptoNoteCore/CryptoNoteTools.h>
#include <Common/Util.h>

std::error_code PaymentService::WalletService::getBaseMoneySupply(uint64_t &baseMoneySupply) {
  try {
    System::EventLock lk(readyEvent);
    baseMoneySupply = currency.moneySupply();
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getBaseMoneySupply error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getCirculatingSupply(uint64_t &circulatingSupply) {
  try {
    System::EventLock lk(readyEvent);
    circulatingSupply = currency.moneySupply();
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getCirculatingSupply error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getBurnPercentage(double &burnPercentage) {
  try {
    System::EventLock lk(readyEvent);
    burnPercentage = 0.1;  // 10% placeholder burn rate
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getBurnPercentage error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getEternalFlame(uint64_t &ethernalXFG) {
  try {
    System::EventLock lk(readyEvent);
    ethernalXFG = 0;  // Placeholder for eternal flame tracking
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getEternalFlame error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::string PaymentService::WalletService::formatXFG(uint64_t heatAmount) {
  try {
    // XFG format for circulating supply (7 decimals): 1 XFG = 10,000,000 HEAT
    double xfgAmount = static_cast<double>(heatAmount) / 10000000.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(7) << xfgAmount << " XFG";
    return ss.str();
  } catch (...) {
    return "0.0000000 XFG";
  }
}

std::string PaymentService::WalletService::formatHEAT(uint64_t heatAmount) {
  try {
    // HEAT format for ERC20 token only (raw atomic units)
    std::stringstream ss;
    ss << heatAmount << " HEAT";
    return ss.str();
  } catch (...) {
    return "0 HEAT";
  }
}