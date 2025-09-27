// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2020 The Karbo developers
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "P2pProtocolTypes.h"

#include "crypto/crypto.h"
#include "CryptoNoteConfig.h"
#include "CryptoNoteCore/CryptoNoteStatInfo.h"

// new serialization
#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "CryptoNoteCore/CryptoNoteSerialization.h"

namespace CryptoNote
{
  inline bool serialize(uuid& v, Common::StringView name, ISerializer& s) {
    return s.binary(&v, sizeof(v), name);
  }

  struct network_config
  {
    void serialize(ISerializer& s) {
      KV_MEMBER(connections_count)
      KV_MEMBER(handshake_interval)
      KV_MEMBER(packet_max_size)
      KV_MEMBER(config_id)
    }

    uint32_t connections_count;
    uint32_t connection_timeout;
    uint32_t ping_connection_timeout;
    uint32_t handshake_interval;
    uint32_t packet_max_size;
    uint32_t config_id;
    uint32_t send_peerlist_sz;
  };

  struct basic_node_data
  {
    uuid network_id;
    uint8_t version;
    uint64_t local_time;
    uint32_t my_port;
    PeerIdType peer_id;

    void serialize(ISerializer& s) {
      KV_MEMBER(network_id)
      if (s.type() == ISerializer::INPUT) {
        version = 0;
      }
      KV_MEMBER(version)
      KV_MEMBER(peer_id)
      KV_MEMBER(local_time)
      KV_MEMBER(my_port)
    }
  };
  
  struct CORE_SYNC_DATA
  {
    uint32_t current_height;
    Crypto::Hash top_id;

    void serialize(ISerializer& s) {
      KV_MEMBER(current_height)
      KV_MEMBER(top_id)
    }
  };

#define P2P_COMMANDS_POOL_BASE 1000

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_HANDSHAKE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 1 };

    struct request
    {
      basic_node_data node_data;
      CORE_SYNC_DATA payload_data;

      void serialize(ISerializer& s) {
        KV_MEMBER(node_data)
        KV_MEMBER(payload_data)
      }

    };

    struct response
    {
      basic_node_data node_data;
      CORE_SYNC_DATA payload_data;
      std::list<PeerlistEntry> local_peerlist; 

      void serialize(ISerializer& s) {
        KV_MEMBER(node_data)
        KV_MEMBER(payload_data)
        serializeAsBinary(local_peerlist, "local_peerlist", s);
      }
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_TIMED_SYNC
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 2 };

    struct request
    {
      CORE_SYNC_DATA payload_data;

      void serialize(ISerializer& s) {
        KV_MEMBER(payload_data)
      }

    };

    struct response
    {
      uint64_t local_time;
      CORE_SYNC_DATA payload_data;
      std::list<PeerlistEntry> local_peerlist;

      void serialize(ISerializer& s) {
        KV_MEMBER(local_time)
        KV_MEMBER(payload_data)
        serializeAsBinary(local_peerlist, "local_peerlist", s);
      }
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct COMMAND_PING
  {
    /*
      Used to make "callback" connection, to be sure that opponent node 
      have accessible connection point. Only other nodes can add peer to peerlist,
      and ONLY in case when peer has accepted connection and answered to ping.
    */
    enum { ID = P2P_COMMANDS_POOL_BASE + 3 };

#define PING_OK_RESPONSE_STATUS_TEXT "OK"

    struct request
    {
      /*actually we don't need to send any real data*/
      void serialize(ISerializer& s) {}
    };

    struct response
    {
      std::string status;
      PeerIdType peer_id;

      void serialize(ISerializer& s) {
        KV_MEMBER(status)
        KV_MEMBER(peer_id)
      }
    };
  };

  
#ifdef ALLOW_DEBUG_COMMANDS
  //These commands are considered as insecure, and made in debug purposes for a limited lifetime. 
  //Anyone who feel unsafe with this commands can disable the ALLOW_GET_STAT_COMMAND macro.

  struct proof_of_trust
  {
    PeerIdType peer_id;
    uint64_t    time;
    Crypto::Signature sign;

    void serialize(ISerializer& s) {
      KV_MEMBER(peer_id)
      KV_MEMBER(time)
      KV_MEMBER(sign)
    }
  };

  inline Crypto::Hash get_proof_of_trust_hash(const proof_of_trust& pot) {
    std::string s;
    s.append(reinterpret_cast<const char*>(&pot.peer_id), sizeof(pot.peer_id));
    s.append(reinterpret_cast<const char*>(&pot.time), sizeof(pot.time));
    return Crypto::cn_fast_hash(s.data(), s.size());
  }

  struct COMMAND_REQUEST_STAT_INFO
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 4 };

    struct request
    {
      proof_of_trust tr;

      void serialize(ISerializer& s) {
        KV_MEMBER(tr)
      }
    };
    
    struct response
    {
      std::string version;
      std::string os_version;
      uint64_t connections_count;
      uint64_t incoming_connections_count;
      core_stat_info payload_info;

      void serialize(ISerializer& s) {
        KV_MEMBER(version)
        KV_MEMBER(os_version)
        KV_MEMBER(connections_count)
        KV_MEMBER(incoming_connections_count)
        KV_MEMBER(payload_info)
      }
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_NETWORK_STATE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 5 };

    struct request
    {
      proof_of_trust tr;

      void serialize(ISerializer& s) {
        KV_MEMBER(tr)
      }
    };

    struct response
    {
      std::list<PeerlistEntry> local_peerlist_white;
      std::list<PeerlistEntry> local_peerlist_gray;
      std::list<connection_entry> connections_list;
      PeerIdType my_id;
      uint64_t local_time;

      void serialize(ISerializer& s) {
        serializeAsBinary(local_peerlist_white, "local_peerlist_white", s);
        serializeAsBinary(local_peerlist_gray, "local_peerlist_gray", s);
        serializeAsBinary(connections_list, "connections_list", s);
        KV_MEMBER(my_id)
        KV_MEMBER(local_time)
      }
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_PEER_ID
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 6 };

    struct request
    {
      void serialize(ISerializer& s) {}
    };

    struct response
    {
      PeerIdType my_id;

      void serialize(ISerializer& s) {
        KV_MEMBER(my_id)
      }
    };
  };

  /************************************************************************/
  /* Elderfier Proof System Commands                                      */
  /* Consensus Flow: FastPass(3/3) -> Fallback(6/8) -> Reject+Council   */
  /************************************************************************/
  struct COMMAND_REQUEST_ELDERFIER_PROOF
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 10 };

    struct request
    {
      Crypto::Hash burn_tx_hash;
      uint8_t consensus_path;  // FAST_PASS=0, FALLBACK=1, COUNCIL_REVIEW=2
      uint64_t timestamp;

      void serialize(ISerializer& s) {
        KV_MEMBER(burn_tx_hash)
        KV_MEMBER(consensus_path)
        KV_MEMBER(timestamp)
      }
    };

    struct response
    {
      Crypto::Hash burn_tx_hash;
      std::vector<uint8_t> proof_data;
      std::vector<Crypto::PublicKey> participating_nodes;
      Crypto::Signature threshold_signature;
      uint8_t consensus_path;
      bool is_success;

      void serialize(ISerializer& s) {
        KV_MEMBER(burn_tx_hash)
        serializeAsBinary(proof_data, "proof_data", s);
        serializeAsBinary(participating_nodes, "participating_nodes", s);
        KV_MEMBER(threshold_signature)
        KV_MEMBER(consensus_path)
        KV_MEMBER(is_success)
      }
    };
  };

  struct COMMAND_ELDERFIER_PROOF_SIGNATURE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 11 };

    struct request
    {
      Crypto::Hash burn_tx_hash;
      std::vector<uint8_t> partial_signature;
      Crypto::PublicKey signer_key;
      uint8_t consensus_path;

      void serialize(ISerializer& s) {
        KV_MEMBER(burn_tx_hash)
        serializeAsBinary(partial_signature, "partial_signature", s);
        KV_MEMBER(signer_key)
        KV_MEMBER(consensus_path)
      }
    };
  };

  struct COMMAND_ELDERFIER_COUNCIL_VOTE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 12 };

    // Used when consensus fails - sends failed case to Elder Council for review
    // This is async and doesn't block the rejection of the proof
    struct request
    {
      Crypto::Hash burn_tx_hash;
      uint8_t failure_reason;  // INVALID_PROOF=0, NETWORK_SYNC=1, BAD_ACTOR=2
      std::vector<Crypto::PublicKey> non_responding_nodes;
      std::string vote_choice;  // "INVALID" | "NETWORK_ISSUE" | "BAD_ACTOR" | "ALL_GOOD"
      Crypto::Signature vote_signature;

      void serialize(ISerializer& s) {
        KV_MEMBER(burn_tx_hash)
        KV_MEMBER(failure_reason)
        serializeAsBinary(non_responding_nodes, "non_responding_nodes", s);
        KV_MEMBER(vote_choice)
        KV_MEMBER(vote_signature)
      }
    };
  };

#endif


}
