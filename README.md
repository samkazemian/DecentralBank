# DecentralBank: Turing-complete currency and banking software 

Decentral bank software is free to use and open source. The software allows a network of peers to partake in decentralized banking activity and monetary policy such as issuing a currency token and keeping its purchasing power stable using a plethora of smart contracts included in the system such as bonds, shares, and collateralized reserves. The decentral bank network contains many of the cryptocurrency industry standard stability mechanisms for stablecoin systems. Using decentral bank software, users can create multiple stablecoin networks which use various monetary policy functions modeled after real-world banking and economic theories. 

A majority of the decentral bank software uses EOSIO technology for Turing-complete smart contract functionality, account-based transactions, and Byzantine consensus.  

## License
Decentral Bank software is released under the terms of the MIT license.

# Features

## Network consensus 
The Decentral Bank system is built using the EOSIO library for the distributed ledger networking layer. As such, the byzantine consensus mechanism of the system is delegated proof of stake system used in EOSIO. The SYS token of the network is used to vote on consensus forming nodes (called block producers). Block producers are rewarded in minted SYS tokens and transaction fees of UTXO tokens.

## Bonds
The bond contract creates bond tokens which are sold through an auction. When the currency needs to retract, bond tokens can be printed and auctioned for currency in a reverse Dutch auction every T time period. To increase retractionary monetary policy through the bond mechanism, the contract can increase the interest rate of the bond tokens by a certain denominated amount.

There can be two types of bonds issued with the contract: coupon bonds and maturity bonds. Coupon bonds pay out the holder a coupon rate in currency until the maturation of the bond. Maturity bonds do not pay out until the maturity date where the holder is issued a lump sum of the face value and interest rate. Bond tokens of the same type with the same maturation date and interest rate are fungible.

## Seigniorage Shares
The seigniorage shares contract prints new share tokens and auctions them for currency in a reverse Dutch auction every R time period. The name of the contract is a homage to Robert Sams' stablecoin mechanism system of the same name since share tokens function similarly here and represent "equity in the network."

All share tokens are fungible with one another. The more equity tokens that are printed and auctioned for currency, the more retractionary policy on the currency. Share tokens are used to vote for block producers as well as allocate the transaction fees of the network. They also have a right to printed currency tokens as the network expands. The share token is also the SYS token of the EOSIO software. 

## UTXO Based Token Contracts
The UTXO token contract is a standardized EOSIO smart contract which allows token transacting using only public-private key pairs without the need for an EOSIO account or system interaction. It is similar in feature to the Bitcoin transaction system. To move tokens in a UTXO based contract, an owner only needs the corresponding private key to the public key. A transaction fee is also paid to move the token. No account or EOSIO network interaction is required. 

## Transaction Fees
The transfer of currency tokens, bond tokens, and seigniorage share tokens on the decentral bank network can be paid in fees denominated in the token or by staking share tokens similar to standard EOSIO software. 

## Private Transactions
The Decentral Bank UTXO token is a fungible token which allows for Bitcoin-like pseudo-anonymity. It would be possible to integrate distributed ledger privacy features such as zero knowledge proofs which would allow for any user to exchange UTXO currency tokens without revealing their identity similar to zCash and Monero networks. 

# Roadmap

## Main network
Once the software is ready for first release and a sufficiently distributed community formed, the "global main network" will be launched by a community initiative. The decentral bank "world network" will be the global main network of the system and the first network launched using decentral bank software. It is a purely permissionless, jurisdictionless, and trustless network committed to the highest form of sovereign-grade decentralization and censorship-resistance issuing the world's first Turing-complete stablecoin currency.  
