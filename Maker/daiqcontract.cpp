/*
 * @ author
 *   richard tiutiun 
 * @ copyright 
 *   defined in ../README.md 
 * @ file
 *   contract implementation for CDP engine
 *   design/features explained in ./README.md
 */

#include "daiqcontract.hpp"

using namespace eosio;      

// ACTION daiqcontract::give( name giver, name taker, symbol_code symbl )
// {  require_auth( giver ); 
//    is_account( taker );
//    eosio_assert( symbl.is_valid(), "invalid symbol name" );
   
//    stats stable( _self, _self.value );
//    const auto& st = stable.get( symbl.raw(), 
//                                     "cdp type doesn't exist" 
//                                   );
//    cdps cdpstable( _self, symbl.raw() );
//    const auto& git = cdpstable.get( giver.value, 
//                                    "(cdp type, owner) mismatch" 
//                                   );  
//    eosio_assert( git.live, "cdp is in liquidation" );

//    auto tit = cdpstable.find( taker.value );
//    eosio_assert( tit == cdpstable.end(), 
//                  "taker already has a cdp of this type" 
//                );
//    cdpstable.emplace( giver, [&]( auto& p ) { 
//       p.owner = taker;
//       p.created = git.created;
//       p.collateral = git.collateral;
//       p.stablecoin = git.stablecoin;
//    });
//    cdpstable.erase( git );
// }

ACTION daiqcontract::open( name owner, symbol_code symbl, name ram_payer )
{  require_auth( ram_payer );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );

   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(),
                                "cdp type doesnt exst"
                              );
   eosio_assert( st.live, 
                 "cdp type not yet live, or in global settlement" 
               );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( st.total_collateral.quantity.symbol.code().raw(), 
                                    "collateral feed doesn't exist" 
                                  );
   eosio_assert( fc.stamp >= now() - FEED_FRESH,
                  "collateral price feed data too stale"
               );    
   cdps cdpstable( _self, symbl.raw() );
   eosio_assert( cdpstable.find( owner.value ) == cdpstable.end(),
                 "cdp of this type already exists for owner" 
               );
   cdpstable.emplace( ram_payer, [&]( auto& p ) {
      p.owner = owner;
      p.created = now();
      p.collateral = asset( 0, st.total_collateral.quantity.symbol );
      p.stablecoin = asset( 0, st.total_stablecoin.symbol );
   }); 
}

ACTION daiqcontract::bail( name owner, symbol_code symbl, asset quantity )
{  require_auth(owner);
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(),
                                "cdp type doesn't exist" 
                              );
   eosio_assert( quantity.symbol == st.total_collateral.quantity.symbol, 
                 "(cdp type, collateral symbol) mismatch" 
               );
   cdps cdpstable( _self, symbl.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "(cdp type, owner) mismatch" 
                                 );
   eosio_assert( it.live, "cdp is in liquidation" );
   eosio_assert( it.collateral > quantity,
                 "can't free this much collateral, try shut?" 
               );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   eosio_assert( fc.stamp >= now() - FEED_FRESH || !st.live,
                 "collateral price feed data too stale"
               ); 
   uint64_t amt = st.liquid8_ratio;
   if ( it.stablecoin.amount ) //just safety against divide by 0
      amt = ( fc.price.amount * 100 / it.stablecoin.amount ) *
            ( it.collateral.amount - quantity.amount ) / 10000;
   eosio_assert( amt >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   add_balance( owner, quantity, st.total_collateral.contract );
   
   cdpstable.modify( it, owner, [&]( auto& p ) 
   {  p.collateral -= quantity; });
   //update amount of collateral locked up by cdps of this type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity -= quantity; });
   //update amount of this collateral in circulation globally 
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

ACTION daiqcontract::draw( name owner, symbol_code symbl, asset quantity )
{  require_auth( owner );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(),
                                    "cdp type doesn't exist" 
                                  );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, 
                 "(cdp type, stablecoin symbol) mismatch" 
               );
   eosio_assert( st.live, 
                 "cdp type not yet live, or in global settlement" 
               );
   cdps cdpstable( _self, symbl.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "(cdp type, owner) mismatch" 
                                 );
   eosio_assert( it.live, "cdp is in liquidation" );
   
   uint64_t amt = it.stablecoin.amount + quantity.amount;
   uint64_t gmt = st.total_stablecoin.amount + quantity.amount; 

   eosio_assert( st.debt_ceiling >= amt, 
                 "can't reach the debt ceiling" 
               );
   eosio_assert( st.global_ceil >= gmt, 
                 "can't reac the global debt ceiling"
               );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   eosio_assert( fc.stamp >= now() - FEED_FRESH,
                 "collateral price feed data too stale"
               ); 
   uint64_t liq = ( fc.price.amount * 100 / amt ) *
                  ( it.collateral.amount ) / 10000;
   eosio_assert( liq >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   add_balance( owner, quantity, _self ); //increase owner's stablecoin balance
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.stablecoin += quantity; });
   //update amount of stablecoin in circulation for this cdp type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_stablecoin += quantity; });
   //update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

ACTION daiqcontract::wipe( name owner, symbol_code symbl, asset quantity ) 
{  require_auth( owner );
   eosio_assert( symbl.is_valid(), "invalid symbol name" ); 
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   stats stable( _self, _self.value );
   feeds feedstable(_self, _self.value );
   const auto& st = stable.get( symbl.raw(),
                                "cdp type does not exist" 
                             );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, 
                 "(cdp type, symbol) mismatch"  
               );
   cdps cdpstable( _self, symbl.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "(cdp type, owner) mismatch" 
                                 ); 
   eosio_assert( it.live, "cdp in liquidation" );
   eosio_assert( it.stablecoin > quantity, 
                 "can't wipe this much, try shut?" 
               ); 
   const auto& fv = feedstable.get( IQ_SYMBOL.code().raw(), 
                                    "no price data" 
                                  );
   uint64_t apr = 1000000 * st.stability_fee * 
                  ( now() - it.created ) / SECYR *
                  ( quantity.amount * 100 / fv.price.amount ) / 100000;
   asset fee = asset( apr, IQ_SYMBOL );
   sub_balance( owner, fee ); //pay stability fee
   //add_balance( _self, fee, IQ_NAME ); //increase contract's total fee balance
   sub_balance( owner, quantity ); //decrease owner's stablecoin balance
   //update amount of stablecoin in circulation for this cdp type
   stable.modify( st, same_payer,  [&]( auto& t ) { 
      t.fee_balance += fee; //just to keep track
      t.total_stablecoin -= quantity;
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.stablecoin -= quantity; });
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   //update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

ACTION daiqcontract::lock( name owner, symbol_code symbl, asset quantity ) 
{  require_auth( owner );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   eosio_assert( st.live, 
                 "cdp type not yet live, or in global settlement"
               );
   eosio_assert( st.total_collateral.quantity.symbol == quantity.symbol, 
                 "(cdp type, collateral symbol) mismatch" 
               );
   cdps cdpstable( _self, symbl.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   eosio_assert( it.live, "cdp in liquidation" );
   name contract = sub_balance( owner, quantity );
   eosio_assert( st.total_collateral.contract == contract,
                 "no using tokens from fake contracts" 
               );
   //update amount of collateral in circulation for this cdp type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity += quantity; });
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.collateral += quantity; });
   feeds feedstable(_self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   //update amount of this collateral in circulation globally
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

ACTION daiqcontract::shut( name owner, symbol_code symbl ) 
{  require_auth( owner );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
  
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), 
                                "cdp type doesn't exist" 
                              );
   cdps cdpstable( _self, symbl.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   eosio_assert( it.live, "cdp in liquidation" );
   
   asset new_total_stabl = st.total_stablecoin;
   if (it.stablecoin.amount > 0) {
      sub_balance( owner, it.stablecoin ); 
      new_total_stabl -= it.stablecoin;
   }
   asset new_total_clatrl = st.total_collateral.quantity;
   if ( it.collateral.amount > 0 ) {
      add_balance( owner, it.collateral, 
                   st.total_collateral.contract 
                 );
      new_total_clatrl += it.collateral;
   }
   stable.modify( st, same_payer, [&]( auto& t ) { 
      t.total_stablecoin = new_total_stabl;
      t.total_collateral.quantity = new_total_clatrl; 
   });
   cdpstable.erase( it );
}

ACTION daiqcontract::settle( name feeder, symbol_code symbl ) 
{  require_auth( feeder );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), 
                                "cdp type doesn't exist" 
                              );
   eosio_assert( st.total_stablecoin.amount > 0 && st.live, 
                 "cdp type already settled, or has 0 debt"
               ); 
   eosio_assert( st.feeder == feeder, 
                 "only feeder can trigger global settlement"
               ); 
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( st.total_collateral.quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   uint64_t liq = ( fc.price.amount * 100 * st.total_collateral.quantity.amount ) /
                  ( st.total_stablecoin.amount * 10000 );
                   
   eosio_assert( st.liquid8_ratio >= liq, 
                 "no liquidation hazard for settlement"
               );                                  
   stable.modify( st, same_payer, [&]( auto& t ) 
   {  t.live = false; });
}

ACTION daiqcontract::vote( name voter, symbol_code symbl, 
                           bool against, asset quantity ) 
{  require_auth( voter );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.is_valid() && quantity.amount > 0, 
                 "invalid quantity" 
               );
   eosio_assert( quantity.symbol == IQ_SYMBOL, 
                 "must vote with governance token"
               );
   props propstable( _self, _self.value );
   const auto& pt = propstable.get( symbl.raw(), 
                                    "cdp type not proposed" 
                                  );
   sub_balance( voter, quantity );
   accounts acnts( _self, symbl.raw() );  
   auto at = acnts.find( voter.value );
   if ( at == acnts.end() )
      acnts.emplace( voter, [&]( auto& a ) {
         //we know the symbol is IQ anyway
         //so instead keep track of prop symbol
         a.owner = voter;
         a.balance = asset( quantity.amount, symbol( symbl, 3 ) ); 
      });
   else
      acnts.modify( at, same_payer, [&]( auto& a ) 
      {  a.balance += asset( quantity.amount, symbol( symbl, 3 ) ); });
   if (against)
      propstable.modify( pt, same_payer, [&]( auto& p ) 
      {  p.nay += quantity; });
   else
      propstable.modify( pt, same_payer, [&]( auto& p ) 
      {  p.yay += quantity; });
}

ACTION daiqcontract::liquify( name bidder, name owner, 
                              symbol_code symbl, asset bidamt ) 
{  require_auth( bidder );
   eosio_assert( symbl.is_valid(),
                 "invalid cdp type symbol name" 
               );
   eosio_assert( bidamt.is_valid() && bidamt.amount > 0,
                 "bid is invalid" 
               );
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), 
                                "cdp type doesn't exist" 
                              );
   cdps cdpstable( _self, symbl.raw() );
   feeds feedstable( _self, _self.value );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   if ( it.live ) {
      eosio_assert ( it.stablecoin.amount > 0, "can't liquify this cdp" );
      const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                       "no price data" 
                                     );
      eosio_assert( fc.stamp >= now() - FEED_FRESH || !st.live,
                    "collateral price feed data too stale"
                  ); 
      uint64_t liq = ( fc.price.amount * 100 / it.stablecoin.amount ) *
                     ( it.collateral.amount ) / 10000;
      eosio_assert( st.liquid8_ratio > liq,
                    "must exceed liquidation ratio" 
                  );
      cdpstable.modify( it, bidder, [&]( auto& p ) { // liquidators get the RAM
         p.live = false; 
         p.stablecoin.amount += ( p.stablecoin.amount * st.penalty_ratio ) / 100;
      });
   } 
   bids bidstable( _self, symbl.raw() );
   auto bt = bidstable.find( owner.value );
   if ( bt == bidstable.end() ) {
      eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                    "bid is of wrong symbol" 
                  );
      sub_balance( bidder, bidamt );
      cdpstable.modify( it, bidder, [&]( auto& p ) 
      {  p.stablecoin -= bidamt; });
      bidstable.emplace( bidder, [&]( auto& b ) {
         b.owner = owner;         
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.started = now();
         b.lastbid = now();
      }); 
   } else if ( st.ttl > ( now() - bt -> lastbid ) &&
               st.tau > ( now() - bt -> started ) ) {
      eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                    "bid is of wrong symbol" 
                  ); name contract = _self;
      uint64_t min = bt -> bidamt.amount + ( bt -> bidamt.amount * st.beg / 100 );
      eosio_assert( bidamt.amount >= min, "bid too small" ); 

      if ( it.stablecoin.symbol == IQ_SYMBOL )     
         contract = IQ_NAME;      
      sub_balance( bidder, bidamt );
      add_balance( bt -> bidder, bt -> bidamt, contract ); 
      //subtract difference between bids cdp balance
      cdpstable.modify( it, bidder, [&]( auto& p ) 
      {  p.stablecoin.amount -= ( bidamt - bt -> bidamt ); }); 
      bidstable.modify( bt, bidder, [&]( auto& b ) { 
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.lastbid = now();
      });   
   } else {
      if ( it.collateral.amount ) {
         cdpstable.modify( it, same_payer, [&]( auto& p )
         {  p.collateral -= p.collateral; });
         const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                          "feed doesn't exist" 
                                        ); name contract = _self;
         //update amount of this collateral in circulation globally
         feedstable.modify( fc, same_payer, [&]( auto& f ) 
         {  f.total += it.collateral; });
         if ( it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;
         //update amount of collateral locked up by cdps of type
         else if ( st.total_collateral.quantity.symbol == it.collateral.symbol ) {
            stable.modify( st, same_payer,  [&]( auto& t ) 
            {  t.total_collateral.quantity -= it.collateral; });
            contract = st.total_collateral.contract;
         }
         add_balance( bt -> bidder, it.collateral, contract );
      } 
      if ( it.stablecoin.amount )  {
         const auto& fv = feedstable.get( IQ_SYMBOL.code().raw(), 
                                          "no price data" 
                                        );
         eosio_assert( fv.stamp >= now() - FEED_FRESH,
                       "iq price feed data too stale"
                     ); 
         uint64_t iqamt = ( it.stablecoin.amount / fv.price.amount ) * 1000;
         if ( it.stablecoin.amount > 0 )
            cdpstable.modify( it, same_payer, [&]( auto& p ) 
            {  p.collateral = asset( iqamt, IQ_SYMBOL ); });   
         else if ( it.stablecoin.amount < 0 )
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = -it.stablecoin;
               p.stablecoin = asset( -iqamt, IQ_SYMBOL );
            });
         eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                       "bid is of wrong symbol" 
                     );
         sub_balance( bidder, bidamt );
         const auto& fs = feedstable.get( bidamt.symbol.code().raw(), 
                                          "no price data" 
                                        );
         feedstable.modify( fs, same_payer, [&]( auto& f ) 
         {  f.total -= bidamt; });
         cdpstable.modify( it, bidder, [&]( auto& p ) 
         {  p.stablecoin -= bidamt; });
         bidstable.modify( bt, bidder, [&]( auto& b ) { 
            b.bidder = bidder;
            b.bidamt = bidamt;
            b.started = now();
            b.lastbid = now();
         });
      }
   } 
   if ( it.stablecoin.amount == 0 ) {
      if ( it.collateral.amount ) {
         name contract = _self;
         if ( it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;
         else if ( it.collateral.symbol == st.total_collateral.quantity.symbol )
            contract = st.total_collateral.contract;
         add_balance( bt -> bidder, it.collateral, contract );
         const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                          "feed doesn't exist" 
                                        );
         feedstable.modify( fc, same_payer, [&]( auto& f ) 
         {  f.total += it.collateral; });
      }
      bidstable.erase( bt );
      cdpstable.erase( it );
   }
}

ACTION daiqcontract::propose( name proposer, symbol_code symbl, 
                              symbol clatrl, symbol_code stabl,
                              uint64_t max, uint64_t gmax, 
                              uint64_t pen, uint64_t fee,
                              uint64_t beg, uint64_t liq, 
                              uint32_t tau, uint32_t ttl,
                              name feeder, name contract ) 
{  require_auth( proposer );
   is_account( feeder );
   eosio_assert( symbl.is_valid(), 
                 "invalid cdp type symbol name" 
               );
   eosio_assert( clatrl.is_valid(),
                 "invalid collateral symbol name" 
               );
   eosio_assert( stabl.is_valid(), 
                 "invalid stablecoin symbol name" 
               );
   props propstable( _self, _self.value );
   eosio_assert( propstable.find( symbl.raw() ) == propstable.end(), 
                 "proposal for this cdp type already in session"
               );
   /* scope into owner because self scope may already have this symbol.
    * if symbol exists in self scope and this proposal is voted into live,
    * then original symbol would be erased, and symbol in proposer's scope
    * will be moved into self scope, then erased from proposer's scope.
    */ 
   stats stable( _self, _self.value );   
   stats pstable( _self, proposer.value ); 
   eosio_assert( pstable.find( symbl.raw() ) == pstable.end(), 
                 "proposer already claimed this cdp type"
               ); feeds feedstable(_self, _self.value);
   eosio_assert( feedstable.find( symbl.raw() ) == feedstable.end(), 
                 "can't propose collateral or governance symbols"
               ); 
   eosio_assert( feedstable.find( stabl.raw() ) != feedstable.end(), 
                 "can't propose an unknown stablecoin symbol"
               );
   if ( !tau && !ttl ) { // passing 0 for ttl, tau = flip settlement
      const auto& st = stable.get( symbl.raw(), 
                                   "cdp type doesn't exist" 
                                 );
      eosio_assert( proposer == st.feeder, 
                    "only priviliged accounts may propose global settlement"
                  );
   } else {
      eosio_assert( max < gmax && max > 0 &&
                    gmax <= max * 1000000 &&
                    pen <= 100 && pen > 0 &&
                    fee <= 100 && fee > 0 &&
                    beg <= 100 && beg > 0 &&
                    liq <= 1000 && liq >= 100
                    && tau <= 3600 
                    && ttl <= 600, "bad props"
                  ); //TODO: are these realistic?
      pstable.emplace( proposer, [&]( auto& t ) {
         t.tau = tau;
         t.ttl = ttl;
         t.beg = beg;
         t.feeder = feeder;
         t.cdp_type = symbl;
         t.global_ceil = gmax;
         t.debt_ceiling = max;
         t.penalty_ratio = pen;
         t.liquid8_ratio = liq;
         t.stability_fee = fee;
         t.fee_balance = asset( 0, IQ_SYMBOL ); 
         t.total_stablecoin = asset( 0, symbol( stabl, 2 ) ); 
         t.total_collateral = extended_asset( asset( 0, clatrl ), 
                                              contract
                                            );
      });
   }
   propstable.emplace( proposer, [&]( auto& p ) {
      p.cdp_type = symbl;
      p.proposer = proposer;
      p.deadline = now() + VOTE_PERIOD;
      p.nay = asset( 0, IQ_SYMBOL );
      p.yay = asset( 0, IQ_SYMBOL );
   }); 
   transaction txn{};
   txn.actions.emplace_back(  permission_level { _self, "active"_n },
                              _self, "referended"_n, 
                              make_tuple( proposer, symbl )
                           ); txn.delay_sec = VOTE_PERIOD;
   uint128_t txid = (uint128_t(proposer.value) << 64) | now();
   txn.send(txid, _self); 
}

ACTION daiqcontract::referended( name proposer, symbol_code symbl ) 
{  require_auth( _self );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
  
   props propstable( _self, _self.value );
   const auto& prop = propstable.get( symbl.raw(), "no such proposal" );
   eosio_assert( now() >= prop.deadline, "too soon to be referended" );
      
   if ( prop.yay == prop.nay ) { //vote tie, so revote
      transaction txn{};
      txn.actions.emplace_back(  permission_level { _self, "active"_n },
                                 _self, "referended"_n, 
                                 make_tuple( proposer, symbl )
                              ); txn.delay_sec = VOTE_PERIOD;
      uint128_t txid = (uint128_t(proposer.value) << 64) | now();
      txn.send(txid, _self); 
   } else { //the proposal was voted in for or against
      stats pstable( _self, proposer.value );
      auto pstat = pstable.find( symbl.raw() );
      bool settlement = ( pstat == pstable.end() );
      if ( prop.yay > prop.nay ) { //implement proposed changes...
         stats stable( _self, _self.value );
         auto stat_exist = stable.find( symbl.raw() );
         if ( settlement )
            stable.modify( stat_exist, same_payer, [&]( auto& t ) 
            {  t.live = !t.live; });
         else {
            if ( stat_exist == stable.end() )
               stable.emplace( _self, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstat -> tau;
                  t.ttl = pstat -> ttl;
                  t.beg = pstat -> beg;
                  t.feeder = pstat -> feeder;
                  t.cdp_type = pstat -> cdp_type;
                  t.global_ceil = pstat -> global_ceil;
                  t.debt_ceiling = pstat -> debt_ceiling;
                  t.stability_fee = pstat -> stability_fee;
                  t.penalty_ratio = pstat -> penalty_ratio;
                  t.liquid8_ratio = pstat -> liquid8_ratio;
                  t.fee_balance = pstat -> fee_balance;
                  t.total_stablecoin = pstat -> total_stablecoin;
                  t.total_collateral = pstat -> total_collateral;
               });
            else
               stable.modify( stat_exist, same_payer, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstat -> tau;
                  t.ttl = pstat -> ttl;
                  t.beg = pstat -> beg;
                  t.feeder = pstat -> feeder;
                  t.global_ceil = pstat -> global_ceil;
                  t.debt_ceiling = pstat -> debt_ceiling;
                  t.stability_fee = pstat -> stability_fee;
                  t.penalty_ratio = pstat -> penalty_ratio;
                  t.liquid8_ratio = pstat -> liquid8_ratio;
               });
         }
      } //refund all voters' voting tokens
      accounts vacnts( _self, symbl.raw() );
      auto it = vacnts.begin();
      while ( it != vacnts.end() ) { //TODO: refactor, unsustainable loop
         add_balance( it -> owner, asset( it -> balance.amount, IQ_SYMBOL ), IQ_NAME );   
         it = vacnts.erase( it );
      } 
      if ( !settlement ) //safely erase the proposal
         pstable.erase( *pstat );
      propstable.erase( prop );
   }
}

ACTION daiqcontract::upfeed( name feeder, asset price, 
                             symbol_code cdp_type, symbol symbl ) 
{  require_auth( feeder );
   eosio_assert( price.is_valid(), "invalid quantity" );
   eosio_assert( symbl.is_valid(), "invalid symbol name" );
   eosio_assert( price.amount > 0, 
                 "must use positive quantity" 
               );
   if ( !has_auth( _self ) ) {
      stats stable( _self, _self.value );
      const auto& st = stable.get( cdp_type.raw(), 
                                   "cdp type doesn't exist" 
                                 );
      eosio_assert( st.feeder == feeder, 
                    "account not authorized to be price feeder"  
                  );
      eosio_assert( st.total_stablecoin.symbol == price.symbol, 
                    "price symbol must match stablecoin symbol"
                  );
      eosio_assert( st.total_collateral.quantity.symbol == symbl, 
                    "asset symbol must match collateral symbol" 
                  );
   } feeds feedstable( _self, _self.value );
   auto ft = feedstable.find( symbl.code().raw() );
   if ( ft != feedstable.end() )
      feedstable.modify( ft, feeder, [&]( auto& f ) {
         f.price.amount *= (17 / 20);
         f.price.amount += (3 / 20) * price.amount;
         f.stamp = now();
      });
   else
      feedstable.emplace( _self, [&]( auto& f ) {
         f.total = asset( 0, symbl );
         f.price = price;
         f.stamp = now();
      }); 
}

ACTION daiqcontract::deposit( name from, name to,
                              asset quantity, string memo ) 
{  require_auth( from );
   name contract = get_code();
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   if ( from != _self ) {
      eosio_assert( to == _self, "can only deposit to self" ); 
      if ( quantity.symbol == IQ_SYMBOL )
         eosio_assert ( contract == IQ_NAME, 
                        "IQ deposit must come from IQ contract" 
                      );
      add_balance( from, quantity, contract );
      feeds feedstable( _self, _self.value );
      const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                       "feed doesn't exist" 
                                     );
      feedstable.modify( fc, same_payer, [&]( auto& f ) 
      {  f.total += quantity; });
   }
}

ACTION daiqcontract::transfer( name from, name to,
                               asset quantity, string memo )
{  require_auth( from );
   eosio_assert( is_account( to ), "to account does not exist");
   eosio_assert( from != to, "cannot transfer to self" );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   name contract = sub_balance( from, quantity );
   eosio_assert( contract == _self, "can only transfer stablecoin");
   add_balance( to, quantity, contract);
}

ACTION daiqcontract::withdraw( name owner, asset quantity, string memo ) 
{  require_auth( owner );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount >= 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   if ( quantity.amount == 0 )
      quantity = get_balance( _self, quantity.symbol.code(), owner );
   name code = sub_balance( owner, quantity );
   eosio_assert( owner != _self && code != _self, 
                 "self cannot withdraw, cannot withdraw into self" 
               ); feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   action(
      permission_level{ _self, name( "active" ) },
      code, name( "transfer" ),
      std::make_tuple( _self, owner, quantity, memo )
   ).send(); 
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

ACTION daiqcontract::close( name owner, symbol_code symbl ) 
{  require_auth( owner );
   accounts acnts( _self, symbl.raw() );
   auto it = acnts.get( owner.value,
                        "no balance object found"
                      );
   eosio_assert( it.balance.amount == 0, 
                 "balance must be zero" 
               );
   acnts.erase( it );
}

name daiqcontract::sub_balance( name owner, asset value ) 
{  accounts from_acnts( _self, value.symbol.code().raw() );
   const auto& from = from_acnts.get( owner.value, 
                                      "no balance object found" 
                                    );
   eosio_assert( from.balance.amount >= value.amount, 
                 "overdrawn balance" 
               );
   from_acnts.modify( from, owner, [&]( auto& a ) 
   {  a.balance -= value; }); return from.code;
}

void daiqcontract::add_balance( name owner, asset value, name code ) 
{  accounts to_acnts( _self, value.symbol.code().raw() );
   auto to = to_acnts.find( owner.value );
   
   if ( to == to_acnts.end() ) 
      to_acnts.emplace( _self, [&]( auto& a ) { 
         a.balance = value;
         a.owner = owner; 
         a.code = code; 
      });
   else {
      eosio_assert( to -> code == code, "wrong contract" );
      to_acnts.modify( to, same_payer, [&]( auto& a ) 
      { a.balance += value; });
   }
}

//checking all transfers, and not only from EOS system token
extern "C" void apply( uint64_t receiver, uint64_t code, uint64_t action ) 
{  if ( action == "transfer"_n.value && code != receiver )
      eosio::execute_action( eosio::name(receiver), eosio::name(code), &daiqcontract::deposit );
   if ( code == receiver )
      switch ( action ) {
         EOSIO_DISPATCH_HELPER( daiqcontract, /*(give)*/(open)(close)(shut)(lock)(bail)(draw)(wipe)(settle)(vote)(propose)(referended)(liquify)(upfeed)(withdraw) )
      }
}