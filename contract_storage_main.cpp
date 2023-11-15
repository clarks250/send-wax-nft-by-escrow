#include <eosio/eosio.hpp>
#include <vector>

using namespace eosio;

CONTRACT nft_contract : public eosio::contract {
public:
    using contract::contract;

    // Define escrows and min_escrows at the beginning of the contract
    std::vector<name> escrows;
    uint32_t min_escrows;

    [[eosio::on_notify("atomicassets::transfer")]]
    void receiveasset(name from, name to, std::vector<uint64_t> asset_ids, std::string memo) {
        if (to != get_self())
            return;

        check(asset_ids.size() == 1, "Error: Number of tokens must be exactly 1");

        for (const auto& asset_id : asset_ids) {
            auto existing_nft = nft_table_inst.find(asset_id);

            if (existing_nft != nft_table_inst.end() && existing_nft->sender == from) {
                nft_table_inst.modify(existing_nft, get_self(), [&](auto &row) {
                    row.recipient = to;
                    row.escrows = escrows;
                    row.min_escrows = min_escrows;
                });

                print("NFT successfully added to the table. Sender: ", from, ", Asset ID: ", asset_id);
            } else {
                print("Error: NFT not found or sender does not match.");
            }
        }
    }

    ACTION updateinfo(name sender, uint64_t asset_id, name recipient, std::vector<name> escrows, uint32_t min_escrows) {
        require_auth(sender);

        nft_table nft_table_inst(get_self(), get_self().value);
        auto existing_nft = nft_table_inst.find(asset_id);

        check(existing_nft != nft_table_inst.end(), "Error: NFT not found");
        check(existing_nft->sender == sender, "Error: Sender does not match");

        nft_table_inst.modify(existing_nft, get_self(), [&](auto &row) {
            row.recipient = recipient;
            row.escrows = escrows;
            row.min_escrows = min_escrows;
        });

        print("Transaction information successfully updated. Asset ID: ", asset_id);
    }

    ACTION signescrow(name escrow, uint64_t asset_id) {
        require_auth(escrow);

        nft_table nft_table_inst(get_self(), get_self().value);
        auto existing_nft = nft_table_inst.find(asset_id);

        check(existing_nft != nft_table_inst.end(), "Error: NFT not found");

        nft_table_inst.modify(existing_nft, get_self(), [&](auto &row) {
            // Check that escrow is not already signed
            auto it = std::find(row.signed_escrows.begin(), row.signed_escrows.end(), escrow);
            check(it == row.signed_escrows.end(), "Error: Escrow already signed");

            // Sign the escrow
            row.signed_escrows.push_back(escrow);
            row.signed_escrows_count++;

            // Check if the minimum number of signatures is reached
            if (row.signed_escrows_count >= row.min_escrows) {
                // Perform action for automatic transfer
                // In the example, sending a transaction to atomicassets::transfer
                action(
                    permission_level{get_self(), "active"_n},
                    "atomicassets"_n,
                    "transfer"_n,
                    std::make_tuple(row.sender, row.recipient, row.asset_id, "")
                ).send();

                // Reset signatures for the next transactions
                row.signed_escrows.clear();
                row.signed_escrows_count = 0;
            }
        });

        print("Escrow successfully signed. Asset ID: ", asset_id);
    }

private:
    struct [[eosio::table]] nft {
        name sender;
        name recipient;
        std::vector<name> escrows;
        std::vector<name> signed_escrows;
        uint32_t min_escrows;
        uint32_t signed_escrows_count;
        uint64_t asset_id;

        uint64_t primary_key() const { return asset_id; }
    };

    typedef multi_index<"nft"_n, nft> nft_table;
    nft_table nft_table_inst{get_self(), get_self().value};
};
