## Network Consensus

The network consensus rules of the network are the same as the current working release of the EOSIO software. A 'delegated proof of stake' system is used to form a pool of 21 'block producers' (consensus forming nodes). 

### Consensus Costs and Payments 

The default settings for the Decentral Bank Network pay out block producers in currency tokens. This can be modified for different implementations or network upgrades. Block producing nodes also earn the transaction fees for processing UTXO transfers. 


### Consensus Cost Theory

There is some total network consensus cost, C, which is the total cost required for network participants to reach consensus on the next state. C can be paid out to consensus forming nodes/participants in any endogenous tokens of the Decentral Bank Network. Decentral Bank is one of few networks which produces multiple endogenous tokens (currency, bonds, shares). Thus, there's a choice what to pay out to consensus forming nodes for their costs while on other networks there is only 1 choice of network token (BTC, ETH, etc).

Since C is deterministic, calculating it and accounting for C in the Decentral Bank currency emission curve is quite easy for the network to manage. In this way, C of the network is planned into the monetary policy of the system so that consensus costs are simply an accounting metric within the network itself. This is akin to a central bank accounting for the money it must print to pay its own employees on payroll.

Secondly, paying consensus forming nodes stable, sound money is likely the preferred type of compensation than holding speculative assets. If it is not, then node operators could simply exchange their currency for their prferred asset (which can be done on-chain in the network).

### Consensus Methods

The current Decentral Bank implementation uses EOSIO software and delegated proof of stake to choose 21 consensus forming nodes for block production. 

Once zero-knowledge proof implementation and STARK libraries are widespread, it is possible and necessary to shift the consensus protocol of Decentral Bank to a more distributed mechanism to increase the number of consensus forming nodes and particpants without lowering transaction throughput. Given this, there should be a social expectation for all network participants to pursue a network upgrade path to maximal node participation in the consensus process. Network participants include the following groups, but not limited to: 1. block producer nodes 2. share token holders 3. bond token holders and 4. users of currency


### Decentral Bank 'Flavors'

Since cost C is represented as any cost, it would be trivial to build a proof of work implementation of Decentral Bank where the costs are strictly hashing/mining costs and running of full nodes by miners. Costs would still be paid out in currency and accounted for in the same manner.