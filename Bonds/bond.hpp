#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosio.system/native.hpp>

using namespace eosio;
using namespace std;

class bond : public contract {
public:
  bond(account_name self) : contract(self) {}

  // @abi action
  void create(account_name issuer,
              asset maximum_supply);

  // @abi action
  void issue(account_name to,
             asset quantity,
             string memo);

  // @abi action
  void transfer(account_name from,
                account_name to,
                asset quantity,
                string memo);

private:
  void create_bond(const account_name account,
                   double face_value,
                   uint64_t maturity_date,
                   double interest_rate);

  // @abi table accounts i64
  struct account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.name(); }
    EOSLIB_SERIALIZE(account, (balance))
  };

  // @abi table bondstats i64
  struct bond_stat {
    asset supply;
    asset max_supply;
    account_name issuer;

    uint64_t primary_key() const { return supply.symbol.name(); }
    EOSLIB_SERIALIZE(bond_stat, (supply)(max_supply)(issuer))
  };

  // @abi table bonds i64
  struct bond_token {
    uint64_t id;
    account_name account;

    double face_value;
    uint64_t maturity_date;
    double interest_rate;

    uint64_t primary_key() const { return id; }
    EOSLIB_SERIALIZE(bond_token, (id)(account)(face_value)(maturity_date)(interest_rate))
  };

  typedef multi_index<N(accounts), account> accounts;
  typedef multi_index<N(bondstats), bond_stat> bondstats;
  typedef multi_index<N(bonds), bond_token> bonds;

  void add_balance(account_name owner,
                   asset value,
                   account_name payer);
  void sub_balance(account_name owner,
                   asset value);
};

EOSIO_ABI(bond, (create)(issue)(transfer))
