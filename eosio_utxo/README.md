# eosio_utxo

## Demo

### Setup
```
cleos create account eosio relayer <account_public_key>
eosio-cpp -w -o verifier.wasm verifier.cpp
```

### Create asset
```
cleos set contract relayer ../verifier
cleos push action relayer create '["main", "100 UTXO"]' -p relayer
cleos get table relayer 340784338176 stats
```

### Issue assets

Generated public/private key pairs from CLI:

```
Private key: 5KjibBBRKcxTWYUsipoS9MftgoqA38TvBT6QcAkUzd5RxMoqFEe
Public key: EOS8S4YmB2fPSZDopZ2pCHFy3p4gCmwHipo4NqwTAgLuggnqRHBAp
```

```
Private key: 5K7iESK2inXeYUApTwFiKobWxHownf1bJbtm5Pm6SuBe1HshcGu
Public key: EOS7PcgxVxfBLSFMhrgYn7LtvP8eLg2BYjCx5yesGw2bdoiabWoY7
```

```
cleos push action relayer issue '["EOS8S4YmB2fPSZDopZ2pCHFy3p4gCmwHipo4NqwTAgLuggnqRHBAp", "1 UTXO", "issue to first account"]' -p relayer
cleos push action relayer issue '["EOS7PcgxVxfBLSFMhrgYn7LtvP8eLg2BYjCx5yesGw2bdoiabWoY7", "5 UTXO", "issue to second account"]' -p relayer
cleos get table relayer relayer accounts
```

### Transfer assets

```
cleos push action relayer transfer '["EOS7PcgxVxfBLSFMhrgYn7LtvP8eLg2BYjCx5yesGw2bdoiabWoY7", "EOS8S4YmB2fPSZDopZ2pCHFy3p4gCmwHipo4NqwTAgLuggnqRHBAp", "2 UTXO", "0 UTXO", "transfer from first account to second account"]' -p relayer
cleos get table relayer relayer accounts
```
