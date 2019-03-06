/**
 *  @ file
 *    contract implementation for CDP engine
 *    tests in ./local.sh and ./testnet.sh
 *  @ copyright MIT, in DecentralBank/README.md 
 *    explanation in ./README.md
 *  @ author
 *    richard tiutiun 
 */

#include "daemu.hpp"

#define DEBUG
#include "logger.hpp"

using namespace eosio;

//TODO: balances are non-zero for debug purposes, should be 0
//except maybe VTO, you should pre-seed accounts with some amt
ACTION daemu::open( name owner, symbol_code symb, name ram_payer )
{  require_auth( ram_payer );
   
   accounts vetos( _self, symbol("VTO", 4).code().raw() );  
   auto vetoit = vetos.find( owner.value );
   if ( vetoit == vetos.end() )
      vetos.emplace( ram_payer, [&]( auto& a ) {
         a.owner = owner;
         a.balance = asset{ 1000000, symbol("VTO", 4) };
      });
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   auto s = statstable.find( symb.raw() );
   if ( s != statstable.end() ) {
      const auto& st = *s;
      accounts stabl( _self, st.total_stablecoin.symbol.code().raw() );  
      auto stablit = stabl.find( owner.value );
      if ( stablit == stabl.end() )
         stabl.emplace( ram_payer, [&]( auto& a ) {
            a.owner = owner;
            a.balance = asset{ 1000, st.total_stablecoin.symbol };
         });
      accounts clatrl( _self, st.total_collateral.symbol.code().raw() );  
      auto collaterit = clatrl.find( owner.value );
      if ( collaterit == clatrl.end() )
         clatrl.emplace( ram_payer, [&]( auto& a ) {
            a.owner = owner;
            a.balance = asset{ 1000000, st.total_collateral.symbol };
         });
      cdps cdpstable( _self, symb.raw() );
      auto it = cdpstable.find( owner.value );
      if ( it == cdpstable.end() ) {
         eosio_assert( st.live != false, 
                       "cdp type not yet live, or in global settlement" 
                     );
         cdpstable.emplace( ram_payer, [&]( auto& p ) {
            p.owner = owner;
            p.collateral = asset{ 0, st.total_collateral.symbol };
            p.stablecoin = asset{ 0, st.total_stablecoin.symbol };
         }); 
      } 
   }
}

ACTION daemu::give( name giver, name taker, symbol_code symb )
{  require_auth( giver );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   cdps cdpstable( _self, symb.raw() );
   const auto& git = cdpstable.get( giver.value, 
                                   "(cdp type, owner) mismatch" 
                                  );  
   eosio_assert( git.live, "cdp is in liquidation" );

   auto tit = cdpstable.find( taker.value );
   eosio_assert( tit == cdpstable.end(), 
                 "taker already has a cdp of this type" 
               );
   cdpstable.emplace( giver, [&]( auto& p ) { 
      p.owner = taker;
      p.collateral = git.collateral;
      p.stablecoin = git.stablecoin;
   });
   cdpstable.erase(git);
}

ACTION daemu::bail( name owner, symbol_code symb, asset quantity )
{  require_auth(owner);
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(),
                                    "cdp type doesn't exist" 
                                  );
   eosio_assert( quantity.symbol == st.total_collateral.symbol, 
                 "(cdp type, collateral symbol) mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                            "(cdp type, owner) mismatch" 
                          );
   eosio_assert( it.live, "cdp is in liquidation" );
   eosio_assert( it.collateral > quantity,
                 "can't free this much collateral" 
               );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   eosio_assert( fc.lastamp >= now() - FEED_FRESHNESS,
                  "collateral price feed data too stale"
               ); 
   uint64_t amt = it.collateral.amount - quantity.amount;

   double liq = (double)( amt * fc.usd_per );
   if ( it.stablecoin.amount > 0 )
      liq /= (double)( it.stablecoin.amount * 100.00 );
   else liq = st.liquid8_ratio;
   
   eosio_assert( liq >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   add_balance( owner, quantity ); //increase owner's collateral balance
   
   cdpstable.modify( it, owner, [&]( auto& p ) { //update the cdp
      p.collateral -= quantity; 
   });
   //update amount of collateral locked up by cdps of this type
   statstable.modify( st, same_payer,  [&]( auto& t ) {
      t.total_collateral -= quantity; 
   });
   //update amount of this collateral in circulation globally 
   feedstable.modify( fc, same_payer, [&]( auto& f ) { 
      f.totals += quantity.amount;
   });
}

ACTION daemu::draw( name owner, symbol_code symb, asset quantity )
{  require_auth( owner );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(),
                                    "cdp type doesn't exist" 
                                  );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, 
                 "(cdp type, stablecoin symbol) mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "(cdp type, owner) mismatch" 
                                 );
   eosio_assert( it.live, "cdp is in liquidation" );
   
   uint64_t amt = it.stablecoin.amount + quantity.amount;
   uint64_t gmt = st.total_stablecoin.amount + quantity.amount; 

   eosio_assert( st.debt_ceiling >= amt, 
                 "can't go above debt ceiling" 
               );
   eosio_assert( st.global_ceil >= gmt, 
                 "can't go above global debt ceiling"
               );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   eosio_assert( fc.lastamp >= now() - FEED_FRESHNESS,
                 "collateral price feed data too stale"
               ); 
   double liq = (double)(it.collateral.amount * fc.usd_per);
   liq /= (double)(amt * 100.00);

   eosio_assert( liq >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   add_balance( owner, quantity ); //increase owner's stablecoin balance

   cdpstable.modify( it, same_payer, [&]( auto& p ) { //update cdp
      p.stablecoin += quantity; 
   });
   //update amount of stablecoin in circulation for this cdp type
   statstable.modify( st, same_payer,  [&]( auto& t ) {
      t.total_stablecoin += quantity; 
   });
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), 
                                    "feed doesn't exist" 
                                  );
   //update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) { 
      f.totals += quantity.amount;
   });
}

ACTION daemu::wipe( name owner, symbol_code symb, asset quantity )
{  require_auth( owner );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symb.is_valid(), "invalid symbol name" ); 
  
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(),
                                    "cdp type does not exist" 
                                  );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, 
                 "(cdp type, symbol) mismatch"  
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "(cdp type, owner) mismatch" 
                                 );
   eosio_assert( it.live, "cdp in liquidation" );
   eosio_assert( it.stablecoin > quantity, 
                 "can't wipe this much" 
               );
   sub_balance( owner, quantity ); //decrease owner's stablecoin balance
   
   //update amount of stablecoin in circulation for this cdp type
   statstable.modify( st, same_payer,  [&]( auto& t ) { 
      t.total_stablecoin -= quantity; 
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) { //update cdp
      p.stablecoin -= quantity; 
   });
   feeds feedstable(_self, _self.value );
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   //update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) { 
      f.totals -= quantity.amount;
   });
}

ACTION daemu::lock( name owner, symbol_code symb, asset quantity ) 
{  require_auth( owner );
   eosio_assert( quantity.amount > 0, 
                 "must use positive quantity" 
               );
   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   eosio_assert( st.live != false, 
                 "cdp type not yet live, or in global settlement"
               );
   eosio_assert( st.total_collateral.symbol == quantity.symbol, 
                 "(cdp type, collateral symbol) mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   eosio_assert( it.live != false, "cdp in liquidation" );
   sub_balance( owner, quantity ); //decrease owner's collateral balance
   
   //update amount of collateral in circulation for this cdp type
   statstable.modify( st, same_payer,  [&]( auto& t ) { 
      t.total_collateral += quantity; 
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) { //update cdp
      p.collateral += quantity; 
   });
   feeds feedstable(_self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   //update amount of this collateral in circulation globally
   feedstable.modify( fc, same_payer, [&]( auto& f ) { 
      f.totals -= quantity.amount;
   });
}

ACTION daemu::shut( name owner, symbol_code symb ) 
{  require_auth( owner );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
  
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   eosio_assert( it.live, "cdp in liquidation" );
   //TODAY: grows over time?
   //do on every bail, pro rata
   //increment cdp type fee balance
   //sub_balance( owner, st.stability_fee); 
   
   asset new_total_stabl = st.total_stablecoin;
   if (it.stablecoin.amount > 0) {
      sub_balance( owner, it.stablecoin ); 
      new_total_stabl -= it.stablecoin;
   }
   asset new_total_clatrl = st.total_collateral;
   if (it.collateral.amount > 0) {
      add_balance( owner, it.collateral );
      new_total_clatrl += it.collateral;
   }
   statstable.modify( st, same_payer, [&]( auto& t ) { 
      //TODAY: t.fee_balance += stability fee;
      t.total_stablecoin = new_total_stabl;
      t.total_collateral = new_total_clatrl; 
   });
   cdpstable.erase(it);
}

ACTION daemu::settle( symbol_code symb ) 
{  eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( st.total_collateral.symbol.raw(), 
                                    "no price data" 
                                  );
   eosio_assert( fc.lastamp >= now() - FEED_FRESHNESS,
                 "collateral price feed data too stale"
               ); 
   double liq = (double)( st.total_collateral.amount * fc.usd_per );
   liq /= (double) st.total_stablecoin.amount; //TODAY: divide by 0
   if ( st.liquid8_ratio > liq )                                  
      statstable.modify( st, same_payer, [&]( auto& t ) { 
         t.live = false; 
      });
   else if ( !st.live )
      statstable.modify( st, same_payer, [&]( auto& t ) { 
         t.live = true; 
      });
   
   // TODAY: vote to settle a type even above conditions are not met
   // propstable.emplace( proposer, [&]( auto& p ) {
   //    p.cdp_type = symb;
   //    p.proposer = proposer;
   //    p.expire = now() + VOTING_PERIOD;
   //    p.vote_no = asset(0, symbol("VETO", 4));
   //    p.vote_yes = asset(0, symbol("VETO", 4));
   // }); 
   //refund collateral to everyone who is holding a cdp type 
   //that is in global settlement
   //
   //SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
   //                                     { st.issuer, to, quantity, memo }
   //                  );
}

ACTION daemu::vote( name voter, symbol_code symb, 
                    bool against, asset quantity 
                  ) { require_auth( voter );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.is_valid() && quantity.amount > 0, 
                 "invalid quantity" 
               );
   eosio_assert( quantity.symbol == symbol("VTO", 4), 
                 "must vote with governance token"
               );
   props propstable( _self, _self.value );
   const auto& pt = propstable.get( symb.raw(), 
                                    "cdp type not proposed" 
                                  );
   sub_balance( voter, quantity );

   accounts acnts( _self, symb.raw() );  
   auto at = acnts.find( voter.value );
   if ( at == acnts.end() )
      acnts.emplace( voter, [&]( auto& a ) {
         //we know the symbol is VETO anyway
         //so instead keep track of prop symbol
         a.owner = voter;
         a.balance = asset(quantity.amount, symbol( symb, 4 )); 
      });
   else
      acnts.modify( at, same_payer, [&]( auto& a ) {
         a.balance += asset(quantity.amount, symbol( symb, 4 ));
      });
   if (against)
      propstable.modify( pt, same_payer, [&]( auto& p ) { 
         p.vote_no += quantity; 
      });
   else
      propstable.modify( pt, same_payer, [&]( auto& p ) { 
         p.vote_yes += quantity; 
      });
}

ACTION daemu::liquify( name bidder, name owner, 
                       symbol_code symb, asset bidamt 
                     ) { require_auth( bidder );
   eosio_assert( bidder.value != owner.value, 
                 "cdp owner can't bid" 
               );
   eosio_assert( symb.is_valid(),
                 "invalid cdp type symbol name" 
               );
   eosio_assert( bidamt.is_valid() && bidamt.amount > 0,
                 "bid is invalid" 
               );
   feeds feedstable( _self, _self.value );
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   if ( it.live ) {
      eosio_assert ( it.stablecoin.amount > 0, "can't liquify this cdp");
      const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                       "no price data" 
                                     );
      eosio_assert( fc.lastamp >= now() - FEED_FRESHNESS,
                    "collateral price feed data too stale"
                  ); 
      double liq = (double)( it.collateral.amount * fc.usd_per );
      liq /= (double) it.stablecoin.amount;
      
      eosio_assert( st.liquid8_ratio > liq,
                    "must exceed liquidation ratio" 
                  );
      cdpstable.modify( it, bidder, [&]( auto& p ) { //liquidators get the RAM
         p.live = false; 
      });
   } bids bidstable( _self, symb.raw() );
   auto bt = bidstable.find( owner.value );
   if ( bt == bidstable.end() ) {
      eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                    "bid is of wrong symbol" 
                  );
      sub_balance( bidder, bidamt );
      bidstable.emplace( bidder, [&]( auto& b ) {
         b.owner = owner;         
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.started = now();
         b.lastbid = now();
      }); 
      cdpstable.modify( it, bidder, [&]( auto& p ) { //liquidators get the RAM
         p.stablecoin -= bidamt;
      });
   } else if ( st.tau > (now() - bt->started) &&
               st.ttl > (now() - bt->lastbid) ) {
      eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                    "bid is of wrong symbol" 
                  );
      eosio_assert( bidamt.amount >= st.beg + bt->bidamt.amount,  
                    "bid too small"
                  );
      sub_balance( bidder, bidamt );
      add_balance( bt->bidder, bt->bidamt ); 
      //subtract difference between bids cdp balance      
      cdpstable.modify( it, bidder, [&]( auto& p ) { 
         p.stablecoin -= ( bidamt - bt->bidamt );
      });
      bidstable.modify( bt, bidder, [&]( auto& b ) { 
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.lastbid = now();
      });   
      //TODAY: penalty
   } else {
      if ( it.collateral.amount ) {
         add_balance( bt->bidder, it.collateral );
         cdpstable.modify( it, same_payer, [&]( auto& p ) { //update cdp 
            p.collateral -= p.collateral;
         });
         const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                          "feed doesn't exist" 
                                       );
         //update amount of this collateral in circulation globally
         feedstable.modify( fc, same_payer, [&]( auto& f ) {
            f.totals += it.collateral.amount;
         });
         //update amount of collateral locked up by cdps of type
         if (st.total_collateral.symbol == it.collateral.symbol)
            statstable.modify( st, same_payer,  [&]( auto& t ) {
               t.total_collateral -= it.collateral; 
            });
      } 
      if ( it.stablecoin.amount )  {
         const auto& fv = feedstable.get( symbol("VTO", 4).code().raw(), 
                                          "no price data" 
                                        );
         eosio_assert( fv.lastamp >= now() - FEED_FRESHNESS,
                       "veto price feed data too stale"
                     ); 
         if ( it.stablecoin.amount > 0 )
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = asset( int64_t(it.stablecoin.amount / fv.usd_per) * 100, symbol("VTO", 4) );
            });   
         else if ( it.stablecoin.amount < 0 ) {
            asset stamt = -it.stablecoin;
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = stamt;
               p.stablecoin = asset( int64_t(stamt.amount / fv.usd_per) * 100, symbol("VTO", 4) );
            });
         }
         eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                       "bid is of wrong symbol" 
                     );
         sub_balance( bidder, bidamt );
         const auto& fs = feedstable.get( bidamt.symbol.code().raw(), 
                                          "no price data" 
                                       );
         feedstable.modify( fs, same_payer, [&]( auto& f ) {
            f.totals -= bidamt.amount;
         });
         cdpstable.modify( it, bidder, [&]( auto& p ) { 
            p.stablecoin -= bidamt;
         });
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
         add_balance( bt->bidder, it.collateral );
         const auto& fc = feedstable.get( it.collateral.symbol.code().raw(), 
                                         "feed doesn't exist" 
                                       );
         feedstable.modify( fc, same_payer, [&]( auto& f ) {
            f.totals += it.collateral.amount;
         });
      }
      bidstable.erase(bt);
      cdpstable.erase(it);
   }
}

ACTION daemu::propose( name proposer, symbol_code symb, 
                       symbol_code cltrl, symbol_code stbl,
                       uint64_t max, uint64_t gmax, 
                       uint64_t pen, uint64_t fee, 
                       uint32_t tau, uint32_t ttl,
                       double beg, double liq 
                     ) { require_auth( proposer );
   eosio_assert( symb.is_valid(), 
                 "invalid cdp type symbol name" 
               );
   eosio_assert( cltrl.is_valid(),
                 "invalid collateral symbol name" 
               );
   eosio_assert( stbl.is_valid(), 
                 "invalid stablecoin symbol name" 
               );
   //TODAY: improve validation
   // eosio_assert( //tau < MAX_INT && different max for 32 bit ints
   //               //ttl < MAX_INT &&
   //               fee < MAX_INT && 
   //               max < MAX_INT && 
   //               beg < MAX_INT && beg > 0 &&
   //               liq < MAX_INT && liq >= 1000 //must be at least 100% collateralized
   //               //cannot be more than 100% of collateral, measured in 10ths of a %
   //               && pen <= 1000, "bad input" 
   //             );
   feeds feedstable(_self, _self.value);
   eosio_assert( feedstable.find( symb.raw() ) == feedstable.end(), 
                 "can't propose collateral or governance symbols"
               ); 
   props propstable( _self, _self.value );
   eosio_assert( propstable.find( symb.raw() ) == propstable.end(), 
                 "proposal for this cdp type exists"
               );
   /* scope into owner because self scope may already have this symbol
    * if symbol exists in self scope and this proposal is voted into live
    * then original symbol would be erased, and symbol in proposer's scope
    * will be moved into self scope, and erased from proposer's scope
    */   
   stats pstable( _self, proposer.value ); 
   eosio_assert( pstable.find( symb.raw() ) == pstable.end(), 
                 "proposer already claimed this cdp type"
               );
   pstable.emplace( proposer, [&]( auto& t ) {
      t.tau = tau;
      t.ttl = ttl;
      t.beg = beg; //TODAY: %
      t.cdp_type = symb;
      t.global_ceil = gmax;
      t.debt_ceiling = max;
      t.penalty_ratio = pen;
      t.liquid8_ratio = liq;
      t.fee_balance = asset(0, symbol("VTO", 4));
      t.stability_fee = asset(fee, symbol("VTO", 4));
      t.total_stablecoin = asset(0, symbol(stbl, 2 )); 
      t.total_collateral = asset(0, symbol(cltrl, 4 ));
   });
   propstable.emplace( proposer, [&]( auto& p ) {
      p.cdp_type = symb;
      p.proposer = proposer;
      p.expire = now() + VOTING_PERIOD;
      p.vote_no = asset(0, symbol("VTO", 4));
      p.vote_yes = asset(0, symbol("VTO", 4));
   }); 
   transaction txn{};
   txn.actions.emplace_back(  permission_level { _self, "active"_n },
                              _self, "referended"_n, 
                              make_tuple( proposer, symb )
                           ); txn.delay_sec = VOTING_PERIOD;
   uint128_t txid = (uint128_t(proposer.value) << 64) | now();
   txn.send(txid, _self); 
}

ACTION daemu::referended( name proposer, symbol_code symb ) 
{  require_auth( _self );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
  
   stats pstable( _self, proposer.value ); //TODAY: acceptable for settlement
   const auto& pst = pstable.get( symb.raw(), "no such proposal" );

   props propstable( _self, _self.value );
   const auto& prop = propstable.get( symb.raw(), "no such proposal" );
   eosio_assert( now() >= prop.expire, "too soon to be referended" );

   //the proposal was successfully voted on
   if ( prop.vote_yes > prop.vote_no ) {
      //implement proposed changes...
      stats statstable( _self, _self.value );
      auto stat_exist = statstable.find( symb.raw() );
      if ( stat_exist == statstable.end() )
         statstable.emplace( _self, [&]( auto& t ) {
            t.live = true;
            t.tau = pst.tau;
            t.ttl = pst.ttl;
            t.beg = pst.beg;
            t.last_veto = now();
            t.cdp_type = pst.cdp_type;
            t.global_ceil = pst.global_ceil;
            t.debt_ceiling = pst.debt_ceiling;
            t.stability_fee = pst.stability_fee;
            t.penalty_ratio = pst.penalty_ratio;
            t.liquid8_ratio = pst.liquid8_ratio;
            t.total_stablecoin = pst.total_stablecoin;
            t.total_collateral = pst.total_collateral;
         });
      else 
         statstable.modify( stat_exist, same_payer, [&]( auto& t ) {
            t.live = true;
            t.tau = pst.tau;
            t.ttl = pst.ttl;
            t.beg = pst.beg;
            t.last_veto = now();
            t.global_ceil = pst.global_ceil;
            t.debt_ceiling = pst.debt_ceiling;
            t.stability_fee = pst.stability_fee;
            t.penalty_ratio = pst.penalty_ratio;
            t.liquid8_ratio = pst.liquid8_ratio;
         });
   } else if ( prop.vote_yes == prop.vote_no ) { //vote tie, so revote
      transaction txn{};
      txn.actions.emplace_back(  permission_level { _self, "active"_n },
                                 _self, "referended"_n, 
                                 make_tuple( proposer, symb )
                              ); txn.delay_sec = VOTING_PERIOD;
      uint128_t txid = (uint128_t(proposer.value) << 64) | now();
      txn.send(txid, _self); 
   } 
   //now we refund everyone who voted with their VETO tokens
   accounts vacnts( _self, symb.raw() );
   auto it = vacnts.begin();
   while ( it != vacnts.end() ) {
      add_balance( it->owner, asset(it->balance.amount, symbol("VTO", 4 )) );   
      it = vacnts.erase(it);
   }
   //now we can safely erase the proposal
   pstable.erase(pst);
   propstable.erase(prop);
}

//TODAY: asset amt, so give price and no need for symb
ACTION daemu::upfeed( symbol_code symb, bool down ) 
{  require_auth( _self );

   feeds feedstable( _self, _self.value );
   auto fit = feedstable.find( symb.raw() );
   if ( fit == feedstable.end() )
      feedstable.emplace( _self, [&]( auto& f ) {
         f.symb = symb;
         f.usd_per = 1;
         f.lastamp = now();
      }); 
   else if (down)
      feedstable.modify( fit, same_payer, [&]( auto& f ) {
         f.usd_per = 0;
         f.lastamp = now();
      });
   else
      feedstable.modify( fit, same_payer, [&]( auto& f ) {
         f.usd_per = 2;
         f.lastamp = now();
      });
}

ACTION daemu::transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo 
                      ) { require_auth( from );
   eosio_assert( from != to, "cannot transfer to self" );
   eosio_assert( is_account( to ), "to account does not exist");
   accounts toke( _self, quantity.symbol.code().raw() );
   eosio_assert( toke.find( from.value ) != toke.end() &&
                 toke.find( to.value ) != toke.end(),
                 "send and recipient must both have balances"
               );
   require_recipient( from );
   require_recipient( to );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   
   sub_balance( from, quantity );
   add_balance( to, quantity);
}

ACTION daemu::close( name owner, symbol_code symb ) 
{  require_auth( owner );
   accounts acnts( _self, symb.raw() );
   auto it = acnts.get( owner.value,
                        "no balance object found"
                      );
   eosio_assert( it.balance.amount == 0, 
                 "Cannot close because the balance is not zero." 
               );
   acnts.erase(it);
}

void daemu::sub_balance( name owner, asset value ) 
{  accounts from_acnts( _self, value.symbol.code().raw() );
   const auto& from = from_acnts.get( owner.value, 
                                      "no balance object found" 
                                    );
   eosio_assert( from.balance.amount >= value.amount, 
                 "overdrawn balance" 
               );
   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.balance -= value;
   });
}

void daemu::add_balance( name owner, asset value ) 
{  accounts to_acnts( _self, value.symbol.code().raw() );
   const auto& to = to_acnts.get( owner.value, 
                                  "no balance object found" 
                                );
   to_acnts.modify( to, same_payer, [&]( auto& a ) {
      a.balance += value;
   });
}

//checking all transfers, and not only from EOS system token
extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
      if (action == "transfer"_n.value && code != receiver)
         eosio::execute_action(eosio::name(receiver), eosio::name(code), &daemu::transfer);
      if (code == receiver)
         switch (action) {
            EOSIO_DISPATCH_HELPER( daemu, (open)(close)(shut)(give)(lock)(bail)(draw)(wipe)(settle)(vote)(propose)(referended)(liquify)(upfeed) )
         }
}
