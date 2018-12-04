# Intro

Decentral Bank is a software framework which allows a network of peers to partake in decentralized banking activity and monetary policy. The network is capable of issuing currency tokens whose purchasing power is maintained through a set of smart contracts included in the system. The mechanisms include but are not limited to the auctioning of bonds and shares and a collateralized reserve system. The Decentral Bank framework contains many of the cryptocurrency industry standard stability mechanisms for stablecoin systems. Users can fork the Decentral Bank software and create stablecoin networks which incorporate various monetary policy functions modeled after real-world banking and economic theories.

Decentral Bank Software is based on EOSIO technology for Turing-complete smart contract functionality, account-based transactions, and Byzantine consensus.  

## License
Decentral Bank software is released under the terms of the MIT license. Decentral Bank software is open source and free to use. The "Decentral Bank" repository is not a bank and does not engage in banking activity, transactions, or business. The software is released in an as-is basis and open to all collaborators.

## Getting Started 

Decentral Bank is built on EOSIO software with each stability mechanism modeled as its own smart contract. 

1. Download the latest EOSIO implementation at https://github.com/EOSIO/eos and follow instructions to set up a local test network (building from source is recommended).

2. Once a local single-node test network is set up with NODEOS creating blocks on the local machine, run the decentralbank_setup.sh script to begin setting up the decentral bank local network. This will setup the local network by creating the necessary EOSIO accounts, deploying the appropriate smart contracts, beginning price feed and monetary policy, and creating currency tokens. 