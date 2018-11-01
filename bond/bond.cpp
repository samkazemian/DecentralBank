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
