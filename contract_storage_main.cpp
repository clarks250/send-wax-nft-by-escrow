#include <eosio/eosio.hpp>
#include <vector>

using namespace eosio;

CONTRACT nft_contract : public eosio::contract
{
public:
    using contract::contract;

    // Define escrows and min_escrows at the beginning of the contract
    uint32_t min_escrows;
    std::vector<name> escrows;

    ACTION choseescrows(name from, uint64_t asset_id, std::vector<name> escrows, name recipient, uint32_t min_escrows)
    {
        require_auth(from);
        name sender = from;
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
        else if (min_escrows > size(escrows))
        {
            print("Error: Min escrows more then number of escrows", size(escrows));
            return;
        }
        else
        {
            auto existing_escrow = escrow_table_inst.find(asset_id);

            if (existing_escrow != escrow_table_inst.end())
            {
                print("Error: Escrow info already exists for the given asset_id. Use a different action for modification.");
            }
            else
            {
                print("Creating new escrow entry...");

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

        auto escrow_index = escrow_table_inst.find(asset_id);
        if (escrow_index == escrow_table_inst.end())
        {
            print("Escrow not found");
            return;
        }

        else if (escrow_index->sender != sender)
        {
            print("Invalid sender");
            return;
        }

        else if (escrow_index->recipient != recipient)
        {
            print("Invalid recipient");
            return;
        }

        else if (escrow_index->asset_id != asset_id)
        {
            print("Invalid asset_id");
            return;
        }

        else
        {

            auto escrow_itr = std::find(escrow_index->escrows.begin(), escrow_index->escrows.end(), escrow);
            if (escrow_itr == escrow_index->escrows.end())
            {
                print("Escrow not found in the list");
                return;
            }

            else
            {

                // Удаляем существующую запись
                escrow_table_inst.erase(escrow_index);

                // Создаем новую запись с обновленными значениями
                escrow_table_inst.emplace(get_self(), [&](auto &row)
                                          {
        row.sender = sender;
        row.recipient = recipient;
        row.escrows = escrow_index->escrows;
        row.min_escrows = escrow_index->min_escrows;
        row.signed_escrows_count = escrow_index->signed_escrows_count + 1;
        row.asset_id = asset_id; });
            }
        }

        //  if (escrow_index->min_escrows >= escrow_index->signed_escrows_count)
        // {
        //     action(
        //         permission_level{get_self(), "active"_n},
        //         "atomicassets"_n,
        //         "transfer"_n,
        //         std::make_tuple(get_self(), recipient , asset_id, sender))
        //         .send();
        // }
    }

    [[eosio::on_notify("atomicassets::transfer")]] void receiveasset(name from, name to, std::vector<uint64_t> asset_ids, std::string memo)
    {
        if (to != get_self())
        {
            print("Invalid recipient. Ignoring the notification.");
            return;
        }

        check(asset_ids.size() == 1, "Error: Number of tokens must be exactly 1");

        for (const auto &asset_id : asset_ids)
        {
            auto existing_nft = nft_table_inst.find(asset_id);

            print("Received asset. From: ", from, ", To: ", to, ", Asset ID: ", asset_id, ", Memo: ", memo);

            if (existing_nft != nft_table_inst.end() && existing_nft->sender == from)
            {
                print("This nft id already in storage");
            }
            else
            {
                print("Creating new NFT entry...");

                nft_table_inst.emplace(get_self(), [&](auto &row)
                                       {
                    row.sender = from;
                    row.asset_id = asset_id; });

                print("New NFT entry created. Sender: ", from, ", Asset ID: ", asset_id);
            }
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

        uint64_t primary_key() const { return asset_id; }
    };

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
        uint64_t primary_key() const { return asset_id; }
    };

    typedef multi_index<"escrow"_n, escrowinfo> escrow_table;
    escrow_table escrow_table_inst{get_self(), get_self().value};
};