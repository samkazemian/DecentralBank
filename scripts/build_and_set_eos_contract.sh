#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "USAGE: build_and_set_eos_contract.sh <contract_name> <account_name> from within the contract directory"
  exit 1
fi

CONTRACT=$1
ACCOUNT=$2
eosiocpp -o ${CONTRACT}.wast ${CONTRACT}.cpp &&
eosiocpp -g ${CONTRACT}.abi ${CONTRACT}.hpp &&
cleos set contract ${ACCOUNT} ../${CONTRACT}
