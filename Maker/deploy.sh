#! /bin/bash
### Usage: deploy [eosio contract to compile, omit file extensions like .cpp] 
### Assumption: you have created a contract account with same name as eosio contract

CONTRACT_NAME="$1"
CONTRACT_ENV="$2"

FILE_NAME_WASM="$CONTRACT_NAME.wasm"
FILE_NAME_ABI="$CONTRACT_NAME.abi"

### Functions
if [ "$CONTRACT_ENV" == "" ] && [ "$CONTRACT_NAME" != "" ]; then
    cleos set contract $CONTRACT_NAME . $FILE_NAME_WASM $FILE_NAME_ABI
elif [ "$CONTRACT_ENV" != "" ] && [ "$CONTRACT_NAME" != "" ]; then
    cleos --url https://jungle.eosio.cr:443 set contract $CONTRACT_NAME . $FILE_NAME_WASM $FILE_NAME_ABI
else
    echo "Usage: deploy [contract name] [environment]"
fi