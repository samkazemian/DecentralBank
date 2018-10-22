#include <eosiolib/eosio.hpp>
#include <eosio.system/native.hpp>

using namespace eosio;

class bond : public contract {
public:
  bond(account_name self) : contract(self) {}

  // @abi action
  void issue(const account_name account,
             double face_value,
             uint64_t maturity_date,
             double interest_rate);
private:

  // @abi table bonds i64
  struct bond_token {
    uint64_t id;
    account_name account;

    double face_value;
    uint64_t maturity_date;
    double interest_rate;

    uint64_t primary_key() const { return id; }
    account_name get_account_name() const { return account; }
    EOSLIB_SERIALIZE(bond_token, (id)(account)(face_value)(maturity_date)(interest_rate))
  };

  // @abi table accounts i64
  struct account_token {
    account_name      account;
    uint64_t          amount;

    account_name primary_key() const { return account; }
    EOSLIB_SERIALIZE(account_token, (account)(amount))
  };

  typedef multi_index<N(accounts), account_token> _accounts;
  typedef multi_index<N(bonds), bond_token,
                      indexed_by<N(account_token), const_mem_fun<bond_token, account_name, &bond_token::get_account_name> >
    > _bonds;
};

EOSIO_ABI(bond, (issue))
