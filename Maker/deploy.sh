#! /bin/bash
### Usage: deploy [eosio contract to compile, omit file extensions like .cpp] [contract account name]
### Assumption: you have created a contract account with same name as eosio contract
### Constants

CONTRACT_NAME="$1"
FILE_NAME_ABI="$CONTRACT_NAME.abi"
FILE_NAME_WASM="$CONTRACT_NAME.wasm"
### Functions
if [ "$CONTRACT_NAME" != "" ]; then
    cleos set contract $CONTRACT_NAME . $FILE_NAME_WASM $FILE_NAME_ABI
else
    echo "Usage: deploy [contract name]"
fi
