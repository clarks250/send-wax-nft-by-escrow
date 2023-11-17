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

1. The contract uses authentication checks (require_auth) to manage access to actions.

2. The contract supports condition checks before executing key actions, such as choosing escrows and signing.

3. NFT transfer occurs using the "transfer" action of the "atomicassets" contract.

4. In case of errors or incorrect conditions, error messages are printed.

5. The "canceltx" action provides a mechanism for canceling transactions and returning assets to the sender.

6. The "clearnft" action efficiently clears all records in both the nft_table and escrow_table.


**How to use**

P.S. You can use WAX blockchain scanners, for example: https://testnet.waxblock.io/account/storagetest3 to work with transactions. Its simple, so I will explain how to use it by console:

1. Install the IDE and CDT from documentation: https://developers.eos.io/welcome/latest/index and https://github.com/AntelopeIO/cdt

2. Create a wallet by command:

cleos wallet create -n <name of wallet> --file <name of file>

3. Import your wallet with your private key, if you have not, create an account and get test tokens on https://waxsweden.org/testnet/

cleos wallet import -n <name of wallet>

4. Transfer NFT to the contract account

For each TX you should use API, you can find it on https://waxsweden.org/testnet/.

cleos -u YOUR_API_LINK push action atomicassets transfer '["sender_account_public_key", "contract_account_public_key", [sender_asset_id], "memo"]' -p sender_account_public_key@active

5. Choose escrows for your TX

cleos -u YOUR_API_LINK push action storagetest3 choseescrows '["sender_account_public_key", sender_asset_id, ["escrow_account_public_key1","escrow_account_public_key2"], "receive_account_public_key", 1]' -p escrow_account_public_key@active

6. Sign TX for escrows

cleos -u YOUR_API_LINK push action storagetest3 escrowsign '["sender_account_public_key", "receive_account_public_key", asset_id , "escrow_account_public_key"]' -p escrow_account_public_key@active

7. Cancel TX for sender

cleos -u YOUR_API_LINK push action storagetest3 canceltx '["sender_account_public_key", "receive_account_public_key", sender_account_asset_id]' -p sender_account_public_key@active

8. Clear Tables TX (ONLY WITH CONTRACT PRIVATE KEY)

cleos -u YOUR_API_LINK push action contract_account_public_key clearnft  -p contract_account_public_key@active