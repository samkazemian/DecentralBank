#include <bond.hpp>
#include "../helpers.cpp"
#include <eosiolib/print.hpp>

void bond::create(account_name issuer,
                  asset maximum_supply) {
  require_auth(_self);

  auto symbol = maximum_supply.symbol;
  eosio_assert(symbol.is_valid(), "invalid symbol name");
  eosio_assert(maximum_supply.is_valid(), "invalid supply");
  eosio_assert(maximum_supply.amount > 0, "maximum supply must be positive");

  auto symbol_name = symbol.name();
  bondstats bond_stats_table(_self, symbol_name);
  auto existing = bond_stats_table.find(symbol_name);
  eosio_assert(existing == bond_stats_table.end(), "token with symbol already exists");

  bond_stats_table.emplace(_self, [&](auto& s) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply = maximum_supply;
    s.issuer = issuer;
  });
}

void bond::issue(account_name to,
                 asset quantity,
                 string memo) {
  auto symbol = quantity.symbol;
  eosio_assert(symbol.is_valid(), "invalid symbol name");
  eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

  auto symbol_name = symbol.name();
  bondstats bond_stats_table(_self, symbol_name);
  auto existing = bond_stats_table.find(symbol_name);
  eosio_assert(existing != bond_stats_table.end(), "token with symbol does not exist, create token before issue");
   const auto& st = *existing;

  require_auth(st.issuer);
  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must issue positive quantity");

  eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

  bond_stats_table.modify(st, 0, [&](auto& s) {
    s.supply += quantity;
  });

  add_balance(st.issuer, quantity, st.issuer);

  if (to != st.issuer) {
     SEND_INLINE_ACTION(*this,
                        transfer,
                        {st.issuer, N(active)},
                        {st.issuer, to, quantity, memo});
  }
}

void bond::transfer(account_name from,
                    account_name to,
                    asset quantity,
                    string memo) {
  eosio_assert(from != to, "cannot transfer to self");
  require_auth(from);
  eosio_assert(is_account(to), "to account does not exist");

  auto symbol_name = quantity.symbol.name();
  bondstats bond_stats_table(_self, symbol_name);
  const auto& st = bond_stats_table.get(symbol_name);

  require_recipient(from);
  require_recipient(to);

  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must transfer positive quantity");
  eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

  sub_balance(from, quantity);
  add_balance(to, quantity, from);
}

void bond::add_balance(account_name owner,
                       asset value,
                       account_name payer) {
  accounts accounts_table(_self, owner);
  auto to = accounts_table.find(value.symbol.name());

  if (to == accounts_table.end()) {
    accounts_table.emplace(payer, [&](auto& a) {
      a.balance = value;
    });
  } else {
    accounts_table.modify(to, 0, [&](auto& a) {
      a.balance += value;
    });
  }
}

void bond::sub_balance(account_name owner,
                        asset value) {
  accounts accounts_table(_self, owner);

  const auto& from = accounts_table.get(value.symbol.name(), "no balance object found");
  eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

  if (from.balance.amount == value.amount) {
    accounts_table.erase(from);
  } else {
    accounts_table.modify(from, owner, [&](auto& a) {
      a.balance -= value;
    });
  }
}

void bond::create_bond(const account_name account,
                       double face_value,
                       uint64_t maturity_date,
                       double interest_rate) {
  bonds bonds_table(_self, _self);

  bonds_table.emplace(account, [&](auto& b) {
    b.id = bonds_table.available_primary_key();
    b.account = account;
    b.face_value = face_value;
    b.maturity_date = maturity_date;
    b.interest_rate = interest_rate;
  });
}
