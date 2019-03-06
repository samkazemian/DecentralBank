#! /bin/bash
### Usage: compile [eosio file to compile, omit .cpp] 
### Constants
FILE_TO_COMPILE="$1"
FILE_NAME_CPP="$FILE_TO_COMPILE.cpp"
FILE_NAME_WASM="$FILE_TO_COMPILE.wasm"
FILE_NAME_ABI="$FILE_TO_COMPILE.abi"
### Functions
if [ "$FILE_TO_COMPILE" != "" ]; then
    eosio-cpp -o $FILE_NAME_WASM $FILE_NAME_CPP
    eosio-abigen $FILE_NAME_CPP --contract=$FILE_TO_COMPILE --output=$FILE_NAME_ABI
else
    echo "Usage: compile [contract file named same as contract]"
fi
