#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "USAGE: decentralbank_setup.sh <contract_name> <account_name> from within the contract directory. Run after a eosio implementation installed from source. This sets up decentral bank system."
  exit 1
fi

CONTRACT=$1
ACCOUNT=$2
eosio-cpp -abigen ${CONTRACT}.cpp -o ${CONTRACT}.wasm
cleos set contract ${ACCOUNT} ../${CONTRACT} -p ${ACCOUNT}@active
