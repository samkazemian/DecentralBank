/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem { class system_contract; }

namespace eosio { 
   using std::string;

   class [[eosio::contract("decentral")]] daemu : public contract 
   {  public:
         using contract::contract;

         const uint32_t VOTING_PERIOD = 604800; // seconds in a week
         
         /* global settlement action
            *
            * the system uses the market price of collateral at the time of settlement 
            * to compute how much collateral each user gets.
            * 
         */

        /* vote action
            *
            * the system uses the market price of collateral at the time of settlement 
            * to compute how much collateral each user gets.
            * 
         /  *
            * 
         */

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
            void give(name from, name to, const symbol& symbol);

         
         /* Unless CDP is in liquidation, 
          * its owner can use lock to lock
          * more collateral. 
         */ [[eosio::action]] 
            void lock(name owner, const symbol& symbol, asset amt);


         /* free action (account owner, cdp id, asset amt)
            * 
            * reclaims collateral from an overcollateralized cdp, without taking the cdp below its liquidation ratio.
            * 
            * assert account owner is owner, require_auth owner
            * assert state not liquidation
            * assert no risk problems (except that debt ceiling may be exceeded)
            * assert symbol of amt matches cdp type's collateral
            * increment this cdp's collateral balance by amt
            * add_balance(amt) to owner
            * SEND_INLINE action to corresponding token contract (designated in cdp type)
            * 
         */


         /* draw action (account owner, cdp id, asset amt)
            *
            * issue fresh stablecoin from this cdp
            * 
            * assert account owner is owner, require_auth owner
            * assert cdp balance + amt <= cdp type debt ceiling
            * assert new liquidation ration <= cdp type liquidation ratio
            * increment this cdp's stablecoin balance by amt
            * increment this cdp type's total stablecoin balance by amt
            * add_balance(amt) to owner's stablecoin balance
            * update this cdp's risk state
         */


         /* wipe action (account owner, cdp id, asset amt)
            *
            * owner can send back dai and reduce the cdp's issued stablecoin balance
            * 
            * assert account owner is owner, require_auth owner
            * assert state not liquidation
            * decrement this cdp's stablecoin balance by amt
            * decrement this cdp type's total stablecoin balance by amt
            * sub_balance(amt) from owner's stablecoin balance
            * burn amt
         */


         /* Owner can close this CDP, if the price
          * feed is up to date and the CDP is not in liquidation. 
          * reclaims all collateral and cancels all issuance plus fee. 
         */ [[eosio::action]]
            void shut( name owner, const symbol& symbol );

         // [[eosio::action]]
         // void issue( name to, asset quantity, string memo );

         // [[eosio::action]]
         // void retire( asset quantity, string memo );

         // [[eosio::action]]
         // void transfer( name    from,
         //                name    to,
         //                asset   quantity,
         //                string  memo );

         [[eosio::action]]
         void open( name owner, const symbol& symbol, name ram_payer );

         [[eosio::action]]
         void close( name owner, const symbol& symbol );

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:

         // struct [[eosio::table]] cdp_bids {

         // };

         struct [[eosio::table]] cdp {
            //SCOPE: cdp type symbol
            name     owner;
            //account liquidator;
            //Amount of collateral currently locked by this CDP
            asset    collateral;
            //stablecoin issued
            asset    balance;
            //asset    issued_so_far;
            //asset    left_to_issue;
            //risk state one of ENUM (eg in liquidation, etc)
            bool     live;

            //CDP type identifier
            uint64_t primary_key()const { return owner.value; }
         };

         struct [[eosio::table]] cdp_props {
            //TODO: keep track how much voted by whom

            //SCOPE: VOTER
            uint32_t expiration; 
            asset    supply_yes;
            asset    supply_no;
            name     proposer;
            symbol   cdp_type;

            uint64_t primary_key()const { return cdp_type.code().raw(); }
         };


         struct [[eosio::table]] cdp_stats 
         {   
            bool     live;
            //auctions will terminate after tau seconds have passed from start
            uint32_t     tau;
            //and after ttl seconds have passed since placement of last bid
            uint32_t     ttl;
            
            uint64_t stability_fee;
            //max total stablecoin issuable by all CDPs of this type
            uint64_t debt_ceiling;
            //collateral asset units needed per issued stable tokens
            uint64_t liquidation_ratio;
            //by default, 13% of the collateral in the CDP 
            uint64_t penalty_ratio; //units are tenths of a %

            symbol   cdp_symbol;   
            //asset  fee_balance;
            //Symbol and amount used as collateral for all CDPs of this type
            asset    total_collateral;
            //Total debt owed by CDPs of this type, denominated in debt unit
            asset    total_stablecoin;

            //CDT Type Symbol e.g...
            //DBEOS / DBKARMA...24*23*22*21*20 = over 5M different variants
            uint64_t primary_key()const { return cdp_symbol.code().raw(); }
         };

         // User's balance of stablecoin
         struct [[eosio::table]] account {
            asset balance; 
               
            uint64_t primary_key() const { return balance.symbol.raw(); }
         };


         typedef eosio::multi_index< "cdp"_n, cdp > cdps;
         //typedef eosio::multi_index< "bid"_n, cdp_bids > bids;
         typedef eosio::multi_index< "stat"_n, cdp_stats > stats;
         typedef eosio::multi_index< "prop"_n, cdp_props > props;
         typedef eosio::multi_index< "accounts"_n, account > accounts;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value );
   };

} /// namespace eosio