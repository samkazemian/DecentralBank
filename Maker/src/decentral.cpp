/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#define DEBUG
#include "logger.hpp"
#include "decentral.hpp"

namespace eosio {

void daemu::open( name owner, const symbol& symbol, name ram_payer )
{  require_auth( ram_payer );

   auto cdp_type_sym = symbol.code().raw();

   stats statstable( _self, _self.value );
   const auto& st = statstable.get( cdp_type_sym, 
                                    "cdp type does not exist" );
   
   cdps cdpstable( _self, cdp_type_sym );
   auto it = cdpstable.find( owner.value );
   
   eosio_assert( it == cdpstable.end(), 
                 "cdp of this type already exists for this owner" );
   
   cdpstable.emplace( ram_payer, [&]( auto& p ) {
      p.owner = owner;
      p.live = true;
      p.collateral = asset{ 0, st.total_collateral.symbol };
      p.balance = asset{ 0, st.total_stablecoin.symbol };
   });

   accounts acnts( _self, owner.value );
   auto collaterit = acnts.find( st.total_collateral.symbol.code().raw() );
   
   if ( collaterit == acnts.end() )
      acnts.emplace( ram_payer, [&]( auto& a ) {
        a.balance = asset{ 0, st.total_collateral.symbol };
      });
   
   auto stablit = acnts.find( st.total_stablecoin.symbol.code().raw() );
   
   if ( stablit == acnts.end() )
      acnts.emplace( ram_payer, [&]( auto& a ) {
        a.balance = asset{ 0, st.total_stablecoin.symbol };
      });
}

void daemu::give(name from, name to, const symbol& symbol)
{  require_auth( from );
   require_auth( to ); //it is a transfer of LIABILITY, after all

   auto cdp_type_sym = symbol.code().raw();

   stats statstable( _self, _self.value );
   const auto& st = statstable.get( cdp_type_sym, "cdp type does not exist" );
   
   eosio_assert( st.live != false, 
                 "cdp type either not yet voted into live, or in global settlement" 
               );

   cdps cdpstable( _self, cdp_type_sym );
   auto it = cdpstable.get( from.value, "owner doesn't have a cdp of this type" );
   
   eosio_assert( it.live != false, "this cdp is in liquidation" );

   cdpstable.modify( it, to, [&]( auto& p ) { p.owner = to; });
}


void daemu::lock(name owner, const symbol& symbol, asset amt) 
{  require_auth( owner );

   auto cdp_type_sym = symbol.code().raw();
  
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( cdp_type_sym, "cdp type does not exist" );
   
   eosio_assert( st.live != false, 
                 "cdp type either not yet voted into live, or in global settlement" 
               );
   eosio_assert( st.total_collateral.symbol == amt.symbol, 
                 "cdp type and collateral symbol mismatch" 
               );

   cdps cdpstable( _self, cdp_type_sym );
   auto it = cdpstable.get( owner.value, "owner doesn't have a cdp of this type" );
   
   eosio_assert( it.live != false, "this cdp is in liquidation" );

   sub_balance( owner, amt );
   cdpstable.modify( it, same_payer, [&]( auto& p ) { p.collateral += amt; });
}

void daemu::shut( name owner, const symbol& symbol ) 
{  require_auth( owner );
   auto cdp_type_sym = symbol.code().raw();
  
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( cdp_type_sym, "cdp type does not exist" );


   cdps cdpstable( _self, cdp_type_sym );
   auto it = cdpstable.get( owner.value, "owner doesn't have a cdp of this type" );
   
   eosio_assert( it.live != false, "this cdp is in liquidation" );
   
   statstable.modify( st, same_payer, [&]( auto& t ) { 
      t.total_collateral -= it.collateral; 
      t.total_stablecoin -= it.balance;
   });

   add_balance( owner, it.collateral );
   sub_balance( owner, it.balance );
   cdpstable.erase( it );
   
   //TODO: increment cdp type fee
   //What if insufficient balance for sub_balance
}

//void daemu::globalsettlement(const symbol& symbol) {
   //assert for cdp type total_collateral / total_stablecoin > liquation_ratio
   //live = false for the cdp type
//}


//void daemu::vote(const symbol& symbol, bool against) {
    //require_auth(voter_name);

    /* check vote has been initialised */
    //eosio_assert(referendum_config.exists(), "vote has not been initialised");

    /* check if vote is active */
    //eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* if user has staked, their will be a voter entry */
    //auto const voter = voter_info.find(voter_name);
//}

//void daemu::propose(name prop, const symbol& symbol, asset max, uint64_t liq, uint64_t pen, uint64_t fee) {
   // auto sym = quantity.symbol.code();
   // props propstable( _self, _self );
   // auto prop_exist = propstable.find( sym.code().raw() );
   // eosio_assert( existing == propstable.end(), "proposal for this cdp type already exists");

   // stats statstable( _self, _self );
   // auto stat_exist = statstable.find( sym.code().raw() );
   // if ( stat_exist == statstable.end() ) {
   //    statstable.emplace(_self, [&]( auto& s ) {

   //    });
   // }

   // propsstable.emplace( _self, [&]( auto& s ) {
   //    s.expiration = now() + VOTING_PERIOD;
   // });
//}

//void daemu::referend(name from, name to, const symbol& symbol) {
   //if expiration period has passed
   //if supply_yes > supply_no
   //live = true 
   //else remove from types
   //erase proposal

//}

//void daemu::bid(name owner, const symbol& symbol) {
   
   //get the cdp, scope the symbol
   //if state is not locked check liquidation
   //if undercollateralized put state locked
   //else return

   //bit smash the owner and the cdp type: ID
   //if bid exists for this id by this bidder 
   //modify amount
   //else emplace

   //get first bid, 

   /*
      * If the collateral value in a CDP drops below 150% of the outstanding Dai, 
      * the contract automatically sells enough of your collateral to buy back as
      * many Dai as you issued. The issued Dai is thus taken out of circulation.
   */
//}

// void token::issue( name to, asset quantity, string memo ) {
//    auto sym = quantity.symbol;
//    eosio_assert( sym.is_valid(), "invalid symbol name" );
//    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

//    stats statstable( _self, sym.code().raw() );
//    auto existing = statstable.find( sym.code().raw() );
//    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
//    const auto& st = *existing;

//    require_auth( st.issuer );
//    eosio_assert( quantity.is_valid(), "invalid quantity" );
//    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

//    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
//    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

//    statstable.modify( st, same_payer, [&]( auto& s ) {
//       s.supply += quantity;
//    });

//    add_balance( st.issuer, quantity, st.issuer );

//    if( to != st.issuer ) {
//    SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
//                         { st.issuer, to, quantity, memo }
//    );
//    }
// }

// void token::retire( asset quantity, string memo ) {
//    auto sym = quantity.symbol;
//    eosio_assert( sym.is_valid(), "invalid symbol name" );
//    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

//    stats statstable( _self, sym.code().raw() );
//    auto existing = statstable.find( sym.code().raw() );
//    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
//    const auto& st = *existing;

//    require_auth( st.issuer );
//    eosio_assert( quantity.is_valid(), "invalid quantity" );
//    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

//    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

//    statstable.modify( st, same_payer, [&]( auto& s ) {
//       s.supply -= quantity;
//    });

//    sub_balance( st.issuer, quantity );
// }

// void token::transfer( name    from,
//                       name    to,
//                       asset   quantity,
//                       string  memo ) {
//    eosio_assert( from != to, "cannot transfer to self" );
//    require_auth( from );
//    eosio_assert( is_account( to ), "to account does not exist");
//    auto sym = quantity.symbol.code();
//    stats statstable( _self, sym.raw() );
//    const auto& st = statstable.get( sym.raw() );

//    require_recipient( from );
//    require_recipient( to );

//    eosio_assert( quantity.is_valid(), "invalid quantity" );
//    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
//    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
//    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

//    auto payer = has_auth( to ) ? to : from;

//    logger_info("quantity.amount: ", quantity.amount);
   
//    sub_balance( from, quantity );
//    add_balance( to, quantity, payer );
// }

void daemu::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.balance -= value;
   });
}

void daemu::add_balance( name owner, asset value ) {
   accounts to_acnts( _self, owner.value );
   const auto& to = to_acnts.get( value.symbol.code().raw(), "no balance object found" );
  
   to_acnts.modify( to, same_payer, [&]( auto& a ) {
      a.balance += value;
   });
   
}

void daemu::close( name owner, const symbol& symbol ) 
{  require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::daemu, (open)(close)(shut)(give)(lock) )