// Copyright (c) 2017-2025 Elderfire Privacy Council
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

#pragma once

#include "IWalletLegacy.h"
#include "Common/ObserverManager.h"

namespace CryptoNote
{

class WalletLegacyEvent
{
public:
  virtual ~WalletLegacyEvent() {
  };

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) = 0;
};

class WalletTransactionUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletTransactionUpdatedEvent(TransactionId transactionId) : m_id(transactionId) {};
  virtual ~WalletTransactionUpdatedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::transactionUpdated, m_id);
  }

private:
  TransactionId m_id;
};

class WalletSendTransactionCompletedEvent : public WalletLegacyEvent
{
public:
  WalletSendTransactionCompletedEvent(TransactionId transactionId, std::error_code result) : m_id(transactionId), m_error(result) {};
  virtual ~WalletSendTransactionCompletedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::sendTransactionCompleted, m_id, m_error);
  }

private:
  TransactionId m_id;
  std::error_code m_error;
};

class WalletExternalTransactionCreatedEvent : public WalletLegacyEvent
{
public:
  WalletExternalTransactionCreatedEvent(TransactionId transactionId) : m_id(transactionId) {};
  virtual ~WalletExternalTransactionCreatedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::externalTransactionCreated, m_id);
  }
private:
  TransactionId m_id;
};

class WalletDepositsUpdatedEvent : public WalletLegacyEvent {
public:
  WalletDepositsUpdatedEvent(std::vector<DepositId>&& depositIds) : updatedDeposits(depositIds) {}

  virtual ~WalletDepositsUpdatedEvent() {}

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override {
    observer.notify(&IWalletLegacyObserver::depositsUpdated, updatedDeposits);
  }
private:
  std::vector<DepositId> updatedDeposits;
};

class WalletSynchronizationProgressUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletSynchronizationProgressUpdatedEvent(uint32_t current, uint32_t total) : m_current(current), m_total(total) {};
  virtual ~WalletSynchronizationProgressUpdatedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::synchronizationProgressUpdated, m_current, m_total);
  }

private:
  uint32_t m_current;
  uint32_t m_total;
};

class WalletSynchronizationCompletedEvent : public WalletLegacyEvent {
public:
  WalletSynchronizationCompletedEvent(uint32_t current, uint32_t total, std::error_code result) : m_ec(result) {};
  virtual ~WalletSynchronizationCompletedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override {
    observer.notify(&IWalletLegacyObserver::synchronizationCompleted, m_ec);
  }

private:
  std::error_code m_ec;
};

class WalletActualBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {};
  virtual ~WalletActualBalanceUpdatedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::actualBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {};
  virtual ~WalletPendingBalanceUpdatedEvent() {};

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::pendingBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletActualDepositBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualDepositBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletActualDepositBalanceUpdatedEvent() {}

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::actualDepositBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingDepositBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingDepositBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletPendingDepositBalanceUpdatedEvent() {}

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::pendingDepositBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

/* investments */

class WalletActualInvestmentBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualInvestmentBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletActualInvestmentBalanceUpdatedEvent() {}

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::actualInvestmentBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingInvestmentBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingInvestmentBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletPendingInvestmentBalanceUpdatedEvent() {}

  virtual void notify(Tools::ObserverManager<CryptoNote::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::pendingInvestmentBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};





} /* namespace CryptoNote */
