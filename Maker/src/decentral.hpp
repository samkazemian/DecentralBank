/**
 *  @file 
 *    contract header for CDP engine 
 *  @copyright 
 *    defined in DecentralBank/README.md
 *  @author
 *    richard tiutiun 
 *  @reference
 *    https://github.com/makerdao/dss/wiki
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include <string>

namespace eosiosystem { class system_contract; }

namespace eosio { 
   using std::string;

   class [[eosio::contract("decentral")]] daemu : public contract 
   {  public:
         using contract::contract;
         static constexpr int64_t  MAX_INT = (1LL << 62) - 1;
         static constexpr uint32_t VOTING_PERIOD = 604800; // seconds in a week
         
         /* Global settlement:
          * The system uses the market price of collateral 
          * at the time of settlement to compute how much
          * collateral each user gets. 
         */ [[eosio::action]]
            void settle( symbol_code symbol );

        /* Publish a proposal that either motions for the 
         * enactment of a new cdp type, or modification
         * of an existing one.
         */ [[eosio::action]] 
            void propose( name proposer, symbol_code symb, 
                          name clatrl, symbol_code cltrl,
                          name stabl, symbol_code stbl,
                          uint64_t max, uint64_t liq, 
                          uint64_t pen, uint64_t fee,
                          uint32_t tau, uint32_t ttl,
                          uint64_t beg );

        /* Take a position (for/against) on a cdp proposal
         */ [[eosio::action]] 
            void vote( name voter, symbol_code symb, 
                       bool against, asset quantity );

        /* Called either automatically after VOTE_PERIOD from the moment
         * of proposal via "propose", via deferred action with proposer's 
         * auth, or (if deferred action fails) may be called directly by
         * proposer after VOTE_PERIOD has passed. Proposal may be enacted
         * if there are more votes in favor than against. Cleanup must be
         * done regardless on relevant multi_index tables.
         */ [[eosio::action]]  
            void referend( name proposer, symbol_code symb );

         /* liquify action 
          * If the collateral value in a CDP drops below 150% of the outstanding Dai, 
          * the contract automatically sells enough of your collateral to buy back as
          * many Dai as you issued. The issued Dai is thus taken out of circulation. 
         */ [[eosio::action]]
            void liquify( name bidder, name owner, 
                          symbol_code symb, asset bidamt );
         
         /* The owner of an urn can transfer
          * its ownership at any time using give.  
         */ [[eosio::action]] 
            void give( name from, name to, symbol_code symb );
         
         /* Unless CDP is in liquidation, 
          * its owner can use lock to lock
          * more collateral. 
         */ [[eosio::action]] 
            void lock( name owner, symbol_code symb, asset quantity );

         /* Reclaims collateral from an overcollateralized cdp, 
          * without taking the cdp below its liquidation ratio. 
         */ [[eosio::action]]   
            void bail( name owner, symbol_code symb, asset quantity );
                 
         /* Issue fresh stablecoin from this cdp.
         */ [[eosio::action]]
            void draw( name owner, symbol_code symb, asset quantity );

         /* Owner can send back dai and reduce 
          * the cdp's issued stablecoin balance.
         */ [[eosio::action]]
            void wipe( name owner, symbol_code symb, asset quantity );            

         /* Owner can close this CDP, if the price feed
          * is up to date and the CDP is not in liquidation. 
          * Reclaims all collateral and cancels all issuance
          * plus fee. 
         */ [[eosio::action]]
            void shut( name owner, symbol_code symb );

         //TODO: dummy action to substitute for price feed ORACLE
         [[eosio::action]]
         void upfeed( name ram_payer, symbol_code symb, bool down );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void open( name owner, symbol_code symb, name ram_payer );

         [[eosio::action]]
         void close( name owner, symbol_code symb );

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:

         struct [[eosio::table]] bid 
         {
            //SCOPE: cdp type symbol
            name        owner; //cdp owner
            name        bidder;
            asset       bidamt;
            
            uint32_t    started;
            uint32_t    lastbid;
            //symbol_code bid_type;

            uint64_t primary_key()const { return owner.value; }
         };
         typedef eosio::multi_index< "bid"_n, bid > bids;

         struct [[eosio::table]] cdp 
         {         
            //SCOPE: cdp type symbol
            name     owner;
            //Amount of collateral currently locked by this CDP
            asset    collateral;
            //stablecoin issued
            asset    stablecoin;
            //in liquidation or not
            bool     live = true;

            uint64_t primary_key()const { return owner.value; }
         };
         typedef eosio::multi_index< "cdp"_n, cdp > cdps;

         struct [[eosio::table]] prop 
         {
            name        proposer;
            symbol_code cdp_type;   
            asset       vote_yes;
            asset       vote_no;
            uint32_t    expire;
            
            uint64_t    primary_key()const { return cdp_type.raw(); }
         };
         typedef eosio::multi_index< "prop"_n, prop > props;

         struct [[eosio::table]] stat 
         {   
            uint64_t    beg; //minimum bid increase
            //auctions will terminate after tau seconds have passed from start
            uint32_t    tau;
            //and after ttl seconds have passed since placement of last bid
            uint32_t    ttl;
            
            //CDT Type Symbol e.g...
            //DBEOS / DBKARMA...24*23*22*21*20 = over 5M different variants
            symbol_code cdp_type;   
            //max total stablecoin issuable by all CDPs of this type
            uint64_t    debt_ceiling;
            
            //by default, 13% of the collateral in the CDP 
            uint64_t    penalty_ratio; //units are tenths of a %
            //collateral asset units needed per issued stable tokens
            uint64_t    liquid8_ratio; //units are tenths of a %
            
            //Total debt owed by CDPs of this type, denominated in debt unit
            asset       fee_balance;
            asset       stability_fee; //units are tenths of a %
            
            asset       total_stablecoin;
            //Symbol and amount used as collateral for all CDPs of this type
            asset       total_collateral;

            //TODO: 
            // uint64_t    global_debt_ceiling
            //Short delay before the auto-liquidation commences
            // uint32_t delay;
            //Instead of these two names use extended assets for the totals
            name        stabl_contract; //account name of eosio.token for stablecoin
            name        clatrl_contract; //account name of eosio.token for collateral

            bool        live = false;
            uint32_t    last_veto = 0; //for parameter updates
            
            uint64_t    primary_key()const { return cdp_type.raw(); }
         };
         typedef eosio::multi_index< "stat"_n, stat > stats;
         
         struct [[eosio::table]] feed
         {
            symbol_code symb;
            uint64_t usd_per;
            uint32_t lastamp;

            uint64_t primary_key() const { return symb.raw(); }
         };
         typedef eosio::multi_index< "feed"_n, feed > feeds;

         // User's balance of stablecoin
         struct [[eosio::table]] account 
         {
            asset    balance;
            name     owner;
               
            uint64_t primary_key() const { return owner.value; }
         };      
         typedef eosio::multi_index< "accounts"_n, account > accounts;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value );
   };

} /// namespace eosio