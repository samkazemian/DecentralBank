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
    });
  } else {
    accounts.modify(iterator, account, [&](auto& a) {
      a.amount++;
    });
  }
}
