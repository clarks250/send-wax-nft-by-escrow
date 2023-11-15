#include <eosio/eosio.hpp>
#include <vector>

using namespace eosio;

CONTRACT nft_contract : public eosio::contract {
public:
    using contract::contract;

    // Define escrows and min_escrows at the beginning of the contract
    uint32_t min_escrows;

    [[eosio::on_notify("atomicassets::transfer")]]
    void receiveasset(name from, name to, std::vector<uint64_t> asset_ids, std::string memo) {
        if (to != get_self()) {
            print("Invalid recipient. Ignoring the notification.");
            return;
        }

        check(asset_ids.size() == 1, "Error: Number of tokens must be exactly 1");

        for (const auto& asset_id : asset_ids) {
            auto existing_nft = nft_table_inst.find(asset_id);

            print("Received asset. From: ", from, ", To: ", to, ", Asset ID: ", asset_id, ", Memo: ", memo);

            if (existing_nft != nft_table_inst.end() && existing_nft->sender == from) {
                print("Existing NFT found. Modifying...");

                print("Existing NFT Sender: ", existing_nft->sender);
                print("Existing NFT Recipient: ", existing_nft->recipient);
                nft_table_inst.modify(existing_nft, get_self(), [&](auto &row) {
                    row.recipient = to;
                    row.min_escrows = min_escrows;
                });

                print("NFT successfully modified. Sender: ", from, ", Asset ID: ", asset_id);
                print("Modified NFT. Recipient: ", existing_nft->recipient, ", Escrows: ", "Min Escrows: ", existing_nft->min_escrows);
            } else {
                print("Creating new NFT entry...");

                // Создайте новый объект NFT и добавьте его в таблицу
                nft_table_inst.emplace(get_self(), [&](auto &row) {
                    row.sender = from;
                    row.recipient = to;
                    row.min_escrows = min_escrows;
                    row.signed_escrows_count = 0;  // Здесь вы можете установить начальное значение по вашему усмотрению
                    row.asset_id = asset_id;
                });

                print("New NFT entry created. Sender: ", from, ", Asset ID: ", asset_id);
            }
        }
    }

private:
    struct [[eosio::table]] nft {
        name sender;
        name recipient;
        uint32_t min_escrows;
        uint32_t signed_escrows_count;
        uint64_t asset_id;

        uint64_t primary_key() const { return asset_id; }
    };

    typedef multi_index<"nft"_n, nft> nft_table;
    nft_table nft_table_inst{get_self(), get_self().value};
};
