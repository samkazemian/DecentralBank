#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>
#include <sstream>

using namespace eosio;
using namespace std;

class verifier : public contract {
  public:
      using contract::contract;

      [[eosio::action]]
      void create(const string issuer,
                  asset maximum_supply) {
        require_auth(_self);

        auto symbol = maximum_supply.symbol;
        eosio_assert(symbol.is_valid(), "invalid symbol name");
        eosio_assert(maximum_supply.is_valid(), "invalid supply");
        eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

        stats statstable(_self, symbol.raw());
        print("Raw symbol: ");
        print(symbol.raw());
        auto existing = statstable.find(symbol.raw());
        eosio_assert(existing == statstable.end(), "token with symbol already exists");

        statstable.emplace(_self, [&](auto& s) {
          s.supply.symbol = maximum_supply.symbol;
          s.max_supply    = maximum_supply;
          s.issuer        = issuer;
        });
      }

      [[eosio::action]]
      void issue(const string to, asset quantity, const string memo) {
        require_auth(_self);

        auto symbol = quantity.symbol;
        eosio_assert(symbol.is_valid(), "invalid symbol name");
        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(_self, symbol.raw());
        auto existing = statstable.find(symbol.raw());
        eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
        const auto& st = *existing;

        eosio_assert(quantity.is_valid(), "invalid quantity");
        eosio_assert(quantity.amount > 0, "must issue positive quantity");

        eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify(st, _self, [&](auto& s) {
           s.supply += quantity;
        });

        add_balance(st.issuer, quantity);

        asset fee = asset(int64_t(0), symbol);
        if (to != st.issuer) {
          action(permission_level{ _self, "active"_n },
                 _self, "transfer"_n,
                 std::make_tuple(st.issuer, to, quantity, fee, memo)
                 ).send();
        }
      }

      [[eosio::action]]
      void transfer(const string pkeyFrom,
                    const string pkeyTo,
                    asset amount,
                    asset fee,
                    string memo) {
        require_auth(_self);
        eosio_assert(pkeyFrom != pkeyTo, "cannot transfer to self");

        capi_checksum256 digest;
        const char* data = "test_data";
        sha256(data, sizeof(data), &digest);

        const string signature = "SIG_K1_KfQ57wLFFiPR85zjuQyZsn7hK3jRicHXg4qETxLvxHQTHHejveGtiYBSx6Z68xBZYrY9Fihz74makocnSLQFBwaHTg6Aaa";
        eosio_assert(recover_key(digest, signature, 101, pkeyFrom, 53), "digest and signature do not match");

        auto sym = amount.symbol.raw();
        stats statstable(_self, sym);
        const auto& st = statstable.get(sym);

        eosio_assert(amount.is_valid(), "invalid quantity" );
        eosio_assert(fee.is_valid(), "invalid quantity" );
        eosio_assert(amount.amount > 0, "must transfer positive quantity");
        eosio_assert(fee.amount >= 0, "must transfer non negative quantity");
        eosio_assert(amount.symbol == st.supply.symbol, "symbol precision mismatch");
        eosio_assert(fee.symbol == st.supply.symbol, "symbol precision mismatch");
        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

        // once for the amount from to to
        sub_balance(pkeyFrom, amount);
        add_balance(pkeyTo, amount);

        // second time to give the relayer the fee
      }

  private:
    bool recover_key(const capi_checksum256 digest,
                     const string signature,
                     size_t siglen,
                     const string pub,
                     size_t publen) {
      // TODO
      // assert_recover_key(digest, signature, pub);
      return true;
    }

    void sub_balance(const string owner, asset value) {
      accounts from_acts(_self, _self.value);

      uint64_t v;
      istringstream iss(owner);
      iss >> v;

      const auto& from = from_acts.get(v, "no public key object found");
      eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

      if (from.balance.amount == value.amount) {
        from_acts.erase(from);
      } else {
        from_acts.modify(from, _self, [&]( auto& a ) {
          a.balance -= value;
        });
      }
    }

    void add_balance(const string recipientKey, asset value) {
      accounts to_acts(_self, _self.value);

      uint64_t v;
      istringstream iss(recipientKey);
      iss >> v;

      auto to = to_acts.find(v);

      if (to == to_acts.end()) {
        to_acts.emplace(_self, [&]( auto& a ){
          a.balance = value;
          a.publickey = recipientKey;
        });
      } else {
        to_acts.modify(to, _self, [&]( auto& a ) {
          a.balance += value;
        });
      }
    }

    struct [[eosio::table]] account {
      string publickey;
      asset balance;

      uint64_t primary_key() const {
        uint64_t v;
        istringstream iss(publickey);
        iss >> v;

        return v;
      }
    };

    struct [[eosio::table]] currstats {
      asset supply;
      asset max_supply;
      string issuer;

      uint64_t primary_key() const { return supply.symbol.raw(); }
    };

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stats"_n, currstats> stats;

};
EOSIO_DISPATCH(verifier, (create)(issue)(transfer))
