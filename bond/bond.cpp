#include <bond.hpp>
#include "../helpers.cpp"

void bond::issue(const account_name account,
                 double face_value,
                 uint64_t maturity_date,
                 double interest_rate) {
  require_auth(account);

  _accounts accounts(_self, _self);

  auto iterator = accounts.find(account);

  if (iterator == accounts.end()) {
    accounts.emplace(account, [&](auto& a) {
      a.account = account;
      a.amount = 1;

      create_bond(account, face_value, maturity_date, interest_rate);
    });
  } else {
    accounts.modify(iterator, account, [&](auto& a) {
      a.amount++;

      create_bond(account, face_value, maturity_date, interest_rate);
    });
  }
}

void bond::create_bond(const account_name account,
                       double face_value,
                       uint64_t maturity_date,
                       double interest_rate) {
  _bonds bonds(_self, _self);

  bonds.emplace(account, [&](auto& b) {
    b.id = bonds.available_primary_key();
    b.account = account;
    b.face_value = face_value;
    b.maturity_date = maturity_date;
    b.interest_rate = interest_rate;
  });
}
