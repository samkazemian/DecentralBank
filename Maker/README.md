
# Future note: some of the information herein should be apt for 
# reorganization into the relevant ricardian clauses of the contract ABI

If you'd like your cdp to dip below the agreed upon liqidation ratio
of your cdp's type, via one of the relevant actions (draw or bail),
then TOO BAD because you won't be permitted (you'll hit an assertion).

If you'd like to liquidate your own CDP in the event of significant price slippage,
then TOO BAD because that would let you print free money and we won't have that kind
of behavior here (unless you collude with another account).

# Differences between CDPeos and CDPether (aka MakerDAO)

The major discrepancy between the two protocols is in how
liquidation auctions are handled. We make add the constraint
where the owner of a cdp cannot liquify their own cdp.

To provide for better liquidity, Maker uses a simultaneous (parallel) 
auctions of four types:

#1 Dai (stable token) bids for collateral
#2 Reverse dutch of the above (collateral bids for dai) in the event of 
#3 Dai bids for Maker (voting token) in the event of balance shortage in #1 (not enough Dai was raised)
#4 Maker bids for Dai in the event of balance surplus in #1 (too much Dai was raised) 

#2 is omitted entirely from our protocol and the rest of the phases are 
execute in sequence rather than in parallel. Unlike in Ethereum where the primary constraint is 
speed and gas, the main constraint in EOS is RAM so our approach was taken from an angle of more space
efficient implementation, and a simpler view overall. 

On that last note, we also omit the calculation of stability fees on an APR basis on favor of a fixed percentage fee
upon every draw action executed on a cdp. Moreover, our voting and referendum procedure has a common sense nature.
Any account may create a proposal, and as many varying proposals as desired,
but we do constrain proposals per CDP type. Only one proposal may be active (in voting) per CDP type (new or modification of existing).
Voters may vote with their tokens as much as they please (a la Maker) and even post two postions simultaneously (both for and against).

All voting tokens are refunded to voters upon the expiration of the voting period for a proposal, and if there is a tie between total 
for and against positions, another voting period is schedule to take place immediately.
Global settlement is subjected to popular vote like any CDP change or creation proposal, rather than designated 
to a select group of Keepers as in MakerDAO.

Global constants are the designated accounts that provide price feeds, the voting period of 2 weeks for proposals, and the minimum 
age of 5 minutes (how recent) for price data that is considered acceptable. 
