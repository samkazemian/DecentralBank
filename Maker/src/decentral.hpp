/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("decentral")]] token : public contract {
      public:
         using contract::contract;

         /* global settlement action
          *
          * the system uses the market price of collateral at the time of settlement 
          * to compute how much collateral each user gets.
          * 
         */

         /* bid action 
          *
          * auction will terminate after t1 seconds have passed from start of auction
          * AND after t2 seconds have passed from placement of last bid
          * THEN winning bidder may their collateral from the CDP 
          * increasing bids are received up to cover the amount of outstanding stabletoken
          * + accumulated interest (called the “stability fee”) and a liquidation fee 
          * The Maker team told us they intend to add a short delay before auto-liquidation 
         */
         

        /* give action
         *
         * The owner of an urn can transfer its ownership at any time using give.
        */


         [[eosio::action]]
         void create( name   issuer,
                      asset  maximum_supply);


         [[eosio::action]]
         void create( name   issuer,
                      asset  maximum_supply);

         [[eosio::action]]
         void issue( name to, asset quantity, string memo );

         [[eosio::action]]
         void retire( asset quantity, string memo );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void open( name owner, const symbol& symbol, name ram_payer );

         [[eosio::action]]
         void close( name owner, const symbol& symbol );

         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:
         struct [[eosio::table]] types {
            //SCOPE: CDT Type
            //Token symbol used as collateral for CDPs of this type
            //Total debt owed by CDPs of this type, denominated in debt unit
            //stability fee rate
            //debt ceiling (max total stablecoin issuable by all CDPs of given type) 
            
            //how many collateral asset units needed per issued stable token
            //collateralization rate 
            
            //liquidation penalty rate.
            //by default, 13% of the collateral in the CDP 
            //vector of symbols for approved collaterals
         };

         struct [[eosio::table]] cdp {
            //SCOPE: CDP type identifier
            //account owner
            //account liquidator;
            //Amount of collateral currently locked by this CDP
            asset    collateral;
            //Amount of outstanding stable token issued by this CDP
            asset    balance;

            //secondary index by owner
            //by CDP id
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };



         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
   };

} /// namespace eosio

/*

If the collateral value in a CDP drops below 150% of the outstanding Dai, 
the contract automatically sells enough of your collateral to buy back as
many Dai as you issued. The issued Dai is thus taken out of circulation.



*/