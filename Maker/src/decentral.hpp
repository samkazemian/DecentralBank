/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 * https://github.com/liquidapps-io/eos-contracts-best-practices
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
            void settle(symbol_code symbol);

        /* Publish a proposal that either motions for the 
         * enactment of a new cdp type, or modification
         * of an existing one.
         */ [[eosio::action]] 
            void propose( name proposer, symbol_code symb, 
                          name clatrl, symbol_code cltrl,
                          name stabl, symbol_code stbl,
                          uint32_t tau, uint32_t ttl,
                          uint64_t max, uint64_t liq, 
                          uint64_t pen, uint64_t fee );

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

         /* bid action 
            *
            * AND after t2 seconds have passed from placement of last bid
            * THEN winning bidder may their collateral from the CDP 
            * increasing bids are received up to cover the amount of outstanding stabletoken
            * + accumulated interest (called the “stability fee”) and a liquidation fee 
            * The Maker team told us they intend to add a short delay before auto-liquidation 
            * 
         */
         
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

         struct [[eosio::table]] cdp 
         {
            //TODO: optimization...will calculate liq ratio less often
            //asset   issued_so_far;
            //asset   left_to_issue;
            
            //SCOPE: cdp type symbol
            name     owner;
            //account liquidator;
            //Amount of collateral currently locked by this CDP
            asset    collateral;
            //stablecoin issued
            asset    stablecoin;
            //risk state one of ENUM (eg in liquidation, etc)
            bool     live = true;

            uint64_t primary_key()const { return owner.value; }
         };

         struct [[eosio::table]] cdp_props 
         {
            symbol_code cdp_type;
            name        proposer;
            uint32_t    expire;
            
            asset       vote_no;
            asset       vote_yes;
            
            uint64_t    primary_key()const { return cdp_type.raw(); }
         };


         struct [[eosio::table]] cdp_stats 
         {   
            //auctions will terminate after tau seconds have passed from start
            uint32_t    tau;
            //and after ttl seconds have passed since placement of last bid
            uint32_t    ttl;
            
            symbol_code cdp_type;   
            //max total stablecoin issuable by all CDPs of this type
            uint64_t    debt_ceiling;
            //TODO: paid when?
            uint64_t    stability_fee;
            //by default, 13% of the collateral in the CDP 
            uint64_t    penalty_ratio; //units are tenths of a %
            //collateral asset units needed per issued stable tokens
            uint64_t    liquidation_ratio;
            
            //asset  fee_balance;
            //Total debt owed by CDPs of this type, denominated in debt unit
            asset       total_stablecoin;
            //Symbol and amount used as collateral for all CDPs of this type
            asset       total_collateral;

            name        stabl_contract; //account name of eosio.token for stablecoin
            name        clatrl_contract; //account name of eosio.token for collateral

            bool        live = false;
            uint64_t    usd_per_clatrl = 0;

            //CDT Type Symbol e.g...
            //DBEOS / DBKARMA...24*23*22*21*20 = over 5M different variants
            uint64_t    primary_key()const { return cdp_type.raw(); }
         };

         //struct [[eosio::table]] auction {
            //TODO: data model to incorporate both sides of auction in one struct
         //}

         // User's balance of stablecoin
         struct [[eosio::table]] account 
         {
            asset    balance;
            name     owner;
               
            uint64_t primary_key() const { return owner.value; }
         };


         typedef eosio::multi_index< "cdp"_n, cdp > cdps;
         typedef eosio::multi_index< "stat"_n, cdp_stats > stats;
         typedef eosio::multi_index< "prop"_n, cdp_props > props;
         typedef eosio::multi_index< "accounts"_n, account > accounts;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value );
   };

} /// namespace eosio