##Bonds

The bond contract creates bond tokens which are sold through an auction. When the currency needs to retract, bond tokens can be printed and auctioned for currency tokens in a reverse Dutch auction every T time period. To incentivize further retraction through the bond mechanism, the contract can increase the interest rate of the bond tokens by a certain denominated amount.

There can be two types of bonds issued with the contract: coupon bonds and maturity bonds. Coupon bonds pay out the holder a coupon rate in currency until the maturation of the bond. Maturity bonds do not pay out until the maturity date, where the holder is issued a lump sum of the value and interest rate. Bond tokens of the same type and with the same maturation date and interest rate are fungible.

Each bond token has a face value of exactly 1 currency token. 

##Getting Started with Bonds

The bond smart contract creates bond tokens, pays out currency based on coupon rates and maturity dates, and auctions off newly printed bond tokens. 

### Coupon Bonds (Symbol: DCB)

Coupon bonds are tokens which pay the holder an interest rate (coupon rate) of currency per fixed number of blocks until maturity date. The CouponPaymentDate is a block increment that interest payments are paid to the holder of the bond. 

Properties

`MaturityDate` (in blocks)

`IssueDate` (in block number)

`CouponRate` (as percentage)

`CouponPaymentDate` (in blocks)


### Maturity Bonds (Symbol: DMB)

Maturity bonds are tokens which pay the holder the entirety of the value plus interest rate (CouponRate) at the maturity date and nothing before. 

Properties

`MaturityDate` (in blocks)

`IssueDate` (in block number)

`CouponRate` (as percentage)





