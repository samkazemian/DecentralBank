#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

#include <string>

using namespace eosio;
using std::string;

class bond : public contract {
public:
  bond(account_name self) : contract(self) {}

  // @abi action
  void create(const account_name account,
              const string& username,
              const string& bio,
              uint32_t age);

  // @abi action
  void remove(const account_name account);

private:

  // @abi table profile i64
  struct profile {
    account_name account;
    string       username;
    string       bio;
    uint32_t     age;

    account_name primary_key() const { return account; }
    EOSLIB_SERIALIZE(profile, (account)(username)(bio)(age))
  };

  typedef multi_index<N(profile), profile> _profiles;
};

EOSIO_ABI(bond, (create)(remove))
