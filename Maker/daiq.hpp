/*
 * @ author
 *   richard tiutiun 
 * @ copyright 
 *   defined in ../README.md
 * @ file 
 *   contract header for CDP engine 
 */
#pragma once

#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;
using std::make_tuple;
using std::string;

CONTRACT daiq : public contract 
{  using contract::contract;
   
   public:
      const eosio::symbol IQSYMBOL = symbol( symbol_code("IQ"), 3 );
      static constexpr uint32_t FEED_FRESH = 300; // seconds in 5 minute period
      static constexpr uint32_t VOTE_PERIOD = 10; //604800; // seconds in a week
      static constexpr uint32_t SECYR = 31557600; // seconds in a year on average
      
      /* Global settlement:
       * The system uses the market price of collateral 
       * at the time of settlement to compute how much
       * collateral each user gets. 
       */ ACTION settle( symbol_code symbl );

      /* Publish a proposal that either motions for the 
       * enactment of a new cdp type, or modification
       * of an existing one.
       */ ACTION propose( name proposer, symbol_code symbl, 
                          symbol_code clatrl, symbol_code stabl,
                          uint64_t max, uint64_t gmax, 
                          uint64_t pen, uint64_t fee,
                          uint64_t beg, uint64_t liq, 
                          uint32_t tau, uint32_t ttl,
                          name feeder );

      /* Take a position (for/against) on a cdp proposal
       */ ACTION vote( name voter, symbol_code symbl, 
                       bool against, asset quantity );

      /* Called either automatically after VOTE_PERIOD from the moment
       * of proposal via "propose", via deferred action with proposer's 
       * auth, or (if deferred action fails) may be called directly by
       * proposer after VOTE_PERIOD has passed. Proposal may be enacted
       * if there are more votes in favor than against. Cleanup must be
       * done regardless on relevant multi_index tables.
       */ ACTION referended( name proposer, symbol_code symbl );

      /* liquify action 
       * If the collateral value in a CDP drops below 150% of the outstanding Dai, 
       * the contract automatically sells enough of your collateral to buy back as
       * many Dai as you issued. The issued Dai is thus taken out of circulation. 
       */ ACTION liquify( name bidder, name owner, 
                          symbol_code symbl, asset bidamt );
      
      /* The owner of an urn can transfer
       * its ownership at any time using give.  
       */ //ACTION give( name giver, name taker, symbol_code symbl );
      
      /* Unless CDP is in liquidation, 
       * its owner can use lock to lock
       * more collateral. 
       */ ACTION lock( name owner, symbol_code symbl, asset quantity );

      /* Reclaims collateral from an overcollateralized cdp, 
       * without taking the cdp below its liquidation ratio. 
       */ ACTION bail( name owner, symbol_code symbl, asset quantity );
               
      /* Issue fresh stablecoin from this cdp.
       */ ACTION draw( name owner, symbol_code symbl, asset quantity );

      /* Owner can send back dai and reduce 
       * the cdp's issued stablecoin balance.
       */ ACTION wipe( name owner, symbol_code symbl, 
                       asset quantity, asset fee );            

      /* Owner can close this CDP, if the price feed
       * is up to date and the CDP is not in liquidation. 
       * Reclaims all collateral and cancels all issuance
       * plus fee. 
       */ ACTION shut( name owner, symbol_code symbl );

      /* Price feed data update action
       */ ACTION upfeed( name feeder, asset price, 
                         symbol_code cdp_type, symbol_code symbl );

      // Open cdp or initiate balances for account
      ACTION open( name owner, symbol_code symbl, name ram_payer );

      // Close balance of given symbol for account
      ACTION close( name owner, symbol_code symbl );

      ACTION deposit( name    from,
                      name    to,
                      asset   quantity,
                      string  memo );
      
      ACTION withdraw( name    from,
                       name    to,
                       asset   quantity,
                       string  memo );
      
      // unchanged from eosio.token
      static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
      {  accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw() );
         return ac.balance;
      }
      
   private:
      TABLE bid 
      {  //SCOPE: cdp type symbol
         name     owner; //cdp owner
         name     bidder;
         asset    bidamt;
         
         uint32_t started;
         uint32_t lastbid;
         //symbol_code bid_type;

         uint64_t primary_key() const { return owner.value; }
      }; typedef eosio::multi_index< "bid"_n, bid > bids;

      TABLE cdp 
      {  //SCOPE: cdp type symbol
         name     owner;
         uint32_t created;
         //Amount of collateral currently locked by this CDP
         asset    collateral;
         //stablecoin issued
         asset    stablecoin;
         //in liquidation or not
         bool     live = true;

         //last draw amt and last draw stamp for stability fee

         uint64_t primary_key() const { return owner.value; }
      }; typedef eosio::multi_index< "cdp"_n, cdp > cdps;

      TABLE prop 
      {  symbol_code cdp_type;
         name        proposer;
         uint32_t    deadline;
         asset       yay;
         asset       nay;
         
         uint64_t    primary_key() const { return cdp_type.raw(); }
      }; typedef     eosio::multi_index< "prop"_n, prop > props;

      TABLE stat 
      {  bool        live = false;
         uint32_t    last_vote = 0; //for parameter updates
         
         //auctions will terminate after tau seconds have passed from start
         uint32_t    tau;
         //and after ttl seconds have passed since placement of last bid
         uint32_t    ttl;
         
         name        feeder; //designated account for providing price feeds
         //CDT Type Symbol e.g...
         //DBEOS / DBKARMA...24*23*22*21*20 = over 5M different variants
         symbol_code cdp_type;   
         //max total stablecoin issuable by all CDPs of this type
         uint64_t    global_ceil;
         uint64_t    debt_ceiling;

         uint64_t     stability_fee; //units are APR %
         uint64_t     beg; //minimum bid increase in %
         //by default, 13% of the collateral in the CDP 
         uint64_t     penalty_ratio; //units are %
         //collateral asset units needed per issued stable tokens
         uint64_t     liquid8_ratio; //units are %
         
         asset       fee_balance; //total stability fees paid
         //Symbol and total amount locked as collateral for all CDPs of this type
         asset       total_collateral; 
         //Symbol and total amount of stablecoin in circulation for CDPs of this type
         asset       total_stablecoin;
         
         uint64_t    primary_key()const { return cdp_type.raw(); }
      }; typedef     eosio::multi_index< "stat"_n, stat > stats;
      
      TABLE feed
      {  symbol_code symbl;
         asset       price;
         uint32_t    stamp = 0;
         uint64_t    total = 0; //amount in circulation, ie not locked in cdp
         
         uint64_t    primary_key() const { return symbl.raw(); }
      }; typedef     eosio::multi_index< "feed"_n, feed > feeds;

      TABLE account
      {  asset    balance;
         name     owner;
            
         uint64_t primary_key() const { return owner.value; }
      }; typedef  eosio::multi_index< "accounts"_n, account > accounts;

      void sub_balance( name owner, asset value );
      void add_balance( name owner, asset value );
};