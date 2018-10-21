#include <bond.hpp>
#include "../helpers.cpp"

void bond::issue(const account_name account,
                 uint32_t amount) {
  require_auth(account);

  _bonds bond(_self, _self);

  auto iterator = bond.find(account);

  if (iterator == bond.end()) {
    bond.emplace(account, [&](auto& b) {
      b.account = account;
      b.amount = amount;
    });
  } else {
    bond.modify(iterator, account, [&](auto& b) {
      b.amount += amount;
    });
  }
}
