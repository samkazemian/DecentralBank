#include <bond.hpp>
#include "../helpers.cpp"

void bond::create(const account_name account,
                  const string& username,
                  const string& bio,
                  uint32_t age) {
  require_auth(account);

  print(_self);
  print(name{_self});
  _profiles profile(_self, _self);

  auto iterator = profile.find(account);

  eosio_assert(iterator == profile.end(), "Account already has profile");

  profile.emplace(_self, [&](auto& p) {
    p.account = account;
    p.username = username;
    p.bio = bio;
    p.age = age;
  });

  print(name{account}, " profile created");
}

void bond::remove(const account_name account) {
  require_auth(account);

  _profiles profile(_self, _self);

  auto iterator = profile.find(account);

  eosio_assert(iterator != profile.end(), "Account does not have a profile");

  profile.erase(iterator);
  print(name{account}, " profile removed");
}
