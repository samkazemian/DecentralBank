#include <eosiolib/eosio.hpp>

using namespace eosio;

class bond : public contract {
public:
  bond(account_name self) : contract(self) {}

  // @abi action
  void issue(const account_name account,
             uint32_t amount);
private:

  // @abi table bonds i64
  struct bondToken {
    account_name account;
    uint32_t     amount;

    account_name primary_key() const { return account; }
    EOSLIB_SERIALIZE(bondToken, (account)(amount))
  };

  typedef multi_index<N(bondToken), bondToken> _bonds;
};

EOSIO_ABI(bond, (issue))
