#!/bin/bash

#=================================================================================#
# Config Constants

# Helper scripts
COMPILE="./compile.sh"
DEPLOY="./deploy.sh"

# Contract accounts
CONTRACT="daiqcontract"

USER_1="$1"
USER_2="$2"

if ["$USER_1" == ""] || [ "$USER_2" == "" ]; then
    echo "Usage: ./testnet.sh [test_account_name_1] [test_account_name_2]"
fi

# Wallet name prereq... 
# set WALLET environment variable via: 

# rm ~/eosio-wallet/default.wallet
# cleos wallet create --to-console
# export wallet="default"

if [[ -z "${WALLET}" ]]; then
  echo "ERROR: run 'export WALLET=[wallet_name] to set your wallet name'"
  exit 1
else
  WALLET="${WALLET}"
fi

# Wallet password prereq...
# set PASSWORD environment variable via:
# export PASSWORD="PW5JjGjm4FjESLYWUqgpEP4sfVCFoCLfNRccSv2XZNd4cgBJBXnDV"

if [[ -z "${PASSWORD}" ]]; then
  echo "ERROR: run 'export PASSWORD=[password] to set your wallet password'"
  exit 1
else
  PASSWORD="${PASSWORD}"
fi

# Account Private keys, first one for eosio
EOSIO_ACCT="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"

# To create new keys run:
# cleos create key --to-console

OWNER_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
ACTIVE_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
# Account public keys
OWNER_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
ACTIVE_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"

## Make sure keys are in the wallet
cleos wallet import -n $WALLET --private-key $EOSIO_ACCT 
cleos wallet import -n $WALLET --private-key $OWNER_ACCT
cleos wallet import -n $WALLET --private-key $ACTIVE_ACCT

# Unlock wallet
echo "=== UNLOCKING WALLET ==="
cleos wallet unlock -n $WALLET --password $PASSWORD

# 3) Compile and Deploy contracts
echo "=== COMPILING CONTRACTS ==="
"$COMPILE" $CONTRACT
echo "=== DEPLOYING CONTRACTS ==="
"$DEPLOY" $CONTRACT -j


#=================================================================================#
# OPEN
echo "=== Opening CDP/Balances  ==="

# on first run, when given cdp type doesn't exist yet, 
# "open" will only pre-fund $USER_1 with voting tokens
# and trigger an assert without executing to completion
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT open '["$USER_1", "FUD", "$USER_1"]' -p $USER_1

# verify that balances have entry for tracking $USER_1's tokens
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "VTO" accounts # verify first run

# verify second run
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "USD" accounts
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "EOS" accounts
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

#=================================================================================#
# SHUT...uncomment all other actions below to test this
#echo "=== Closing CDP  ==="
#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT shut '["$USER_1", "FUD"]' -p $USER_1

# verify that $USER_1's cdp was in fact shut
#cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

#=================================================================================#
# PROPOSE
echo "=== Proposing new CDP type  ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT propose '["$USER_1", "FUD", "EOS", "USD", 2000, 2000000000, 1, 1, 30, 5, 0.20, 1.5]' -p $USER_1

#propose global settlement 
#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT propose '["rick", "FUD", "EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0]' -p rick

# verify that stats have temporary entry for storing $USER_1's proposed cdp type
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $USER_1 stat

#=================================================================================#
# VOTE
echo "=== Voting FOR proposal ===" 

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT vote '["$USER_1", "FUD", false, "0.0001 VTO" ]' -p $USER_1

# vote against the proposal, for testing tie
#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT vote '["$USER_1", "FUD", true, "0.0001 VTO" ]' -p $USER_1

# verify that balances have entry for tracking $USER_1's vote
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" accounts
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $CONTRACT prop

echo "=== Waiting for referendum... ==="
# wait for referended to get called automatically via deferred transaction
sleep 10

# or call referended manually...
#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT referended '["$USER_1", "FUD"]' -p daiq

# for tie
#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT vote '["$USER_1", "FUD", true, "0.0002 VTO" ]' -p $USER_1
#sleep 10

# verify that the vote tokens were refunded to $USER_1 after referended
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" accounts
# verify that prop entry was deleted
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $CONTRACT prop
# verify that stat entry for this cdp type was referended into existence
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $CONTRACT stat

# on second run, "open" will create a cdp for $USER_1 of given type
# and create relevant balance entries if they dont exist
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT open '["$USER_1", "FUD", "$USER_1"]' -p $USER_1

#=================================================================================#
# GIVE
echo "=== Giving CDP  to USER_2 ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT give '["$USER_1", "$USER_2" "FUD"]' -p $USER_1
# verify that $USER_1's cdp was transfered to $USER_2
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp


echo "=== Giving CDP back to USER_1  ==="
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT open '["$USER_2", "FUD", "$USER_2"]' -p $USER_2
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT give '["$USER_2", "$USER_1" "FUD"]' -p $USER_2
# verify that $USER_2's cdp was transfered back to $USER_1
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

#=================================================================================#
# CREATE PRICE FEEDS
echo "=== Make price go up ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT upfeed '["EOS", false]' -f -p daiq
# create price feed for VTO and FUD (1:1)
echo "=== Init a price feed for VTO ==="
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT upfeed '["VTO", false]' -f -p daiq
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT upfeed '["USD", false]' -f -p daiq

# verify that price was updated
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $CONTRACT feed

#=================================================================================#
# LOCK
echo "=== Push collateral ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT lock '["$USER_1", "FUD", "6.0000 EOS"]' -p $USER_1

# verify that $USER_1's clatrl balance was updated
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "EOS" accounts
# verify that clatrl amount was pushed
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

#=================================================================================#
# DRAW
echo "=== Pull Dai ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT draw '["$USER_1", "FUD", "4.00 USD"]' -p $USER_1

# verify that stabl amount was pulled
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
# verify that $USER_1's stabl balance was updated
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "USD" accounts

#=================================================================================#
# WIPE 
echo "=== Push Dai ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT wipe '["$USER_1", "FUD", "2.00 USD"]' -p $USER_1

# verify that stabl amount was pushed
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
#verify that $USER_1's stabl balance was updated
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "USD" accounts

#=================================================================================#
#BAIL
echo "=== Pull collateral ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT bail '["$USER_1", "FUD", "1.0000 EOS"]' -p $USER_1

# verify that clatrl amount was pulled
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
# verify that $USER_1's clatrl balance was updated
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "EOS" accounts

#=================================================================================#
# UPDATE EOS PRICE FEED - DOWN
echo "=== First, make price go down ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT upfeed '["EOS", true]' -f -p daiq

#=================================================================================#
# LIQUIFY
echo "=== Liquifying USER_1's cdp ==="

cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
#verify $USER_1's balances during liquidation
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "USD" accounts


cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "1.25 USD"]' -p $USER_2
sleep 1
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "1.50 USD"]' -p $USER_2

# wait for round to expire and switch to selling off VTO for the remaining balance
echo "=== First round auction done, waiting for round to expire... ==="
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
sleep 5

# in order to test round 3, change the amount here to 0.25 and uncomment below
cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "0.50 USD"]' -p $USER_2
sleep 5

echo "=== Second round auction mid ==="
#cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

#cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "1.00 USD"]' -p $USER_2

# auction covered more than the cdp debt, proceed to take bids in VTO for the diff
# echo "=== Second round auction done, waiting for round to expire... ==="
# cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
# sleep 5

# cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "0.2000 VTO"]' -p $USER_2
# sleep 1
# cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "0.4000 VTO"]' -p $USER_2
# sleep 1

# echo "=== Third round auction mid ==="
# cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp

# cleos --url https://jungle.eosio.cr:443 push action $CONTRACT liquify '["$USER_2", "$USER_1", "FUD", "0.5000 VTO"]' -p $USER_2

# echo "=== Third round auction done, balance closed ==="

#verify that bid entry and cdp were in fact erased
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" cdp
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "FUD" bid
#verify $USER_1's balances after liquidation
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "USD" accounts
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "EOS" accounts
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT "VTO" accounts

#=================================================================================#
# SETTLE
echo "=== Global settlement ==="

cleos --url https://jungle.eosio.cr:443 push action $CONTRACT settle '["FUD"]'
#verify that FUD cdp type is not live
cleos --url https://jungle.eosio.cr:443 get table $CONTRACT $CONTRACT stat

# Exit on success
exit 0
