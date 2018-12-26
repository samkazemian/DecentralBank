##Bonds

The bond contract creates bond tokens which are sold through an auction. When the currency needs to retract, bond tokens can be printed and auctioned for currency tokens in a reverse Dutch auction every T time period. To incentivize further retraction through the bond mechanism, the contract can increase the interest rate of the bond tokens by a certain denominated amount.

Currently, there is a single type of bonds issued with the contract: maturity bonds. Maturity bonds do not pay out until the maturity date, where the holder is issued a lump sum of the value and interest rate. This means that bonds with the same maturity date and same interest rate are fungible allowing bond markets to form easily and exchanges to list Decentral Bank Bonds quickly with little overhead. 

Each bond token has a face value of exactly 1 currency token. 

##Getting Started with Bonds

The bond smart contract creates bond tokens, pays out currency based on maturity dates and interest rate, and auctions off newly printed bond tokens. 


### Maturity Bonds (Symbol: DMB)

Maturity bonds are tokens which pay the holder the entirety of the value plus interest rate (CouponRate) at the maturity date and nothing before. 

Properties

`MaturityDate` (in blocks)

`IssueDate` (in block number)

`CouponRate` (as percentage)

## Theory 

The reasoning behind having a single type of bond is to increase the fungibility between bond tokens so that markets can form. Any bond with the same MaturityDate and CouponRate should theoretically have the same price and be interchangeable. 

Bonds issued at later block numbers with the same CouponRate and MaturityDate should still be interchangeable since they do not pay out until the MaturityDate. This creates windows where all bonds of the same MaturityDate and CouponRate are traded in the same markets and can be continually issued/printed by the bond contract. 
