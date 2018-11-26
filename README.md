# Decentral Bank: Turing-Complete Currency and Banking Software 

Decentral Bank is a software framework which allows a network of peers to partake in decentralized banking activity and monetary policy. The network is capable of issuing currency tokens whose purchasing power is maintained through a set of smart contracts included in the system. The mechanisms include but are not limited to the auctioning of bonds and shares and a collateralized reserve system. The Decentral Bank framework contains many of the cryptocurrency industry standard stability mechanisms for stablecoin systems. Users can fork the Decentral Bank software and create stablecoin networks which incorporate various monetary policy functions modeled after real-world banking and economic theories.

Decentral Bank Software is based on EOSIO technology for Turing-complete smart contract functionality, account-based transactions, and Byzantine consensus.  

## License
Decentral Bank software is released under the terms of the MIT license. Decentral Bank software is open source and free to use. The "Decentral Bank" repository is not a bank and does not engage in banking activity, transactions, or business. The software is released in an as-is basis and open to all collaborators.


# Features

## Network Consensus 
The Decentral Bank system is built using the EOSIO library for the distributed ledger networking layer. As such, the byzantine consensus mechanism of the system is delegated proof of stake system used in EOSIO. The SYS token of the network is used to vote on consensus forming nodes (called block producers). Block producers are rewarded in minted SYS tokens and transaction fees of UTXO tokens.

## Bonds
The bond contract creates bond tokens which are sold through an auction. When the currency needs to retract, bond tokens can be printed and auctioned for currency tokens in a reverse Dutch auction every T time period. To incentivize further retraction through the bond mechanism, the contract can increase the interest rate of the bond tokens by a certain denominated amount.

There can be two types of bonds issued with the contract: coupon bonds and maturity bonds. Coupon bonds pay out the holder a coupon rate in currency until the maturation of the bond. Maturity bonds do not pay out until the maturity date, where the holder is issued a lump sum of the face value and interest rate. Bond tokens of the same type and with the same maturation date and interest rate are fungible.

## Seigniorage Shares
The seigniorage shares contract prints new share tokens and auctions them for currency in a reverse Dutch auction every R time period. The name of the contract is a homage to Robert Sams' stablecoin mechanism system of the same name since share tokens function similarly here and represent "equity in the network".

All share tokens are fungible with one another. The more equity tokens that are printed and auctioned for currency, the greater the retraction of the currency. Share tokens are used to vote for block producers as well as allocate the transaction fees of the network. They also have a right to printed currency tokens as the network expands. The share token is also the SYS token of the EOSIO software.

## Reserve
The collateralized token reserve allows for the issued currency tokens to be backed by cryptographic assets across multiple chains. The reserve is controlled by multi-signature authorization of network consensus nodes/block producers. When new currency is printed by the network and not needed to pay bond holders, seigniorage share holders, and other users, it is placed in the reserve to auction off for various crypto assets. The types of assets held in the reserve, and their proportion, are decided by network consensus nodes/block producers.  

### Basket Reserve
Basketing multiple crypto assets across multiple chains is important to keep volatility low in assets held. The network's basket smart contract allows conesnsus nodes/block producers to set basket ratios and targets for the crypto assets held in the reserve.

### Depository Reserve
Users can make time-restricted deposits into the network, through which they can accrue transaction fees collected over a set number of block periods. Transaction fees are distributed to depositors pro-rata.

## UTXO Based Token Contracts
The UTXO token contract is a standardized EOSIO smart contract which allows token transacting using only public-private key pairs without the need for an EOSIO account or system action. It is similar in feature to the Bitcoin transaction system. To move tokens in a UTXO based contract, an owner only needs the corresponding private key to the public key. A transaction fee is also paid to move the token. No account or EOSIO network interaction is required. The Decentral Bank network will develop and employ the UTXO based token contracts for its stable currency unit. 

## Transaction Fees
The transfer of currency tokens, bond tokens, and seigniorage share tokens on the Decentral Bank Network can be paid in fees denominated in the token or by staking share tokens similar to standard EOSIO software. Fees can be modified to adjust incentives to better match the network's needs.

## Private Transactions
The Decentral Bank UTXO token is a fungible token which allows for Bitcoin-like pseudo-anonymity. It would be possible to integrate distributed ledger privacy features such as zero knowledge proofs which would allow for any user to exchange UTXO currency tokens without revealing their identity similar to zCash and Monero networks. 

# Roadmap

## Main network
Once the software is ready for first release and a sufficiently distributed community formed, the "global main network" will be launched by a community initiative. The decentral bank "world network" will be the global main network of the system and the first network launched using decentral bank software. It is a purely permissionless, jurisdictionless, and trustless network committed to the highest form of sovereign-grade decentralization and censorship-resistance issuing the world's first Turing-complete stablecoin currency.  
