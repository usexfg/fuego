// Copyright (c) 2019-2021 Fango Developers
// Copyright (c) 2018-2021 Fandom Gold Society
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
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

#include "CryptoNote.h"
#include "P2pProtocolTypes.h"
#include <list>
#include <boost/uuid/uuid.hpp>

namespace CryptoNote {

  struct CryptoNoteConnectionContext;

  struct IP2pEndpoint {
    virtual void relay_notify_to_all(int command, const BinaryArray& data_buff, const net_connection_id* excludeConnection) = 0;
    virtual bool invoke_notify_to_peer(int command, const BinaryArray& req_buff, const CryptoNote::CryptoNoteConnectionContext& context) = 0;
    virtual uint64_t get_connections_count()=0;
    virtual void for_each_connection(std::function<void(CryptoNote::CryptoNoteConnectionContext&, PeerIdType)> f) = 0;
    virtual void drop_connection(CryptoNoteConnectionContext &context, bool add_fail) = 0;

    // can be called from external threads
    virtual void externalRelayNotifyToAll(int command, const BinaryArray &data_buff, const net_connection_id *excludeConnection) = 0;
    virtual void externalRelayNotifyToList(int command, const BinaryArray &data_buff, const std::list<boost::uuids::uuid> relayList) = 0;
  };

  struct p2p_endpoint_stub: public IP2pEndpoint {
    virtual void relay_notify_to_all(int command, const BinaryArray& data_buff, const net_connection_id* excludeConnection) override {}
    virtual bool invoke_notify_to_peer(int command, const BinaryArray& req_buff, const CryptoNote::CryptoNoteConnectionContext& context) override { return true; }
    virtual void drop_connection(CryptoNoteConnectionContext &context, bool add_fail) override {}
    virtual void for_each_connection(std::function<void(CryptoNote::CryptoNoteConnectionContext&, PeerIdType)> f) override {}
    virtual uint64_t get_connections_count() override { return 0; }
    virtual void externalRelayNotifyToAll(int command, const BinaryArray &data_buff, const net_connection_id *excludeConnection) override {}
    virtual void externalRelayNotifyToList(int command, const BinaryArray &data_buff, const std::list<boost::uuids::uuid> relayList) override {}
  };
}
