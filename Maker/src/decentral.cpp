/**
 *  @file
 *    contract implementation for CDP engine
 *  @copyright 
 *    defined in DecentralBank/README.md
 *  @author
 *    richard tiutiun 
 */

#define DEBUG
#include "logger.hpp"
#include "decentral.hpp"

namespace eosio {

void daemu::open( name owner, symbol_code symb, name ram_payer )
{  require_auth( ram_payer );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type does not exist" 
                                  );
   eosio_assert( st.live != false, 
                 "cdp type not yet live, or in global settlement" 
               );
   cdps cdpstable( _self, symb.raw() );
   auto it = cdpstable.find( owner.value );
   eosio_assert( it == cdpstable.end(), 
                 "owner already has a cdp of this type" 
               );
   cdpstable.emplace( ram_payer, [&]( auto& p ) {
      p.owner = owner;
      p.collateral = asset{ 0, st.total_collateral.symbol };
      p.stablecoin = asset{ 0, st.total_stablecoin.symbol };
   }); 

   //TODO: refactor...not sure if I can reuse one variable
   accounts stabl( _self, st.total_stablecoin.symbol.code().raw() );  
   accounts clatrl( _self, st.total_collateral.symbol.code().raw() );  
   accounts vetos( _self, symbol("VTO", 4).code().raw() );  

   auto stablit = stabl.find( owner.value );
   if ( stablit == stabl.end() )
      stabl.emplace( ram_payer, [&]( auto& a ) {
         a.balance = asset{ 0, st.total_stablecoin.symbol };
      });
   auto collaterit = clatrl.find( owner.value );
   if ( collaterit == clatrl.end() )
      clatrl.emplace( ram_payer, [&]( auto& a ) {
         a.balance = asset{ 0, st.total_collateral.symbol };
      });
   auto vetoit = vetos.find( symbol("VTO", 4).code().raw() );
   if ( vetoit == vetos.end() )
      vetos.emplace( ram_payer, [&]( auto& a ) {
         a.balance = asset{ 0, symbol("VTO", 4) };
      });
}

void daemu::give( name from, name to, symbol_code symb )
{  require_auth( to ); //because it's transfer of LIABILITY
   require_auth( from );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( from.value, 
                                   "cdp type + owner mismatch" 
                                 );  
   eosio_assert( it.live != false, "cdp is in liquidation" );
   
   cdpstable.modify( it, same_payer, [&]( auto& p ) { 
      p.owner = to; 
   });
}

void daemu::bail( name owner, symbol_code symb, asset quantity )
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
   // eosio_assert( st.last_update >= now() - 2,
   //                "collateral usd price stale"
   //             ); 
   eosio_assert( quantity.symbol == st.total_collateral.symbol, 
                 "cdp type + collateral symbol mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                            "cdp type + owner mismatch" 
                          );
   eosio_assert( it.live != false, "cdp is in liquidation" );
   eosio_assert( it.collateral.amount > quantity.amount,
                 "can't free this much collateral" 
               );
   feeds feedstable( _self, _self.value );
   const auto& ft = feedstable.get( it.collateral.symbol.raw(), 
                                    "no price data" 
                                  );
   uint64_t amt = it.collateral.amount - quantity.amount;
   amt *= ft.usd_per / it.stablecoin.amount; //TODO: divide by zero

   eosio_assert( amt >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   add_balance( owner, quantity ); //increase the owner's collateral balance
   
   cdpstable.modify( it, owner, [&]( auto& p ) { //decrease cdp's collateral balance
      p.collateral -= quantity; 
   });
   statstable.modify( st, same_payer,  [&]( auto& t ) { //and cdp type's as well
      t.total_collateral -= quantity; 
   });
}

void daemu::draw( name owner, symbol_code symb, asset quantity )
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
   // eosio_assert( st.last_update >= now() - 2,
   //                "collateral usd price stale"
   //             );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, 
                 "cdp type + stablecoin symbol mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "cdp type + owner mismatch" 
                                 );
   eosio_assert( it.live != false, "cdp is in liquidation" );
   
   uint64_t amt = it.stablecoin.amount + quantity.amount; 
   eosio_assert( st.debt_ceiling >= amt, 
                 "can't go above debt ceiling" 
               );
   feeds feedstable( _self, _self.value );
   const auto& ft = feedstable.get( it.collateral.symbol.raw(), 
                                    "no price data" 
                                  );
   uint64_t liq = it.collateral.amount * ft.usd_per / amt;
   eosio_assert( liq >= st.liquid8_ratio, 
                 "can't go below liquidation ratio" 
               );
   add_balance( owner, quantity );

   statstable.modify( st, same_payer,  [&]( auto& t ) {
      t.total_stablecoin += quantity; 
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) { 
      p.stablecoin += quantity; 
   });
}

void daemu::wipe( name owner, symbol_code symb, asset quantity )
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
   eosio_assert( it.live != false, "cdp in liquidation" );
   eosio_assert( it.stablecoin.amount > quantity.amount,
                 "can't wipe this much" 
               );
   sub_balance( owner, quantity );
   
   statstable.modify( st, same_payer,  [&]( auto& t ) { 
      t.total_stablecoin -= quantity; 
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) { 
      p.stablecoin -= quantity; 
   });
}

void daemu::lock( name owner, symbol_code symb, asset quantity ) 
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
                 "cdp type + collateral symbol mismatch" 
               );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   eosio_assert( it.live != false, "cdp in liquidation" );
   sub_balance( owner, quantity );

   statstable.modify( st, same_payer,  [&]( auto& t ) { 
      t.total_collateral += quantity; 
   });
   cdpstable.modify( it, same_payer, [&]( auto& p ) { 
      p.collateral += quantity; 
   });
}

void daemu::shut( name owner, symbol_code symb ) 
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
   eosio_assert( it.live != false, "cdp in liquidation" );
   //TODO: grows over time?
   //do on every bail, pro rata
   sub_balance( owner, st.stability_fee); 
   sub_balance( owner, it.stablecoin ); 
   add_balance( owner, it.collateral );
   //TODO: increment cdp type fee
   statstable.modify( st, same_payer, [&]( auto& t ) { 
      //t.fee_balance += stability fee;
      t.total_collateral -= it.collateral; 
      t.total_stablecoin -= it.stablecoin;
   });
   cdpstable.erase( it );
}

void daemu::settle( symbol_code symb ) 
{  eosio_assert( symb.is_valid(), "invalid symbol name" );
   
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   // eosio_assert( st.last_update >= now() - 2,
   //                "collateral usd price stale"
   //             );
   feeds feedstable( _self, _self.value );
   const auto& ft = feedstable.get( st.total_collateral.symbol.raw(), 
                                    "no price data" 
                                  );
   uint64_t liq = st.total_collateral.amount * ft.usd_per;
   liq /= st.total_stablecoin.amount;

   if ( st.liquid8_ratio > liq )                                  
      statstable.modify( st, same_payer, [&]( auto& t ) { 
         t.live = false; 
      });
   else if ( !st.live )
      statstable.modify( st, same_payer, [&]( auto& t ) { 
         t.live = true; 
      });
   
   // TODO: vote to settle a type even above conditions are not met
   // propstable.emplace( proposer, [&]( auto& p ) {
   //    p.cdp_type = symb;
   //    p.proposer = proposer;
   //    p.expire = now() + VOTING_PERIOD;
   //    p.vote_no = asset(0, symbol("VETO", 4));
   //    p.vote_yes = asset(0, symbol("VETO", 4));
   // }); 

   //TODO: to refund collateral to everyone who is holding a cdp type 
   //that is in global settlement
   //
   //SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
   //                                     { st.issuer, to, quantity, memo }
   //                  );
}

void daemu::vote( name voter, symbol_code symb, 
                  bool against, asset quantity ) {  
   require_auth( voter );
  
   eosio_assert( symb.is_valid(), "invalid symbol name" );
   eosio_assert( quantity.is_valid() && quantity.amount > 0, 
                 "invalid quantity" 
               );
   eosio_assert( quantity.symbol == symbol("VETO", 4), 
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
         a.balance = asset(0, symbol( symb, 4 )); 
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

void daemu::liquify( name bidder, name owner, 
                     symbol_code symb, asset bidamt ) {

   require_auth( bidder );
   eosio_assert( symb.is_valid(),
                 "invalid cdp type symbol name" 
               );
   eosio_assert( bidamt.is_valid() && bidamt.amount > 0,
                 "bid is invalid" 
               );
   stats statstable( _self, _self.value );
   const auto& st = statstable.get( symb.raw(), 
                                    "cdp type doesn't exist" 
                                  );
   cdps cdpstable( _self, symb.raw() );
   const auto& it = cdpstable.get( owner.value, 
                                   "this cdp doesn't exist" 
                                 );
   if ( it.live ) {
      feeds feedstable( _self, _self.value );
      const auto& ft = feedstable.get( it.collateral.symbol.raw(), 
                                       "no price data" 
                                     );
      uint64_t liq = it.collateral.amount * ft.usd_per;
      liq /= it.stablecoin.amount;
      eosio_assert( liq > st.liquid8_ratio,
                    "must exceed liquidation ratio" 
                  );
      eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                    "bid is of wrong symbol" 
                  );
      eosio_assert( bidamt.amount > 0,
                    "bid must be > 0" 
                  );
      cdpstable.modify( it, bidder, [&]( auto& p ) { //liquidators get the RAM
         p.live = false; 
         p.stablecoin -= bidamt;
      });
   } 
   bids bidstable( _self, symb.raw() );
   auto bt = bidstable.find( owner.value );
   
   if ( bt == bidstable.end() ) {
      eosio_assert( bidamt.symbol == st.total_stablecoin.symbol, 
                     "bid in wrong symbol" 
                  );
      sub_balance( bidder, bidamt );
      bidstable.emplace( bidder, [&]( auto& b ) {
         b.owner = owner;         
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.started = now();
         b.lastbid = now();
      }); 
      //decrement from cdp dai balance
   } else if ( st.tau > now() - bt->started &&
               st.ttl > now() - bt->lastbid ) {
      eosio_assert( bidamt.symbol == bt->bidamt.symbol,
                    "bid in wrong symbol" 
                  );
      eosio_assert( (1 + st.beg) * bt->bidamt.amount <= bidamt.amount,  
                    "bid too small"
                  ); 
      //if ( it.stablecoin >= bidamt ) {
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
         //if it was a maker bid
      //}
      
   } else {
      if ( it.collateral.amount ) {
         add_balance( bt->bidder, it.collateral );
         cdpstable.modify( it, same_payer, [&]( auto& p ) {  
            p.collateral -= p.collateral;
         });
      } 
      if ( !it.stablecoin.amount ) {
         bidstable.erase(bt);
         cdpstable.erase(it);
      }
      else {
         feeds feedstable( _self, _self.value );
         const auto& ft = feedstable.get( symbol("VTO", 4).raw(), 
                                          "no price data" 
                                        );
         if ( it.stablecoin.amount > 0 ) {
            uint64_t amt = it.stablecoin.amount / ft.usd_per;
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = asset( amt, symbol("VTO", 4) );
            });   
         } 
         else if ( it.stablecoin.amount < 0 ) {
            asset stamt = -it.stablecoin;
            uint64_t amt = stamt.amount / ft.usd_per;
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = stamt;
               p.stablecoin = asset(amt, symbol("VTO", 4) );
            });
         }
         eosio_assert( bidamt.symbol == it.stablecoin.symbol, 
                       "bid is of wrong symbol" 
                     );
         bidstable.modify( bt, bidder, [&]( auto& b ) { 
            b.bidder = bidder;
            b.bidamt = bidamt;
            b.started = now();
            b.lastbid = now();
         });
      }
   }
}

void daemu::propose( name proposer, symbol_code symb, 
                     name clatrl, symbol_code cltrl,
                     name stabl, symbol_code stbl,
                     uint64_t max, uint64_t liq, 
                     uint64_t pen, uint64_t fee, 
                     uint32_t tau, uint32_t ttl,
                     uint64_t beg ) {  
   
   require_auth( proposer );
   is_account( clatrl ); 
   is_account( stabl );   
   eosio_assert( symb.is_valid(), 
                 "invalid cdp type symbol name" 
               );
   eosio_assert( cltrl.is_valid(),
                 "invalid collateral symbol name" 
               );
   eosio_assert( stbl.is_valid(), 
                 "invalid stablecoin symbol name" 
               );
   //TODO: improve validation
   eosio_assert( //tau < MAX_INT && TODO: different max for 32 bit ints
                 //ttl < MAX_INT &&
                 fee < MAX_INT && 
                 max < MAX_INT && 
                 liq < MAX_INT && liq >= 1000 //must be at least 100% collateralized
                 //cannot be more than 100% of collateral, measured in 10ths of a %
                 && pen <= 1000, "bad input" 
               );
   props propstable( _self, _self.value );
   auto prop_exist = propstable.find( symb.raw() );
   eosio_assert( prop_exist == propstable.end(), 
                 "proposal for this cdp type exists"
               );
   /* scope into owner because self scope may already have this symbol
    * if symbol exists in self scope and this proposal is voted into live
    * then original symbol would be erased, and symbol in proposer's scope
    * will be moved into self scope, and erased from prposer's scope
    */   
   stats statstable( _self, proposer.value ); 
   auto stat_exist = statstable.find( symb.raw() );
   eosio_assert( stat_exist == statstable.end(), 
                 "proposer already claimed this cdp type"
               );
   statstable.emplace( proposer, [&]( auto& t ) {
      t.tau = tau;
      t.ttl = ttl;
      t.beg = beg;
      t.cdp_type = symb;
      t.debt_ceiling = max;
      t.penalty_ratio = pen;
      t.liquid8_ratio = liq;
      t.fee_balance = asset( 0, symbol("VETO", 4));
      t.stability_fee = asset(fee, symbol("VETO", 4));
      t.total_stablecoin = asset(0, symbol(stbl, 4 ));
      t.total_collateral = asset(0, symbol(cltrl, 4 ));
      t.stabl_contract = stabl;
      t.clatrl_contract = clatrl;
   });
   propstable.emplace( proposer, [&]( auto& p ) {
      p.cdp_type = symb;
      p.proposer = proposer;
      p.expire = now() + VOTING_PERIOD;
      p.vote_no = asset(0, symbol("VTO", 4));
      p.vote_yes = asset(0, symbol("VTO", 4));
   }); 
   eosio::transaction txn{};
   txn.actions.emplace_back(  eosio::permission_level{ proposer, "active"_n },
                              _self, "referend"_n,
                              std::make_tuple(proposer, symb)
                           ); txn.delay_sec = VOTING_PERIOD;
   
   uint128_t txid = (((uint128_t) symb.raw()) << 32) + proposer.value;
   txn.send(txid, proposer, false); 
}

void daemu::referend( name proposer, symbol_code symb ) 
{  require_auth( proposer );
   eosio_assert( symb.is_valid(), "invalid symbol name" );
  
   stats pstable( _self, proposer.value );
   const auto& pst = pstable.get( symb.raw(), "no such proposal" );

   props propstable( _self, _self.value );
   const auto& prop = propstable.get( symb.raw(), "no such proposal" );
   eosio_assert( now() > prop.expire, "too soon to referend" );

   //the proposal was successfully voted on
   if ( prop.vote_yes > prop.vote_no ) {
      //implement proposed changes...
      stats statstable( _self, _self.value );
      auto stat_exist = statstable.find( symb.raw() );
      if ( stat_exist == statstable.end() )
         statstable.emplace( proposer, [&]( auto& t ) {
            t.live = true;
            t.tau = pst.tau;
            t.ttl = pst.ttl;
            t.beg = pst.beg;
            t.last_veto = now();
            t.cdp_type = pst.cdp_type;
            t.debt_ceiling = pst.debt_ceiling;
            t.stability_fee = pst.stability_fee;
            t.penalty_ratio = pst.penalty_ratio;
            t.liquid8_ratio = pst.liquid8_ratio;
            t.total_stablecoin = pst.total_stablecoin;
            t.total_collateral = pst.total_collateral;
            t.stabl_contract = pst.stabl_contract;
            t.clatrl_contract = pst.clatrl_contract;
         });
      else 
         statstable.modify( stat_exist, same_payer, [&]( auto& t ) {
            t.live = true;
            t.tau = pst.tau;
            t.ttl = pst.ttl;
            t.beg = pst.beg;
            t.last_veto = now();
            t.debt_ceiling = pst.debt_ceiling;
            t.stability_fee = pst.stability_fee;
            t.penalty_ratio = pst.penalty_ratio;
            t.liquid8_ratio = pst.liquid8_ratio;
         });
   } else if ( prop.vote_yes > prop.vote_no ) { //vote tie, so revote
      eosio::transaction txn{};
      txn.actions.emplace_back(  eosio::permission_level{ proposer, "active"_n },
                                 _self, "referend"_n,
                                 std::make_tuple(proposer, symb)
                              ); txn.delay_sec = VOTING_PERIOD;
      
      uint128_t txid = (((uint128_t) symb.raw()) << 32) + proposer.value;
      txn.send(txid, proposer, false); 
   } else {
      //now we refund everyone who voted with their VETO tokens
      accounts vacnts( _self, symb.raw() );
      for ( auto it = vacnts.begin(); it != vacnts.end(); ++it ) {
         add_balance( it->owner, asset( it->balance.amount, symbol("VTO", 4 )) );   
         vacnts.erase( it );
      }
      //now we can safely erase the proposal
      pstable.erase( pst );
      propstable.erase( prop );
   }
}

//TODO: replace
void daemu::upfeed( name ram_payer, symbol_code symb, bool down ) 
{  require_auth( ram_payer );

   feeds feedstable( _self, _self.value );
   auto fit = feedstable.find( symb.raw() );
   if ( fit == feedstable.end() ) {
      feedstable.emplace( ram_payer, [&]( auto& f ) {
         f.symb = symb;
         f.usd_per = 420;
         f.lastamp = now();
      }); 
   } else {
      if (down)
         feedstable.modify( fit, same_payer, [&]( auto& f ) {
            f.usd_per = 42;
            f.lastamp = now();
         });
      else
         feedstable.modify( fit, same_payer, [&]( auto& f ) {
            f.usd_per = 442;
            f.lastamp = now();
         });
   }
}

void daemu::transfer( name    from,
                      name    to,
                      asset   quantity,
                      string  memo ) {
   
   eosio_assert( from != to, "cannot transfer to self" );
   require_auth( from );
   eosio_assert( is_account( to ), "to account does not exist");
   auto sym = quantity.symbol.code();

   require_recipient( from );
   require_recipient( to );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

   logger_info("quantity.amount: ", quantity.amount);
   
   sub_balance( from, quantity );
   add_balance( to, quantity);
}

void daemu::sub_balance( name owner, asset value ) 
{  accounts from_acnts( _self, value.symbol.raw() );
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
{  accounts to_acnts( _self, value.symbol.raw() );
   const auto& to = to_acnts.get( owner.value, 
                                  "no balance object found" 
                                );
   to_acnts.modify( to, same_payer, [&]( auto& a ) {
      a.balance += value;
   });
}

void daemu::close( name owner, symbol_code symb ) 
{  require_auth( owner );
   accounts acnts( _self, symb.raw() );
   auto it = acnts.get( owner.value,
                        "no balance object found"
                      );
   eosio_assert( it.balance.amount == 0, 
                 "Cannot close because the balance is not zero." 
               );
   acnts.erase( it );
}

} /// namespace eosio

//checking all transfers, and not only from EOS system token
extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
      if (action == "transfer"_n.value && code != receiver) {
         eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosio::daemu::transfer);
      }
      if (code == receiver) {
         switch (action) { 
               EOSIO_DISPATCH_HELPER(eosio::daemu, (open)(close)(shut)(give)(lock)(bail)(draw)(wipe)(settle)(vote)(propose)(referend)(liquify)(upfeed)) 
         }    
      }
}

//EOSIO_DISPATCH( eosio::daemu, (open)(close)(shut)(give)(lock)(bail)(draw)(wipe)(settle)(transfer) )