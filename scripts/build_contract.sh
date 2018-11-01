#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "USAGE: build_contract.sh <contract_name> from within the contract directory"
  exit 1
fi

CONTRACT=$1
eosiocpp -o ${CONTRACT}.wast ${CONTRACT}.cpp &&
eosiocpp -g ${CONTRACT}.abi ${CONTRACT}.hpp
