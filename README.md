**Typical Contract Actions:**

***1.choseescrows (possibly a typo, should be chooseescrows):*** This action allows choosing escrow agents for a specific NFT. You need to specify the sender (from), a unique asset identifier (asset_id), a list of escrow agents (escrows), the recipient (recipient), and the minimum number of escrows (min_escrows). If conditions are met, a new entry is created in the escrow_table, and the associated entry in the nft_table is deleted.

***2.escrowsign:*** This action allows escrow agents to sign the agreement. When the number of signatures reaches or exceeds the minimum number of escrows (min_escrows), the asset is transferred from the sender to the recipient using the "transfer" action of the "atomicassets" contract.

***3.receiveasset:*** This action is triggered when receiving an NFT from another contract (in this case, "atomicassets::transfer"). It checks if the notification is intended for this contract and then creates a new entry in the nft_table if the NFT with the given asset_id is not present in the table.

***4.canceltx:*** This action allows the sender to cancel a transaction by retrieving the asset_id from either the escrow_table or nft_table and returning it to the sender using the "transfer" action of the "atomicassets" contract.

***5.clearnft:*** This action clears all records in the nft_table and escrow_table. It is crucial to note that this action requires authentication from the contract itself.

**Tables:**

***1.nft_table:*** This table stores information about NFTs. Each record contains the sender (sender) and the unique asset identifier (asset_id).

***2.escrow_table:*** This table stores information about escrows. Each record includes the sender (sender), recipient (recipient), a list of escrow agents (escrows), the minimum number of escrows (min_escrows), the number of signed escrows (signed_escrows_count), and the unique asset identifier (asset_id).

**Key Points:**

1.The contract uses authentication checks (require_auth) to manage access to actions.

2.The contract supports condition checks before executing key actions, such as choosing escrows and signing.

3.NFT transfer occurs using the "transfer" action of the "atomicassets" contract.

4.In case of errors or incorrect conditions, error messages are printed.

5.The "canceltx" action provides a mechanism for canceling transactions and returning assets to the sender.

6.The "clearnft" action efficiently clears all records in both the nft_table and escrow_table.