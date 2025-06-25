// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
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

#include "Currency.h"
#include <cctype>
#include <boost/algorithm/string/trim.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/lexical_cast.hpp>
#include "../Common/Base58.h"
#include "../Common/int-util.h"
#include "../Common/StringTools.h"

#include "CryptoNoteConfig.h"
#include "Account.h"
#include "CryptoNoteBasicImpl.h"
#include "CryptoNoteFormatUtils.h"
#include "CryptoNoteTools.h"
#include "TransactionExtra.h"
#include "UpgradeDetector.h"

#undef ERROR

using namespace Logging;
using namespace Common;

namespace CryptoNote
{

  const std::vector<uint64_t> Currency::PRETTY_AMOUNTS = {
      1, 2, 3, 4, 5, 6, 7, 8, 9,
      10, 20, 30, 40, 50, 60, 70, 80, 90,
      100, 200, 300, 400, 500, 600, 700, 800, 900,
      1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
      10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
      100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
      1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
      10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000,
      100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000,
      1000000000, 2000000000, 3000000000, 4000000000, 5000000000, 6000000000, 7000000000, 8000000000, 9000000000,
      10000000000, 20000000000, 30000000000, 40000000000, 50000000000, 60000000000, 70000000000, 80000000000, 90000000000,
      100000000000, 200000000000, 300000000000, 400000000000, 500000000000, 600000000000, 700000000000, 800000000000, 900000000000,
      1000000000000, 2000000000000, 3000000000000, 4000000000000, 5000000000000, 6000000000000, 7000000000000, 8000000000000, 9000000000000,
      10000000000000, 20000000000000, 30000000000000, 40000000000000, 50000000000000, 60000000000000, 70000000000000, 80000000000000, 90000000000000,
      100000000000000, 200000000000000, 300000000000000, 400000000000000, 500000000000000, 600000000000000, 700000000000000, 800000000000000, 900000000000000,
      1000000000000000, 2000000000000000, 3000000000000000, 4000000000000000, 5000000000000000, 6000000000000000, 7000000000000000, 8000000000000000, 9000000000000000,
      10000000000000000, 20000000000000000, 30000000000000000, 40000000000000000, 50000000000000000, 60000000000000000, 70000000000000000, 80000000000000000, 90000000000000000,
      100000000000000000, 200000000000000000, 300000000000000000, 400000000000000000, 500000000000000000, 600000000000000000, 700000000000000000, 800000000000000000, 900000000000000000,
      1000000000000000000, 2000000000000000000, 3000000000000000000, 4000000000000000000, 5000000000000000000, 6000000000000000000, 7000000000000000000, 8000000000000000000, 9000000000000000000,
      10000000000000000000ull};

     bool Currency::init() {
    if (!generateGenesisBlock())
    {
      logger(ERROR, BRIGHT_RED) << "Failed to generate genesis block";
      return false;
    }

    if (!get_block_hash(m_genesisBlock, m_genesisBlockHash))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to get genesis block hash";
      return false;
    }

		if (isTestnet()) {
			m_upgradeHeightV2 = 2;
			m_upgradeHeightV3 = 3;
			m_upgradeHeightV4 = 4;	
			m_upgradeHeightV5 = 5;
			m_upgradeHeightV6 = 6;	
			m_upgradeHeightV7 = 7;	
			m_upgradeHeightV8 = 8;
			m_upgradeHeightV9 = 9;

      m_blocksFileName = "testnet_" + m_blocksFileName;
      m_blocksCacheFileName = "testnet_" + m_blocksCacheFileName;
      m_blockIndexesFileName = "testnet_" + m_blockIndexesFileName;
      m_txPoolFileName = "testnet_" + m_txPoolFileName;
      m_blockchinIndicesFileName = "testnet_" + m_blockchinIndicesFileName;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::generateGenesisBlock()
  {
    m_genesisBlock = boost::value_initialized<Block>();

    // Hard code coinbase tx in genesis block, because "tru" generating tx use random, but genesis should be always the same
    std::string genesisCoinbaseTxHex = m_testnet ? GENESIS_COINBASE_TX_HEX_TESTNET : GENESIS_COINBASE_TX_HEX;
    BinaryArray minerTxBlob;

    bool r =
        fromHex(genesisCoinbaseTxHex, minerTxBlob) &&
        fromBinaryArray(m_genesisBlock.baseTransaction, minerTxBlob);

    if (!r)
    {
      logger(ERROR, BRIGHT_RED) << "failed to parse coinbase tx from hard coded blob";
      return false;
    }

    m_genesisBlock.majorVersion = BLOCK_MAJOR_VERSION_1;
    m_genesisBlock.minorVersion = BLOCK_MINOR_VERSION_0;
    m_genesisBlock.timestamp = 0;
    m_genesisBlock.nonce = 70;
    if (m_testnet)
    {
      ++m_genesisBlock.nonce;
    }

    //miner::find_nonce_for_given_block(bl, 1, 0);
    return true;
  }

	size_t Currency::blockGrantedFullRewardZoneByBlockVersion(uint8_t blockMajorVersion) const {
		if (blockMajorVersion >= BLOCK_MAJOR_VERSION_3) {
			return m_blockGrantedFullRewardZone;
		}
		else if (blockMajorVersion == BLOCK_MAJOR_VERSION_2) {
			return CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2;
		}
		else {
			return CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
		}
	}

	uint32_t Currency::upgradeHeight(uint8_t majorVersion) const {
		if (majorVersion == BLOCK_MAJOR_VERSION_2) {
			return m_upgradeHeightV2;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_3) {
			return m_upgradeHeightV3;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_4) {
			return m_upgradeHeightV4;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_5) {
			return m_upgradeHeightV5;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_6) {
			return m_upgradeHeightV6;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_7) {
			return m_upgradeHeightV7;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_8) {
			return m_upgradeHeightV8;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_9) {
			return m_upgradeHeightV9;
		}
		else {
			return static_cast<uint32_t>(-1);
		}
	}

	
	bool Currency::getBlockReward(uint8_t blockMajorVersion, size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins,
		uint64_t fee, uint32_t height, uint64_t& reward, int64_t& emissionChange) const {
		unsigned int m_emissionSpeedFactor = emissionSpeedFactor(blockMajorVersion);

    assert(alreadyGeneratedCoins <= m_moneySupply);
    assert(m_emissionSpeedFactor > 0 && m_emissionSpeedFactor <= 8 * sizeof(uint64_t));

    uint64_t baseReward = (m_moneySupply - alreadyGeneratedCoins) >> m_emissionSpeedFactor;
    size_t blockGrantedFullRewardZone = blockGrantedFullRewardZoneByBlockVersion(blockMajorVersion);
    medianSize = std::max(medianSize, blockGrantedFullRewardZone);
    if (currentBlockSize > UINT64_C(2) * medianSize)
    {
      logger(TRACE) << "Block cumulative size is too big: " << currentBlockSize << ", expected less than " << 2 * medianSize;
      return false;
    }

		uint64_t penalizedBaseReward = getPenalizedAmount(baseReward, medianSize, currentBlockSize);
		uint64_t penalizedFee = blockMajorVersion >= BLOCK_MAJOR_VERSION_2 ? getPenalizedAmount(fee, medianSize, currentBlockSize) : fee;
		if (cryptonoteCoinVersion() == 1) {
			penalizedFee = getPenalizedAmount(fee, medianSize, currentBlockSize);
		}

    emissionChange = penalizedBaseReward - (fee - penalizedFee);
    reward = penalizedBaseReward + penalizedFee;

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::calculateInterest(uint64_t amount, uint32_t term, uint32_t height) const
  {

    /* deposits 3.0 and investments 1.0 
    if (term % 21900 == 0)
    {
      return calculateInterestV3(amount, term);
    }

    // deposits 2.0 and investments 1.0 
    if (term % 64800 == 0)
    {
      return calculateInterestV2(amount, term);
    }

    if (term % 5040 == 0)
    {
      return calculateInterestV2(amount, term);
    }
*/
    uint64_t a = static_cast<uint64_t>(term) * m_depositMaxTotalRate - m_depositMinTotalRateFactor;
    uint64_t bHi;
    uint64_t bLo = mul128(amount, a, &bHi);
    uint64_t cHi;
    uint64_t cLo;
    uint64_t offchaininterest = 0;
    assert(std::numeric_limits<uint32_t>::max() / 100 > m_depositMaxTerm);
    div128_32(bHi, bLo, static_cast<uint32_t>(100 * m_depositMaxTerm), &cHi, &cLo);
    assert(cHi == 0);

    // early deposit multiplier 
    uint64_t interestHi;
    uint64_t interestLo;
    if (height <= CryptoNote::parameters::END_MULTIPLIER_BLOCK)
    {
      interestLo = mul128(cLo, CryptoNote::parameters::MULTIPLIER_FACTOR, &interestHi);
      assert(interestHi == 0);
    }
    else
    {
      interestHi = cHi;
      interestLo = cLo;
    }
    return offchaininterest;
  }

  /* ---------------------------------------------------------------------------------------------------- */
/*
  uint64_t Currency::calculateInterestV2(uint64_t amount, uint32_t term) const
  {

    uint64_t returnVal = 0;

    // investments 
    if (term % 64800 == 0)
    {

      // minimum 50000 for investments 
      uint64_t amount4Humans = amount / 1000000;
      // assert(amount4Humans >= 50000); //fails at block 166342

     //  quantity tiers 
      float qTier = 1;
      if (amount4Humans > 110000 && amount4Humans < 180000)
        qTier = static_cast<float>(1.01);

      if (amount4Humans >= 180000 && amount4Humans < 260000)
        qTier = static_cast<float>(1.02);

      if (amount4Humans >= 260000 && amount4Humans < 350000)
        qTier = static_cast<float>(1.03);

      if (amount4Humans >= 350000 && amount4Humans < 450000)
        qTier = static_cast<float>(1.04);

      if (amount4Humans >= 450000 && amount4Humans < 560000)
        qTier = static_cast<float>(1.05);

      if (amount4Humans >= 560000 && amount4Humans < 680000)
        qTier = static_cast<float>(1.06);

      if (amount4Humans >= 680000 && amount4Humans < 810000)
        qTier = static_cast<float>(1.07);

      if (amount4Humans >= 810000 && amount4Humans < 950000)
        qTier = static_cast<float>(1.08);

      if (amount4Humans >= 950000 && amount4Humans < 1100000)
        qTier = static_cast<float>(1.09);

      if (amount4Humans >= 1100000 && amount4Humans < 1260000)
        qTier = static_cast<float>(1.1);

      if (amount4Humans >= 1260000 && amount4Humans < 1430000)
        qTier = static_cast<float>(1.11);

      if (amount4Humans >= 1430000 && amount4Humans < 1610000)
        qTier = static_cast<float>(1.12);

      if (amount4Humans >= 1610000 && amount4Humans < 1800000)
        qTier = static_cast<float>(1.13);

      if (amount4Humans >= 1800000 && amount4Humans < 2000000)
        qTier = static_cast<float>(1.14);

      if (amount4Humans > 2000000)
        qTier = static_cast<float>(1.15);

      float mq = static_cast<float>(1.4473);
      float termQuarters = term / 64800;
      float m8 = 100.0 * pow(1.0 + (mq / 100.0), termQuarters) - 100.0;
      float m5 = termQuarters * 0.5;
      float m7 = m8 * (1 + (m5 / 100));
      float rate = m7 * qTier;
      float interest = amount * (rate / 100);
      returnVal = static_cast<uint64_t>(interest);
      return returnVal;
    }

    // weekly deposits 
    if (term % 5040 == 0)
    {
      uint64_t actualAmount = amount;
      float weeks = term / 5040;
      float baseInterest = static_cast<float>(0.0696);
      float interestPerWeek = static_cast<float>(0.0002);
      float interestRate = baseInterest + (weeks * interestPerWeek);
      float interest = actualAmount * ((weeks * interestRate) / 100);
      returnVal = static_cast<uint64_t>(interest);
      return returnVal;
    }

    return returnVal;

  }  Currency::calculateInterestV2 

  uint64_t Currency::calculateInterestV3(uint64_t amount, uint32_t term) const
  {

    uint64_t returnVal = 0;
    uint64_t amount4Humans = amount / 1000000;

    float baseInterest = static_cast<float>(0.029);

    if (amount4Humans >= 10000 && amount4Humans < 20000)
      baseInterest = static_cast<float>(0.039);

    if (amount4Humans >= 20000)
      baseInterest = static_cast<float>(0.049);

    // Consensus 2019 - Monthly deposits 

    float months = term / 21900;
    if (months > 12)
    {
      months = 12;
    }
    float ear = baseInterest + (months - 1) * 0.001;
    float eir = (ear / 12) * months;
    returnVal = static_cast<uint64_t>(eir);

    float interest = amount * eir;
    returnVal = static_cast<uint64_t>(interest);
    return returnVal;
  }  Currency::calculateInterestV3 
*/
  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::calculateTotalTransactionInterest(const Transaction &tx, uint32_t height) const
  {
    uint64_t interest = 0;
    for (const TransactionInput &input : tx.inputs)
    {
      if (input.type() == typeid(MultisignatureInput))
      {
        const MultisignatureInput &multisignatureInput = boost::get<MultisignatureInput>(input);
        if (multisignatureInput.term != 0)
        {
          interest += calculateInterest(multisignatureInput.amount, multisignatureInput.term, height);
        }
      }
    }

    return interest;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionInputAmount(const TransactionInput &in, uint32_t height) const
  {
    if (in.type() == typeid(KeyInput))
    {
      return boost::get<KeyInput>(in).amount;
    }
    else if (in.type() == typeid(MultisignatureInput))
    {
      const MultisignatureInput &multisignatureInput = boost::get<MultisignatureInput>(in);
      if (multisignatureInput.term == 0)
      {
        return multisignatureInput.amount;
      }
      else
      {
        return multisignatureInput.amount + calculateInterest(multisignatureInput.amount, multisignatureInput.term, height);
      }
    }
      else if (in.type() == typeid(BaseInput))
    {
      return 0;
    }
    else
    {
      assert(false);
      return 0;
    }
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionAllInputsAmount(const Transaction &tx, uint32_t height) const
  {
    uint64_t amount = 0;
    for (const auto &in : tx.inputs)
    {
      amount += getTransactionInputAmount(in, height);
    }

    return amount;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::getTransactionFee(const Transaction &tx, uint64_t &fee, uint32_t height) const
  {
    uint64_t amount_in = 0;
    uint64_t amount_out = 0;

    //if (tx.inputs.size() == 0)// || tx.outputs.size() == 0) //0 outputs needed in TestGenerator::constructBlock
    //	  return false;

    for (const auto &in : tx.inputs)
    {
      amount_in += getTransactionInputAmount(in, height);
    }

    for (const auto &o : tx.outputs)
    {
      amount_out += o.amount;
    }

    if (amount_out > amount_in)
    {
      // interest shows up in the output of the W/D transactions and W/Ds always have min fee
      if (tx.inputs.size() > 0 && tx.outputs.size() > 0 && amount_out > amount_in + parameters::MINIMUM_FEE)
      {
        fee = parameters::MINIMUM_FEE;
        logger(INFO) << "TRIGGERED: Currency.cpp getTransactionFee";
      }
      else
      {
        return false;
      }
    }
    else
    {
      fee = amount_in - amount_out;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionFee(const Transaction &tx, uint32_t height) const
  {
    uint64_t r = 0;
    if (!getTransactionFee(tx, r, height))
    {
      r = 0;
    }

    return r;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  size_t Currency::maxBlockCumulativeSize(uint64_t height) const
  {
    assert(height <= std::numeric_limits<uint64_t>::max() / m_maxBlockSizeGrowthSpeedNumerator);
    size_t maxSize = static_cast<size_t>(m_maxBlockSizeInitial +
                                         (height * m_maxBlockSizeGrowthSpeedNumerator) / m_maxBlockSizeGrowthSpeedDenominator);

    assert(maxSize >= m_maxBlockSizeInitial);
    return maxSize;
  }

	bool Currency::constructMinerTx(uint8_t blockMajorVersion, uint32_t height, size_t medianSize, uint64_t alreadyGeneratedCoins, size_t currentBlockSize,
		uint64_t fee, const AccountPublicAddress& minerAddress, Transaction& tx, const BinaryArray& extraNonce/* = BinaryArray()*/, size_t maxOuts/* = 1*/) const {

		tx.inputs.clear();
		tx.outputs.clear();
		tx.extra.clear();

    KeyPair txkey = generateKeyPair();
    addTransactionPublicKeyToExtra(tx.extra, txkey.publicKey);
    if (!extraNonce.empty())
    {
      if (!addExtraNonceToTransactionExtra(tx.extra, extraNonce))
      {
        return false;
      }
    }

    BaseInput in;
    in.blockIndex = height;

    uint64_t blockReward;
    int64_t emissionChange;
    if (!getBlockReward(blockMajorVersion, medianSize, currentBlockSize, alreadyGeneratedCoins, fee, height, blockReward, emissionChange))
    {
      logger(INFO) << "Block is too big";
      return false;
    }

    std::vector<uint64_t> outAmounts;
    decompose_amount_into_digits(
        blockReward, m_defaultDustThreshold,
        [&outAmounts](uint64_t a_chunk) { outAmounts.push_back(a_chunk); },
        [&outAmounts](uint64_t a_dust) { outAmounts.push_back(a_dust); });

    if (!(1 <= maxOuts))
    {
      logger(ERROR, BRIGHT_RED) << "max_out must be non-zero";
      return false;
    }

    while (maxOuts < outAmounts.size())
    {
      outAmounts[outAmounts.size() - 2] += outAmounts.back();
      outAmounts.resize(outAmounts.size() - 1);
    }

    uint64_t summaryAmounts = 0;
    for (size_t no = 0; no < outAmounts.size(); no++)
    {
      Crypto::KeyDerivation derivation = boost::value_initialized<Crypto::KeyDerivation>();
      Crypto::PublicKey outEphemeralPubKey = boost::value_initialized<Crypto::PublicKey>();

      bool r = Crypto::generate_key_derivation(minerAddress.viewPublicKey, txkey.secretKey, derivation);

      if (!(r))
      {
        logger(ERROR, BRIGHT_RED)
            << "while creating outs: failed to generate_key_derivation("
            << minerAddress.viewPublicKey << ", " << txkey.secretKey << ")";

        return false;
      }

      r = Crypto::derive_public_key(derivation, no, minerAddress.spendPublicKey, outEphemeralPubKey);

      if (!(r))
      {
        logger(ERROR, BRIGHT_RED)
            << "while creating outs: failed to derive_public_key("
            << derivation << ", " << no << ", "
            << minerAddress.spendPublicKey << ")";

        return false;
      }

      KeyOutput tk;
      tk.key = outEphemeralPubKey;

      TransactionOutput out;
      summaryAmounts += out.amount = outAmounts[no];
      out.target = tk;
      tx.outputs.push_back(out);
    }

    if (!(summaryAmounts == blockReward))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, summaryAmounts = " << summaryAmounts << " not equal blockReward = " << blockReward;
      return false;
    }

    tx.version = TRANSACTION_VERSION_1;
    // lock
    tx.unlockTime = height + m_minedMoneyUnlockWindow;
    tx.inputs.push_back(in);
    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const std::vector<uint64_t> &inputsAmounts, const std::vector<uint64_t> &outputsAmounts, size_t size) const
  {
    if (size > fusionTxMaxSize())
    {
      return false;
    }

    if (inputsAmounts.size() < fusionTxMinInputCount())
    {
      return false;
    }

    if (inputsAmounts.size() < outputsAmounts.size() * fusionTxMinInOutCountRatio())
    {
      return false;
    }

    uint64_t inputAmount = 0;
    for (auto amount : inputsAmounts)
    {
      if (amount < defaultDustThreshold())
      {
        return false;
      }

      inputAmount += amount;
    }

    std::vector<uint64_t> expectedOutputsAmounts;
    expectedOutputsAmounts.reserve(outputsAmounts.size());
    decomposeAmount(inputAmount, defaultDustThreshold(), expectedOutputsAmounts);
    std::sort(expectedOutputsAmounts.begin(), expectedOutputsAmounts.end());

    return expectedOutputsAmounts == outputsAmounts;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const Transaction &transaction, size_t size) const
  {
    assert(getObjectBinarySize(transaction) == size);

    std::vector<uint64_t> outputsAmounts;
    outputsAmounts.reserve(transaction.outputs.size());
    for (const TransactionOutput &output : transaction.outputs)
    {
      outputsAmounts.push_back(output.amount);
    }

    return isFusionTransaction(getInputsAmounts(transaction), outputsAmounts, size);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const Transaction &transaction) const
  {
    return isFusionTransaction(transaction, getObjectBinarySize(transaction));
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint32_t height) const
  {
    uint8_t ignore;
    return isAmountApplicableInFusionTransactionInput(amount, threshold, ignore, height);
  }

  bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint8_t &amountPowerOfTen, uint32_t height) const
  {
    if (amount >= threshold)
    {
      return false;
    }

    if (height < CryptoNote::parameters::UPGRADE_HEIGHT_V4 && amount < defaultDustThreshold())
    {
      return false;
    } /* why upgrade condition ?? */

    auto it = std::lower_bound(PRETTY_AMOUNTS.begin(), PRETTY_AMOUNTS.end(), amount);
    if (it == PRETTY_AMOUNTS.end() || amount != *it)
    {
      return false;
    }

    amountPowerOfTen = static_cast<uint8_t>(std::distance(PRETTY_AMOUNTS.begin(), it) / 9);
    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::accountAddressAsString(const AccountBase &account) const
  {
    return getAccountAddressAsStr(m_publicAddressBase58Prefix, account.getAccountKeys().address);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::accountAddressAsString(const AccountPublicAddress &accountPublicAddress) const
  {
    return getAccountAddressAsStr(m_publicAddressBase58Prefix, accountPublicAddress);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::parseAccountAddressString(const std::string &str, AccountPublicAddress &addr) const
  {
    uint64_t prefix;
    if (!CryptoNote::parseAccountAddressString(prefix, addr, str))
    {
      return false;
    }

    if (prefix != m_publicAddressBase58Prefix)
    {
      logger(DEBUGGING) << "Wrong address prefix: " << prefix << ", expected " << m_publicAddressBase58Prefix;
      return false;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::formatAmount(uint64_t amount) const
  {
    std::string s = std::to_string(amount);
    if (s.size() < m_numberOfDecimalPlaces + 1)
    {
      s.insert(0, m_numberOfDecimalPlaces + 1 - s.size(), '0');
    }

    s.insert(s.size() - m_numberOfDecimalPlaces, ".");
    return s;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::formatAmount(int64_t amount) const
  {
    std::string s = formatAmount(static_cast<uint64_t>(std::abs(amount)));

    if (amount < 0)
    {
      s.insert(0, "-");
    }

    return s;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::parseAmount(const std::string &str, uint64_t &amount) const
  {
    std::string strAmount = str;
    boost::algorithm::trim(strAmount);

    size_t pointIndex = strAmount.find_first_of('.');
    size_t fractionSize;

    if (std::string::npos != pointIndex)
    {
      fractionSize = strAmount.size() - pointIndex - 1;
      while (m_numberOfDecimalPlaces < fractionSize && '0' == strAmount.back())
      {
        strAmount.erase(strAmount.size() - 1, 1);
        --fractionSize;
      }

      if (m_numberOfDecimalPlaces < fractionSize)
      {
        return false;
      }

      strAmount.erase(pointIndex, 1);
    }
    else
    {
      fractionSize = 0;
    }

    if (strAmount.empty())
    {
      return false;
    }

    if (!std::all_of(strAmount.begin(), strAmount.end(), ::isdigit))
    {
      return false;
    }

    if (fractionSize < m_numberOfDecimalPlaces)
    {
      strAmount.append(m_numberOfDecimalPlaces - fractionSize, '0');
    }

    return Common::fromString(strAmount, amount);
  }

	difficulty_type Currency::nextDifficulty(uint32_t height, uint8_t blockMajorVersion, std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {

		if (blockMajorVersion >= BLOCK_MAJOR_VERSION_7) {
			return nextDifficultyV5(height, blockMajorVersion, timestamps, cumulativeDifficulties);
		}
		else if (blockMajorVersion >= BLOCK_MAJOR_VERSION_4) {
			return nextDifficultyV4(height, blockMajorVersion, timestamps, cumulativeDifficulties);
		}
		else if (blockMajorVersion >= BLOCK_MAJOR_VERSION_3) {
			return nextDifficultyV3(timestamps, cumulativeDifficulties);
		}
		else if (blockMajorVersion == BLOCK_MAJOR_VERSION_2) {
			return nextDifficultyV2(timestamps, cumulativeDifficulties);
		}
		else {
			return nextDifficultyV1(timestamps, cumulativeDifficulties);
		}
	}


	difficulty_type Currency::nextDifficultyV1(std::vector<uint64_t> timestamps,
				std::vector<difficulty_type> cumulativeDifficulties) const {
		assert(m_difficultyWindow >= 2);

    if (timestamps.size() > m_difficultyWindow)
    {
      timestamps.resize(m_difficultyWindow);
      cumulativeDifficulties.resize(m_difficultyWindow);
    }

    size_t length = timestamps.size();
    assert(length == cumulativeDifficulties.size());
    assert(length <= m_difficultyWindow);
    if (length <= 1)
    {
      return 1;
    }

    sort(timestamps.begin(), timestamps.end());

    size_t cutBegin, cutEnd;
    assert(2 * m_difficultyCut <= m_difficultyWindow - 2);
    if (length <= m_difficultyWindow - 2 * m_difficultyCut)
    {
      cutBegin = 0;
      cutEnd = length;
    }
    else
    {
      cutBegin = (length - (m_difficultyWindow - 2 * m_difficultyCut) + 1) / 2;
      cutEnd = cutBegin + (m_difficultyWindow - 2 * m_difficultyCut);
    }

    assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
    uint64_t timeSpan = timestamps[cutEnd - 1] - timestamps[cutBegin];
    if (timeSpan == 0)
    {
      timeSpan = 1;
    }

    difficulty_type totalWork = cumulativeDifficulties[cutEnd - 1] - cumulativeDifficulties[cutBegin];
    assert(totalWork > 0);

    uint64_t low, high;
    low = mul128(totalWork, m_difficultyTarget_DRGL, &high);
    if (high != 0 || low + timeSpan - 1 < low)
    {
      return 0;
    }

    return (low + timeSpan - 1) / timeSpan;
  }

	difficulty_type Currency::nextDifficultyV2(std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {

		// Difficulty calculation v. 2
		// based on Zawy difficulty algorithm v1.0
		// next Diff = Avg past N Diff * TargetInterval / Avg past N solve times
		// as described at https://github.com/monero-project/research-lab/issues/3
		// Window time span and total difficulty is taken instead of average as suggested by Nuclear_chaos

		size_t m_difficultyWindow_2 = CryptoNote::parameters::DIFFICULTY_WINDOW_V2;
		assert(m_difficultyWindow_2 >= 2);

		if (timestamps.size() > m_difficultyWindow_2) {
			timestamps.resize(m_difficultyWindow_2);
			cumulativeDifficulties.resize(m_difficultyWindow_2);
		}

		size_t length = timestamps.size();
		assert(length == cumulativeDifficulties.size());
		assert(length <= m_difficultyWindow_2);
		if (length <= 1) {
			return 1;
		}

		sort(timestamps.begin(), timestamps.end());

		uint64_t timeSpan = timestamps.back() - timestamps.front();
		if (timeSpan == 0) {
			timeSpan = 1;
		}

		difficulty_type totalWork = cumulativeDifficulties.back() - cumulativeDifficulties.front();
		assert(totalWork > 0);

		// uint64_t nextDiffZ = totalWork * m_difficultyTarget / timeSpan; 

		uint64_t low, high;
		low = mul128(totalWork, m_difficultyTarget_DRGL, &high);
		// blockchain error "Difficulty overhead" if this function returns zero
		if (high != 0) {
			return 0;
		}

		uint64_t nextDiffZ = low / timeSpan;

		// minimum limit
 		if (!isTestnet() && nextDiffZ < 10000) {
 			nextDiffZ = 10000;
 		}

		return nextDiffZ;
	}

	difficulty_type Currency::nextDifficultyV3(std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {

		// LWMA difficulty algorithm
		// Copyright (c) 2017-2018 Zawy
		// MIT license http://www.opensource.org/licenses/mit-license.php.
		// This is an improved version of Tom Harding's (Deger8) "WT-144"  
		// Karbowanec, Masari, Bitcoin Gold, and Bitcoin Cash have contributed.
		// See https://github.com/zawy12/difficulty-algorithms/issues/1 for other algos.
		// Do not use "if solvetime < 0 then solvetime = 1" which allows a catastrophic exploit.
		// T= target_solvetime;
		// N = int(45 * (600 / T) ^ 0.3));

		const int64_t T = static_cast<int64_t>(m_difficultyTarget_DRGL);
		size_t N = CryptoNote::parameters::DIFFICULTY_WINDOW_V3;

		// return a difficulty of 1 for first 3 blocks if it's the start of the chain
		if (timestamps.size() < 4) {
			return 1;
		}
		// otherwise, use a smaller N if the start of the chain is less than N+1
		else if (timestamps.size() < N + 1) {
			N = timestamps.size() - 1;
		}
		else if (timestamps.size() > N + 1) {
			timestamps.resize(N + 1);
			cumulativeDifficulties.resize(N + 1);
		}

		// To get an average solvetime to within +/- ~0.1%, use an adjustment factor.
		const double adjust = 0.998;
		// The divisor k normalizes LWMA.
		const double k = N * (N + 1) / 2;

		double LWMA(0), sum_inverse_D(0), harmonic_mean_D(0), nextDifficulty(0);
		int64_t solveTime(0);
		uint64_t difficulty(0), next_difficulty(0);

		// Loop through N most recent blocks.
		for (size_t i = 1; i <= N; i++) {
			solveTime = static_cast<int64_t>(timestamps[i]) - static_cast<int64_t>(timestamps[i - 1]);
			solveTime = std::min<int64_t>((T * 7), std::max<int64_t>(solveTime, (-6 * T)));
			difficulty = cumulativeDifficulties[i] - cumulativeDifficulties[i - 1];
			LWMA += (int64_t)(solveTime * i) / k;
			sum_inverse_D += 1 / static_cast<double>(difficulty);
		}

		// Keep LWMA sane in case something unforeseen occurs.
		if (static_cast<int64_t>(boost::math::round(LWMA)) < T / 20)
			LWMA = static_cast<double>(T) / 20;

		harmonic_mean_D = N / sum_inverse_D * adjust;
		nextDifficulty = harmonic_mean_D * T / LWMA;
		next_difficulty = static_cast<uint64_t>(nextDifficulty);
		
		// minimum limit
 		if (!isTestnet() && next_difficulty < 10000) {
 			next_difficulty = 10000;
 		} 

		return next_difficulty;
	}	
	
	

	difficulty_type Currency::nextDifficultyV4(uint32_t height, uint8_t blockMajorVersion,
		std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulativeDifficulties) const {
			
			// LWMA-1 difficulty algorithm 
			// Copyright (c) 2017-2018 Zawy, MIT License
			// https://github.com/zawy12/difficulty-algorithms/issues/3
			// See commented version for explanations & required config file changes. Fix FTL and MTP!

			   const uint64_t T = CryptoNote::parameters::DIFFICULTY_TARGET_DRGL;
			   uint64_t N = CryptoNote::parameters::DIFFICULTY_WINDOW_V3; // N=60, 90, and 120 for T=600, 120, 60.
			   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
			   uint32_t Dracarys = CryptoNote::parameters::UPGRADE_HEIGHT_V4;
	   		   uint64_t difficulty_plate = 10000;
	   		   

			   assert(timestamps.size() == cumulativeDifficulties.size() && timestamps.size() <= static_cast<uint64_t>(N + 1));

			   // If it's a new coin, do startup code. Do not remove in case other coins copy your code.
			   // uint64_t difficulty_guess = 10000;
			   // if (timestamps.size() <= 12 ) {   return difficulty_guess;   }
			   // if ( timestamps.size()  < N +1 ) { N = timestamps.size()-1;  }
			   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 blocks after fork. 
			   // This will also cover up a very common type of backwards-incompatible fork.
			   // difficulty_guess = 10000; //  Dev may change.  Guess lower than anything expected.
			   
	  		   if ( height <= Dracarys + 1 + N ) { return difficulty_plate;  }
 
			   previous_timestamp = timestamps[0];
			   for ( i = 1; i <= N; i++) {        
			      // Safely prevent out-of-sequence timestamps
			      if ( timestamps[i]  > previous_timestamp ) {   this_timestamp = timestamps[i];  } 
			      else {  this_timestamp = previous_timestamp;   }
			      L +=  i*std::min(6*T , this_timestamp - previous_timestamp);
			      previous_timestamp = this_timestamp; 
			   }
			   if (L < N*N*T/20 ) { L =  N*N*T/20; }
			   avg_D = ( cumulativeDifficulties[N] - cumulativeDifficulties[0] )/ N;
   
			   // Prevent round off error for small D and overflow for large D.
			   if (avg_D > 2000000*N*N*T) { 
			       next_D = (avg_D/(200*L))*(N*(N+1)*T*97);   
			   }   
			   else {    next_D = (avg_D*N*(N+1)*T*97)/(200*L);    }
	
			   // Optional. Make all insignificant digits zero for easy reading.
			   i = 1000000000;
			   while (i > 1) { 
			     if ( next_D > i*100 ) { next_D = ((next_D+i/2)/i)*i; break; }
			     else { i /= 10; }
			   }
			   // Make least 2 digits = size of hash rate change last 11 blocks if it's statistically significant.
			   // D=2540035 => hash rate 3.5x higher than D expected. Blocks coming 3.5x too fast.
			   if ( next_D > 10000 ) { 
			     uint64_t est_HR = (10*(11*T+(timestamps[N]-timestamps[N-11])/2))/(timestamps[N]-timestamps[N-11]+1);
			     if (  est_HR > 5 && est_HR < 22 )  {  est_HR=0;   }
			     est_HR = std::min(static_cast<uint64_t>(99), est_HR);
			     next_D = ((next_D+50)/100)*100 + est_HR;  
			   }
	         	   // mini-lim
	   		   if (!isTestnet() && next_D < 10000) {
	  		   	next_D = 10000;
			   
			   }

			   return  next_D;
	}

		difficulty_type Currency::nextDifficultyV5(uint32_t height, uint8_t blockMajorVersion,
		std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulativeDifficulties) const {
			
			// LWMA-1 difficulty algorithm 
			// Copyright (c) 2017-2018 Zawy, MIT License
			// https://github.com/zawy12/difficulty-algorithms/issues/3
			// See commented version for explanations & required config file changes. Fix FTL and MTP!
			   
			   const uint64_t T = CryptoNote::parameters::DIFFICULTY_TARGET;
			   uint64_t N = CryptoNote::parameters::DIFFICULTY_WINDOW_V4; // N=60, 90, and 120 for T=600, 120, 60.
			   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
			   uint32_t FanG = CryptoNote::parameters::UPGRADE_HEIGHT_V7;
	   		   uint64_t difficulty_plate = 100000;
	   		   

			   assert(timestamps.size() == cumulativeDifficulties.size() && timestamps.size() <= static_cast<uint64_t>(N + 1));

			   // If it's a new coin, do startup code. Do not remove in case other coins copy your code.
			   // uint64_t difficulty_guess = 10000;
			   // if (timestamps.size() <= 12 ) {   return difficulty_guess;   }
			   // if ( timestamps.size()  < N +1 ) { N = timestamps.size()-1;  }
			   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 blocks after fork. 
			   // This will also cover up a very common type of backwards-incompatible fork.
			   // difficulty_guess = 10000; //  Dev may change.  Guess lower than anything expected.
			  
	  		   if ( height <= FanG + 1 + N ) { return difficulty_plate;  }

			   previous_timestamp = timestamps[0];
			   for ( i = 1; i <= N; i++) {        
			      // Safely prevent out-of-sequence timestamps
			      if ( timestamps[i]  > previous_timestamp ) {   this_timestamp = timestamps[i];  } 
			      else {  this_timestamp = previous_timestamp;   }
			      L +=  i*std::min(6*T , this_timestamp - previous_timestamp);
			      previous_timestamp = this_timestamp; 
			   }
			   if (L < N*N*T/20 ) { L =  N*N*T/20; }
			   avg_D = ( cumulativeDifficulties[N] - cumulativeDifficulties[0] )/ N;
   
			   // Prevent round off error for small D and overflow for large D.
			   if (avg_D > 2000000*N*N*T) { 
			       next_D = (avg_D/(200*L))*(N*(N+1)*T*97);   
			   }   
			   else {    next_D = (avg_D*N*(N+1)*T*97)/(200*L);    }
	
			   // Optional. Make all insignificant digits zero for easy reading.
			   i = 1000000000;
			   while (i > 1) { 
			     if ( next_D > i*100 ) { next_D = ((next_D+i/2)/i)*i; break; }
			     else { i /= 10; }
			   }
			   // Make least 2 digits = size of hash rate change last 11 blocks if it's statistically significant.
			   // D=2540035 => hash rate 3.5x higher than D expected. Blocks coming 3.5x too fast.
			   if ( next_D > 10000 ) { 
			     uint64_t est_HR = (10*(11*T+(timestamps[N]-timestamps[N-11])/2))/(timestamps[N]-timestamps[N-11]+1);
			     if (  est_HR > 5 && est_HR < 22 )  {  est_HR=0;   }
			     est_HR = std::min(static_cast<uint64_t>(99), est_HR);
			     next_D = ((next_D+50)/100)*100 + est_HR;  
			   }
	         	   // mini-lim
	   		   if (!isTestnet() && next_D < 10000) {
	  		   	next_D = 10000;
			   
			   }

			   return  next_D;
	}


	bool Currency::checkProofOfWorkV1(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic,
		Crypto::Hash& proofOfWork) const {
		if (BLOCK_MAJOR_VERSION_1 != block.majorVersion) {
			return false;
		}

		if (!get_block_longhash(context, block, proofOfWork)) {
			return false;
		}

		return check_hash(proofOfWork, currentDiffic);
	}

	bool Currency::checkProofOfWorkV2(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic,
		Crypto::Hash& proofOfWork) const {
		if (block.majorVersion < BLOCK_MAJOR_VERSION_2) {
			return false;
		}

		if (!get_block_longhash(context, block, proofOfWork)) {
			return false;
		}

		if (!check_hash(proofOfWork, currentDiffic)) {
			return false;
		}

		TransactionExtraMergeMiningTag mmTag;
		if (!getMergeMiningTagFromExtra(block.parentBlock.baseTransaction.extra, mmTag)) {
			logger(ERROR) << "merge mining tag wasn't found in extra of the parent block miner transaction";
			return false;
		}

		if (8 * sizeof(m_genesisBlockHash) < block.parentBlock.blockchainBranch.size()) {
			return false;
		}

		Crypto::Hash auxBlockHeaderHash;
		if (!get_aux_block_header_hash(block, auxBlockHeaderHash)) {
			return false;
		}

		Crypto::Hash auxBlocksMerkleRoot;
		Crypto::tree_hash_from_branch(block.parentBlock.blockchainBranch.data(), block.parentBlock.blockchainBranch.size(),
			auxBlockHeaderHash, &m_genesisBlockHash, auxBlocksMerkleRoot);

		if (auxBlocksMerkleRoot != mmTag.merkleRoot) {
			logger(ERROR, BRIGHT_YELLOW) << "Aux block hash wasn't found in merkle tree";
			return false;
		}

		return true;
	}

	bool Currency::checkProofOfWork(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic, Crypto::Hash& proofOfWork) const {
		switch (block.majorVersion) {
		case BLOCK_MAJOR_VERSION_1:
			return checkProofOfWorkV1(context, block, currentDiffic, proofOfWork);

		case BLOCK_MAJOR_VERSION_2:
		case BLOCK_MAJOR_VERSION_3:
		case BLOCK_MAJOR_VERSION_4:
		case BLOCK_MAJOR_VERSION_5:
		case BLOCK_MAJOR_VERSION_6:
		case BLOCK_MAJOR_VERSION_7:
		case BLOCK_MAJOR_VERSION_8:
		case BLOCK_MAJOR_VERSION_9:


			return checkProofOfWorkV2(context, block, currentDiffic, proofOfWork);
		}

		logger(ERROR, BRIGHT_RED) << "Unknown block major version: " << block.majorVersion << "." << block.minorVersion;
		return false;
	}
    size_t Currency::getApproximateMaximumInputCount(size_t transactionSize, size_t outputCount, size_t mixinCount) const {
    const size_t KEY_IMAGE_SIZE = sizeof(Crypto::KeyImage);
    const size_t OUTPUT_KEY_SIZE = sizeof(decltype(KeyOutput::key));
    const size_t AMOUNT_SIZE = sizeof(uint64_t) + 2;                   // varint
    const size_t GLOBAL_INDEXES_VECTOR_SIZE_SIZE = sizeof(uint8_t);    // varint
    const size_t GLOBAL_INDEXES_INITIAL_VALUE_SIZE = sizeof(uint32_t); // varint
    const size_t GLOBAL_INDEXES_DIFFERENCE_SIZE = sizeof(uint32_t);    // varint
    const size_t SIGNATURE_SIZE = sizeof(Crypto::Signature);
    const size_t EXTRA_TAG_SIZE = sizeof(uint8_t);
    const size_t INPUT_TAG_SIZE = sizeof(uint8_t);
    const size_t OUTPUT_TAG_SIZE = sizeof(uint8_t);
    const size_t PUBLIC_KEY_SIZE = sizeof(Crypto::PublicKey);
    const size_t TRANSACTION_VERSION_SIZE = sizeof(uint8_t);
    const size_t TRANSACTION_UNLOCK_TIME_SIZE = sizeof(uint64_t);

    const size_t outputsSize = outputCount * (OUTPUT_TAG_SIZE + OUTPUT_KEY_SIZE + AMOUNT_SIZE);
    const size_t headerSize = TRANSACTION_VERSION_SIZE + TRANSACTION_UNLOCK_TIME_SIZE + EXTRA_TAG_SIZE + PUBLIC_KEY_SIZE;
    const size_t inputSize = INPUT_TAG_SIZE + AMOUNT_SIZE + KEY_IMAGE_SIZE + SIGNATURE_SIZE + GLOBAL_INDEXES_VECTOR_SIZE_SIZE +
                             GLOBAL_INDEXES_INITIAL_VALUE_SIZE + mixinCount * (GLOBAL_INDEXES_DIFFERENCE_SIZE + SIGNATURE_SIZE);

    return (transactionSize - headerSize - outputsSize) / inputSize;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder::CurrencyBuilder(Logging::ILogger &log) : m_currency(log)
  {
    maxBlockNumber(parameters::CRYPTONOTE_MAX_BLOCK_NUMBER);
    maxBlockBlobSize(parameters::CRYPTONOTE_MAX_BLOCK_BLOB_SIZE);
    maxTxSize(parameters::CRYPTONOTE_MAX_TX_SIZE);
    publicAddressBase58Prefix(parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
    minedMoneyUnlockWindow(parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);

    timestampCheckWindow(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
    timestampCheckWindow_v1(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1);
    blockFutureTimeLimit(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT);
    blockFutureTimeLimit_v1(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1);
    blockFutureTimeLimit_v2(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V2);

		moneySupply(parameters::MONEY_SUPPLY);
		emissionSpeedFactor(parameters::EMISSION_SPEED_FACTOR);
		emissionSpeedFactor_FANGO(parameters::EMISSION_SPEED_FACTOR_FANGO);
                emissionSpeedFactor_FUEGO(parameters::EMISSION_SPEED_FACTOR_FUEGO);


		cryptonoteCoinVersion(parameters::CRYPTONOTE_COIN_VERSION);

		rewardBlocksWindow(parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW);
		blockGrantedFullRewardZone(parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
		minerTxBlobReservedSize(parameters::CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
	
		minMixin(parameters::MIN_TX_MIXIN_SIZE);
		maxMixin(parameters::MAX_TX_MIXIN_SIZE);

    numberOfDecimalPlaces(parameters::CRYPTONOTE_DISPLAY_DECIMAL_POINT);

    minimumFee(parameters::MINIMUM_FEE);
    minimumFeeV1(parameters::MINIMUM_FEE_V1);
    minimumFeeV2(parameters::MINIMUM_FEE_V2);
    minimumFeeBanking(parameters::MINIMUM_FEE_BANKING);
    defaultDustThreshold(parameters::DEFAULT_DUST_THRESHOLD);

    difficultyTarget(parameters::DIFFICULTY_TARGET);
    difficultyTarget_DRGL(parameters::DIFFICULTY_TARGET_DRGL);
    difficultyWindow(parameters::DIFFICULTY_WINDOW);
    difficultyLag(parameters::DIFFICULTY_LAG);
    difficultyCut(parameters::DIFFICULTY_CUT);

    depositMinAmount(parameters::DEPOSIT_MIN_AMOUNT);
    depositMinTerm(parameters::DEPOSIT_MIN_TERM);
    depositMaxTerm(parameters::DEPOSIT_MAX_TERM);

    maxBlockSizeInitial(parameters::MAX_BLOCK_SIZE_INITIAL);
    maxBlockSizeGrowthSpeedNumerator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR);
    maxBlockSizeGrowthSpeedDenominator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR);

    lockedTxAllowedDeltaSeconds(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS);
    lockedTxAllowedDeltaSeconds_v2(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2);
    lockedTxAllowedDeltaBlocks(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);

    mempoolTxLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_LIVETIME);
    mempoolTxFromAltBlockLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME);
    numberOfPeriodsToForgetTxDeletedFromPool(parameters::CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL);

    upgradeHeightV2(parameters::UPGRADE_HEIGHT_V2);
    upgradeHeightV3(parameters::UPGRADE_HEIGHT_V3);
    upgradeHeightV4(parameters::UPGRADE_HEIGHT_V4);
    upgradeHeightV5(parameters::UPGRADE_HEIGHT_V5);
    upgradeHeightV6(parameters::UPGRADE_HEIGHT_V6);
    upgradeHeightV7(parameters::UPGRADE_HEIGHT_V7);
    upgradeHeightV8(parameters::UPGRADE_HEIGHT_V8);
    upgradeHeightV9(parameters::UPGRADE_HEIGHT_V9);

    upgradeVotingThreshold(parameters::UPGRADE_VOTING_THRESHOLD);
    upgradeVotingWindow(parameters::UPGRADE_VOTING_WINDOW);
    upgradeWindow(parameters::UPGRADE_WINDOW);

    transactionMaxSize(parameters::CRYPTONOTE_MAX_TX_SIZE_LIMIT);
    fusionTxMaxSize(parameters::FUSION_TX_MAX_SIZE);
    fusionTxMinInputCount(parameters::FUSION_TX_MIN_INPUT_COUNT);
    fusionTxMinInOutCountRatio(parameters::FUSION_TX_MIN_IN_OUT_COUNT_RATIO);

    blocksFileName(parameters::CRYPTONOTE_BLOCKS_FILENAME);
    blocksCacheFileName(parameters::CRYPTONOTE_BLOCKSCACHE_FILENAME);
    blockIndexesFileName(parameters::CRYPTONOTE_BLOCKINDEXES_FILENAME);
    txPoolFileName(parameters::CRYPTONOTE_POOLDATA_FILENAME);
    blockchinIndicesFileName(parameters::CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME);

    testnet(false);
  }

	Transaction CurrencyBuilder::generateGenesisTransaction() {
		CryptoNote::Transaction tx;
		CryptoNote::AccountPublicAddress ac = boost::value_initialized<CryptoNote::AccountPublicAddress>();
		m_currency.constructMinerTx(1, 0, 0, 0, 0, 0, ac, tx); // zero fee in genesis
		return tx;
	}
	CurrencyBuilder& CurrencyBuilder::emissionSpeedFactor(unsigned int val) {
		if (val <= 0 || val > 8 * sizeof(uint64_t)) {
			throw std::invalid_argument("val at emissionSpeedFactor()");
		}

		m_currency.m_emissionSpeedFactor = val;
		return *this;
	}
        CurrencyBuilder& CurrencyBuilder::emissionSpeedFactor_FANGO(unsigned int val) {
		if (val <= 0 || val > 8 * sizeof(uint64_t)) {
			throw std::invalid_argument("val at emissionSpeedFactor_FANGO()");
		}

		m_currency.m_emissionSpeedFactor_FANGO = val;
		return *this;
	}
        CurrencyBuilder& CurrencyBuilder::emissionSpeedFactor_FUEGO(unsigned int val) {
                if (val <= 0 || val > 8 * sizeof(uint64_t)) {
                        throw std::invalid_argument("val at emissionSpeedFactor_FUEGO()");
                }

                m_currency.m_emissionSpeedFactor_FUEGO = val;
                return *this;
        }

	CurrencyBuilder& CurrencyBuilder::numberOfDecimalPlaces(size_t val) {
		m_currency.m_numberOfDecimalPlaces = val;
		m_currency.m_coin = 1;
		for (size_t i = 0; i < m_currency.m_numberOfDecimalPlaces; ++i) {
			m_currency.m_coin *= 10;
		}

    return *this;
  }

  CurrencyBuilder &CurrencyBuilder::difficultyWindow(size_t val)
  {
    if (val < 2)
    {
      throw std::invalid_argument("val at difficultyWindow()");
    }

    m_currency.m_difficultyWindow = val;
    return *this;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder &CurrencyBuilder::upgradeVotingThreshold(unsigned int val)
  {
    if (val <= 0 || val > 100)
    {
      throw std::invalid_argument("val at upgradeVotingThreshold()");
    }

    m_currency.m_upgradeVotingThreshold = val;
    return *this;
  }

	CurrencyBuilder& CurrencyBuilder::upgradeWindow(size_t val) {
		if (val <= 0) {
			throw std::invalid_argument("val at upgradeWindow()");
		}

		m_currency.m_upgradeWindow = static_cast<uint32_t>(val);
		return *this;
	}

} // namespace CryptoNote
