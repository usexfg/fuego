#include "WalletService.h"
#include <System/EventLock.h>
#include "Wallet/WalletErrors.h"
#include <iomanip>
#include <CryptoNoteCore/CryptoNoteTools.h>
#include <Common/Util.h>

namespace PaymentService {

// Response structs for RPC methods
struct GetTotalDepositAmount {
  struct Response {
    std::string totalDepositAmount;
    std::string status;
  };
};

struct GetCirculatingSupply {
  struct Response {
    std::string circulatingSupply;
    std::string status;
  };
};

struct GetEthernalXFG {
  struct Response {
    std::string ethernalXFG;
    std::string status;
  };
};

} // namespace PaymentService

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
  try {
    System::EventLock lk(readyEvent);
    // Base money supply from currency (total emission curve)
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
    uint64_t totalSupply = currency.moneySupply();
    uint64_t burned = 0;
    circulatingSupply = totalSupply - burned;
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getCirculatingSupply error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}
  try {
    System::EventLock lk(readyEvent);

    uint64_t totalSupply = currency.moneySupply();
    uint64_t burned = 0;

    // Calculate circulating supply: total supply minus forever (burn) deposits
    // Note: In production, query actual blockchain state via node
    circulatingSupply = totalSupply - burned;

    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getCirculatingSupply error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getBurnPercentage(double &burnPercentage) {
  try {
    System::EventLock lk(readyEvent);
    uint64_t totalSupply = currency.moneySupply();
    uint64_t burned = 0;
    if (totalSupply == 0) {
      burnPercentage = 0.0;
    } else {
      burnPercentage = static_cast<double>(burned * 100.0) / totalSupply;
    }
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getBurnPercentage error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}
  try {
    System::EventLock lk(readyEvent);

    uint64_t totalSupply = currency.moneySupply();
    uint64_t burned = 0; // Query burn deposits from blockchain

    if (totalSupply == 0) {
      burnPercentage = 0.0;
    } else {
      burnPercentage = static_cast<double>(burned * 100.0) / totalSupply;
    }

    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getBurnPercentage error: " << e.what();
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getTotalDepositAmount(PaymentService::GetTotalDepositAmount::Response &response) {
  try {
    System::EventLock lk(readyEvent);

    uint64_t totalDeposits = 0;
    // Sum all deposits from wallet (replace with actual deposit tracking)

    response.totalDepositAmount = formatXFG(totalDeposits);
    response.status = "success";
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getTotalDepositAmount error: " << e.what();
    response.status = "error";
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getCirculatingSupply(PaymentService::GetCirculatingSupply::Response &response) {
  try {
    uint64_t supply = 0;
    auto ec = getCirculatingSupply(supply);
    if (!ec) {
      response.circulatingSupply = formatXFG(supply);
      response.status = "success";
    } else {
      response.status = "error";
    }
    return ec;
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getCirculatingSupply RPC error: " << e.what();
    response.status = "error";
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

std::error_code PaymentService::WalletService::getEternalFlame(PaymentService::GetEthernalXFG::Response &response) {
  try {
    System::EventLock lk(readyEvent);

    uint64_t eternalXFG = 0;
    // EternalFlame = accumulated burn rewards (OSAVVARIK trickle-down)
    // Query burn deposit interest from blockchain state

    response.ethernalXFG = formatXFG(eternalXFG);
    response.status = "success";
    return std::error_code();
  } catch (std::exception &e) {
    logger(Logging::WARNING) << "getEternalFlame error: " << e.what();
    response.status = "error";
    return make_error_code(CryptoNote::error::INTERNAL_WALLET_ERROR);
  }
}

// Placeholder formatAmount helper (replace with actual implementation)
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
