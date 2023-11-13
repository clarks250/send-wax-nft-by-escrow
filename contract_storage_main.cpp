#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/symbol.hpp>
#include <string>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

using namespace eosio;

class nft_contract : public eosio::contract {
public:
    using contract::contract;

    nft_contract(name receiver, name code, eosio::datastream<const char*> ds)
        : contract(receiver, code, ds), trans_table(receiver, receiver.value)
    {}

   [[eosio::action]]
void sendnft(const name& sender, const name& recipient, const std::vector<name>& approvers, uint64_t nft_id) {
    require_auth(sender);
    eosio::print("Sending NFT from ", sender, " to ", recipient, " with NFT ID ", nft_id);

    uint64_t transaction_id = current_time_point().time_since_epoch().count();

    std::string transaction_data = std::to_string(sender.value) + std::to_string(recipient.value) + std::to_string(nft_id);
    for (const auto& approver : approvers) {
        transaction_data += std::to_string(approver.value);
    }
    checksum256 transaction_hash = sha256(transaction_data.c_str(), transaction_data.size());

    trans_table.emplace(get_self(), [&](auto& row) {
        row.id = transaction_id;
        row.sender = sender;
        row.recipient = recipient;
        row.approvers = approvers;
        row.transaction_hash = transaction_hash;
        row.nft_id = nft_id;
    });

    for (const auto& approver : approvers) {
        eosio::action(
            eosio::permission_level{get_self(), eosio::name("active")},
            get_self(),
            eosio::name("notify"),
            std::make_tuple(sender, recipient, approver, transaction_id, transaction_hash)
        ).send();
    }
}

    [[eosio::action]]
void approve(const name& approver, uint64_t transaction_id, const checksum256& transaction_hash) {
        require_auth(approver);
         eosio::print("Approving transaction with ID ", transaction_id, " by ", approver);

        auto transaction_itr = trans_table.find(transaction_id);
        eosio::check(transaction_itr != trans_table.end(), "Transaction does not exist");
        eosio::check(transaction_itr->approved_by.find(approver) == transaction_itr->approved_by.end(), "Transaction already approved by this account");

        eosio::check(transaction_itr->transaction_hash == transaction_hash, "Transaction data has been tampered with");

        trans_table.modify(transaction_itr, get_self(), [&](auto& row) {
            row.approved_by.insert(approver);
        });

        if (transaction_itr->approved_by.size() == transaction_itr->approvers.size()) {
            eosio::action(
                eosio::permission_level{get_self(), eosio::name("active")},
                get_self(),
                eosio::name("transfernft"),
                std::make_tuple(transaction_itr->sender, transaction_itr->recipient, transaction_itr->nft_id)
            ).send();
        }
    }

    [[eosio::action]]
   void notify(const name& sender, const name& recipient, const name& approver, uint64_t transaction_id, const checksum256& transaction_hash) {
        require_auth(get_self());
        eosio::print("Notifying sender, recipient, and approver about transaction with ID ", transaction_id);
    }

private:
    struct [[eosio::table("txs")]] tx {
        uint64_t id;
        name sender;
        name recipient;
        std::vector<name> approvers;
        checksum256 transaction_hash;
        uint64_t nft_id;
        std::set<name> approved_by;
        uint64_t primary_key() const { return id; }
    };

    typedef eosio::multi_index<name("txs"), tx> txs;
    txs trans_table;
};

