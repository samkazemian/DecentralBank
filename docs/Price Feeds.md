## Price Feeds

In order for Decentral Bank currency to track the price of an external asset (ie: US Dollar), market data must be brought on chain. The data is imported each block by consensus forming nodes. 

Each block producing node adds price data within each block they produce. The data is then weighted using an exponential moving average (EMA) and incorporated in the current global state table of the eosio.system contract. 

The multi-index table in the system contract is updated each block with the EMA of the price of each asset in relation to the exogenous asset (US Dollar). To access this table from cleos, simply:

`cleos get table eosio eosio global`



### Expanding Price Feeds

As more price feeds are needed in the future, it's recommended to simply add additional rows to the global_state table within the system contract.