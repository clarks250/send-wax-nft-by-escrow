#include <eosio/eosio.hpp>
#include <vector>
#include <algorithm>

using namespace eosio;

CONTRACT nft_contract : public eosio::contract
{
public:
    using contract::contract;

    // Define escrows and min_escrows at the beginning of the contract
    uint32_t min_escrows;
    std::vector<name> escrows;

    [[eosio::on_notify("atomicassets::transfer")]] void receiveasset(name from, name to, std::vector<uint64_t> asset_ids, std::string memo)
    {
        // Check if the notification is intended for this contract
        if (to != get_self())
        {
            print("Invalid recipient. Ignoring the notification.");
            return;
        }

        // Ensure that the number of tokens is exactly 1
        check(asset_ids.size() == 1, "Error: Number of tokens must be exactly 1");

        // Process each asset_id in the notification
        for (const auto &asset_id : asset_ids)
        {
            // Find the existing NFT record by asset_id
            auto existing_nft = nft_table_inst.find(asset_id);

            // Print information about the received asset
            print("Received asset. From: ", from, ", To: ", to, ", Asset ID: ", asset_id, ", Memo: ", memo);

            // Check if the NFT with the given asset_id is already in storage
            if (existing_nft != nft_table_inst.end() && existing_nft->sender == from)
            {
                print("This nft id already in storage");
            }
            else
            {
                // Create a new NFT entry
                print("Creating new NFT entry...");

                nft_table_inst.emplace(get_self(), [&](auto &row)
                                       {
                    row.sender = from;
                    row.asset_id = asset_id; });
                // Print information about the new NFT entry
                print("New NFT entry created. Sender: ", from, ", Asset ID: ", asset_id);
            }
        }
    }

    // Action to choose escrows for a given NFT
    ACTION choseescrows(name from, uint64_t asset_id, std::vector<name> escrows, name recipient, uint32_t min_escrows)
    {
        // Require authentication from the caller
        require_auth(from);

        // Alias for the sender's name
        name sender = from;

        // Find existing NFT record by asset_id
        auto existing_nft = nft_table_inst.find(asset_id);
        if (existing_nft == nft_table_inst.end() || existing_nft->sender != sender || existing_nft->asset_id != asset_id)
        {
            print("Error: NFT record not found or sender/asset_id does not match for the given asset_id.", sender);
            return;
        }
        else if (min_escrows <= 0 || min_escrows >= 100)
        {
            print("Error: You should select from 1 to 99 escrows", min_escrows);
            return;
        }
        else if (min_escrows > escrows.size())
        {
            print("Error: Min escrows more than the number of escrows", escrows.size());
            return;
        }
        else
        {
            // Check escrows for uniques
            std::set<name> uniqueEscrows(escrows.begin(), escrows.end());
            if (uniqueEscrows.size() != escrows.size())
            {
                print("Error: Duplicate escrow accounts found.");
                return;
            }
            // Check escrows for existing
            for (const auto &escrowAccount : escrows)
            {
                if (!is_account(escrowAccount))
                {
                    print("Error: Account ", escrowAccount, " does not exist.");
                    return;
                }
            }

            // Find existing escrow record by asset_id
            auto existing_escrow = escrow_table_inst.find(asset_id);

            if (existing_escrow != escrow_table_inst.end())
            {
                print("Error: Escrow info already exists for the given asset_id. Use a different action for modification.");
            }
            else
            {
                print("Creating a new escrow entry...");

                escrow_table_inst.emplace(get_self(), [&](auto &row)
                                          {
                    row.sender = sender;
                    row.recipient = recipient;
                    row.escrows = escrows;
                    row.min_escrows = min_escrows;
                    row.signed_escrows_count = 0;
                    row.asset_id = asset_id; });

                nft_table_inst.erase(existing_nft);
                print("NFT record deleted for asset_id: ", asset_id);

                print("New escrow entry created. Sender: ", sender, ", Asset ID: ", asset_id);
            }
        }
    }

    [[eosio::action]] void escrowsign(name sender, name recipient, uint64_t asset_id, name escrow)
    {
        require_auth(escrow);

        // Find the escrow entry by asset_id
        auto escrow_index = escrow_table_inst.find(asset_id);
        if (escrow_index == escrow_table_inst.end())
        {
            // Escrow entry not found, print an error message
            print("Escrow not found");
            return;
        }
        else if (escrow_index->sender != sender || escrow_index->recipient != recipient || escrow_index->asset_id != asset_id)
        {
            // Check if sender, recipient, or asset_id mismatch, print an error message
            print("Invalid sender, recipient, or asset_id");
            return;
        }
        else
        {

            auto foundEscrow = std::find(escrow_index->escrows.begin(), escrow_index->escrows.end(), escrow);
            if (foundEscrow == escrow_index->escrows.end())
            {
                print("Error: Escrow account not found in the list.");
                return;
            }
            else
            {
                // Create a copy of the escrows vector and remove the escrow from the list
                auto new_escrows = escrow_index->escrows;
                new_escrows.erase(std::remove(new_escrows.begin(), new_escrows.end(), escrow), new_escrows.end());

                // Modify the escrow entry in the table
                escrow_table_inst.modify(escrow_index, get_self(), [&](auto &row)
                                         {
            row.escrows = new_escrows;
            row.signed_escrows_count += 1; });

                // Check if the minimum number of escrows is reached
                if (escrow_index->min_escrows <= escrow_index->signed_escrows_count)
                {
                    // Transfer the asset using the "atomicassets" contract
                    action(
                        permission_level{get_self(), "active"_n},
                        "atomicassets"_n,
                        "transfer"_n,
                        std::make_tuple(get_self(), recipient, std::vector<uint64_t>{asset_id}, std::string("")))
                        .send();
                    escrow_table_inst.modify(escrow_index, get_self(), [&](auto &row)
                                             {
        // Modify the escrow entry again
        row.escrows = new_escrows;
        row.signed_escrows_count += 1; });

                    // Erase the escrow entry from the table
                    escrow_table_inst.erase(escrow_index);
                }
            }
        }
    }

    [[eosio::action]] void canceltx(name sender, name recipient, uint64_t asset_id)
    {
        require_auth(sender);

        // Check in escrow table
        auto escrow_index = escrow_table_inst.find(asset_id);
        if (escrow_index != escrow_table_inst.end() &&
            escrow_index->sender == sender &&
            escrow_index->recipient == recipient &&
            escrow_index->asset_id == asset_id)
        {

            // Send asset_id back to sender
            action(
                permission_level{get_self(), "active"_n},
                "atomicassets"_n,
                "transfer"_n,
                std::make_tuple(get_self(), sender, std::vector<uint64_t>{asset_id}, std::string("")))
                .send();

            // Delete from escrowinfo
            escrow_table_inst.erase(escrow_index);
            print("Transaction canceled. Asset ID ", asset_id, " returned to sender ", sender);
            return;
        }

        // If there was no info in escrow table, check in nft table
        auto nft_index = nft_table_inst.find(asset_id);
        if (nft_index != nft_table_inst.end() &&
            nft_index->sender == sender &&
            nft_index->asset_id == asset_id)
        {

            // Send asset_id back to sender
            action(
                permission_level{get_self(), "active"_n},
                "atomicassets"_n,
                "transfer"_n,
                std::make_tuple(get_self(), sender, std::vector<uint64_t>{asset_id}, std::string("")))
                .send();

            // Delete from nft table
            nft_table_inst.erase(nft_index);
            print("Transaction canceled. Asset ID ", asset_id, " returned to sender ", sender);
        }
        else
        {
            print("Error: Escrow not found in either table for the given asset_id.");
            return;
        }
    }

    ACTION clearnft()
    {
        require_auth(get_self());

        // Iterate through all records in the table and remove them
        auto itr = nft_table_inst.begin();
        while (itr != nft_table_inst.end())
        {
            itr = nft_table_inst.erase(itr);
        }

        // Iterate through all records in the escrow table and remove them
        auto itresc = escrow_table_inst.begin();
        while (itresc != escrow_table_inst.end())
        {
            itresc = escrow_table_inst.erase(itresc);
        }

        print("All records in the nft table have been cleared.");
    }

private:
    struct [[eosio::table]] nft
    {
        name sender;
        uint64_t asset_id;

        // Define the primary key for the NFT table
        uint64_t primary_key() const { return asset_id; }
    };

    // Define the multi-index table for NFTs
    typedef multi_index<"nft"_n, nft> nft_table;
    nft_table nft_table_inst{get_self(), get_self().value};

    struct [[eosio::table]] escrowinfo
    {
        name sender;
        name recipient;
        std::vector<name> escrows;
        uint32_t min_escrows;
        uint32_t signed_escrows_count;
        uint64_t asset_id;

        // Define the primary key for the escrow table
        uint64_t primary_key() const { return asset_id; }
    };

    // Define the multi-index table for escrow information
    typedef multi_index<"escrow"_n, escrowinfo> escrow_table;
    escrow_table escrow_table_inst{get_self(), get_self().value};
};