#!/bin/bash

CYAN='\033[1;36m'
NC='\033[0m'

EOSIO_CONTRACTS_ROOT=/home/ricardo/Documents/eosio.contracts/build
NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

alias cleos="cleos --url=${NODEOS_LOCATION}"

###############################################RICH ADDED###############################################
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
    # 0) Unlock wallet
    echo "=== UNLOCKING WALLET ==="
    cleos wallet unlock -n $WALLET --password $PASSWORD

    CONTRACT="daiqcontract"
    CONTRACT_WASM="$CONTRACT.wasm"
    CONTRACT_ABI="$CONTRACT.abi"

    OWNER_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
    ACTIVE_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
    # DAIQ Account public keys
    OWNER_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
    ACTIVE_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
    ## Make sure DAIQ keys are in the wallet
    cleos wallet import -n $WALLET --private-key $OWNER_ACCT
    cleos wallet import -n $WALLET --private-key $ACTIVE_ACCT
#///////////////////////////////////////////////RICH ADDED///////////////////////////////////////////////

    # EOSIO system-related keys
    echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
    cleos wallet import -n $WALLET --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
    cleos wallet import -n $WALLET --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
    cleos wallet import -n $WALLET --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
    cleos wallet import -n $WALLET --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
    cleos wallet import -n $WALLET --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
    cleos wallet import -n $WALLET --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
    cleos wallet import -n $WALLET --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
    cleos wallet import -n $WALLET --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
    cleos wallet import -n $WALLET --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
    cleos wallet import -n $WALLET --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF

    # Create system accounts
    echo -e "${CYAN}-----------------------SYSTEM ACCOUNTS-----------------------${NC}"
    cleos create account eosio eosio.bpay EOS7gFoz5EB6tM2HxdV9oBjHowtFipigMVtrSZxrJV3X6Ph4jdPg3
    cleos create account eosio eosio.msig EOS6QRncHGrDCPKRzPYSiWZaAw7QchdKCMLWgyjLd1s2v8tiYmb45
    cleos create account eosio eosio.names EOS7ygRX6zD1sx8c55WxiQZLfoitYk2u8aHrzUxu6vfWn9a51iDJt
    cleos create account eosio eosio.ram EOS5tY6zv1vXoqF36gUg5CG7GxWbajnwPtimTnq6h5iptPXwVhnLC
    cleos create account eosio eosio.ramfee EOS6a7idZWj1h4PezYks61sf1RJjQJzrc8s4aUbe3YJ3xkdiXKBhF
    cleos create account eosio eosio.saving EOS8ioLmKrCyy5VyZqMNdimSpPjVF2tKbT5WKhE67vbVPcsRXtj5z
    cleos create account eosio eosio.stake EOS5an8bvYFHZBmiCAzAtVSiEiixbJhLY8Uy5Z7cpf3S9UoqA3bJb
    cleos create account eosio eosio.token EOS7JPVyejkbQHzE9Z4HwewNzGss11GB21NPkwTX2MQFmruYFqGXm
    cleos create account eosio eosio.vpay EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U

    
    # Bootstrap new system contracts
    echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
    cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
    cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/
    cleos push action eosio.token create '[ "eosio", "10000000000.0000 EOS" ]' -p eosio.token
    cleos push action eosio.token create '[ "eosio", "10000000000.0000 SYS" ]' -p eosio.token
    echo -e "      EOS TOKEN CREATED"
    cleos push action eosio.token issue '[ "eosio", "1000000000.0000 EOS", "memo" ]' -p eosio
    cleos push action eosio.token issue '[ "eosio", "1000000000.0000 SYS", "memo" ]' -p eosio
    echo -e "      EOS TOKEN ISSUED"
    cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.bios/
    echo -e "      BIOS SET"
    cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
    echo -e "      SYSTEM SET"
    cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio@active
    cleos push action eosio init '[0, "4,EOS"]' -p eosio@active
    cleos push action eosio init '[0, "4,SYS"]' -p eosio@active

    cleos create account eosio $CONTRACT $OWNER_KEY $ACTIVE_KEY

    # Import user keys
    echo -e "${CYAN}-----------------------USER KEYS-----------------------${NC}"
    cleos wallet import -n $WALLET --private-key 5JVvgRBGKXSzLYMHgyMFH5AHjDzrMbyEPRdj8J6EVrXJs8adFpK # everipediaiq
   
    cleos wallet import -n $WALLET --private-key 5J9UYL9VcDfykAB7mcx9nFfRKki5djG9AXGV6DJ8d5XPYDJDyUy # eptestusersa
    cleos wallet import -n $WALLET --private-key 5HtnwWCbMpR1ATYoXY4xb1E4HAU9mzGvDrawyK5May68cYrJR7r # eptestusersb
    cleos wallet import -n $WALLET --private-key 5Jjx6z5SJ7WhVU2bgG2si6Y1up1JTXHj7qhC9kKUXPXb1K1Xnj6 # eptestusersc

    # Create user accounts
    echo -e "${CYAN}-----------------------USER ACCOUNTS-----------------------${NC}"
    cleos system newaccount eosio everipediaiq EOS6XeRbyHP1wkfEvFeHJNccr4NA9QhnAr6cU21Kaar32Y5aHM5FP --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

###############################################RICH ADDED###############################################
    cleos system newaccount eosio $CONTRACT $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
    cleos system newaccount eosio rick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
    cleos system newaccount eosio dick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
#///////////////////////////////////////////////RICH ADDED///////////////////////////////////////////////

    cleos system newaccount eosio eptestusersa EOS6HfoynFKZ1Msq1bKNwtSTTpEu8NssYMcgsy6nHqhRp3mz7tNkB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
    cleos system newaccount eosio eptestusersb EOS68s2PrHPDeGWTKczrNZCn4MDMgoW6SFHuTQhXYUNLT1hAmJei8 --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
    cleos system newaccount eosio eptestusersc EOS7LpZDPKwWWXgJnNYnX6LCBgNqCEqugW9oUQr7XqcSfz7aSFk8o --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

    # Deploy eosio.wrap
    echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
    cleos wallet import -n $WALLET --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
    cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
    cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
    cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/

    # Transfer EOS to testing accounts
    echo -e "${CYAN}-----------------------TRANSFERRING IQ-----------------------${NC}"
    
###############################################RICH ADDED###############################################
    
    cleos transfer eosio rick "1000 EOS"
    cleos transfer eosio dick "1000 EOS"

    ## Deploy contracts
    echo -e "${CYAN}-----------------------DEPLOYING EVERIPEDIA CONTRACTS-----------------------${NC}"
    cleos set contract everipediaiq . everipediaiq.wasm everipediaiq.abi
    cleos set contract $CONTRACT . $CONTRACT_WASM $CONTRACT_ABI

    cleos set account permission $CONTRACT active ./perm.json -p $CONTRACT@active
    cleos set account permission everipediaiq active '{ "threshold": 1, "keys": [{ "key": "EOS6XeRbyHP1wkfEvFeHJNccr4NA9QhnAr6cU21Kaar32Y5aHM5FP", "weight": 1 }], "accounts": [{ "permission": { "actor":"everipediaiq","permission":"eosio.code" }, "weight":1 }] }' owner -p everipediaiq

#///////////////////////////////////////////////RICH ADDED///////////////////////////////////////////////        

    # Create and issue token
    echo -e "${CYAN}-----------------------CREATING IQ TOKEN-----------------------${NC}"
    cleos push action everipediaiq create '["everipediaiq", "100000000000.000 IQ"]' -p everipediaiq@active
    cleos push action everipediaiq issue '["everipediaiq", "10000000000.000 IQ", "initial supply"]' -p everipediaiq@active


# Test IQ transfers
echo -e "${CYAN}-----------------------MOVING TOKENS FROM THE CONTRACT TO SOME USERS -----------------------${NC}"

###############################################RICH ADDED###############################################
cleos push action everipediaiq transfer '["everipediaiq", "rick", "10000.000 IQ", "test"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dick", "10000.000 IQ", "test"]' -p everipediaiq

cleos push action everipediaiq transfer '["rick", "daiqcontract", "10000.000 IQ", "test"]' -p rick
cleos push action everipediaiq transfer '["dick", "daiqcontract", "10000.000 IQ", "test"]' -p dick

cleos push action eosio.token transfer '["rick", "daiqcontract", "10000.0000 EOS", "test"]' -p rick
cleos push action eosio.token transfer '["dick", "daiqcontract", "10000.0000 EOS", "test"]' -p dick

# verify that balances have entry for tracking rick's tokens
cleos get table $CONTRACT "IQ" accounts # verify first run

# verify second run
# cleos get table $CONTRACT "USD" accounts
# cleos get table $CONTRACT "EOS" accounts
# cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# SHUT...uncomment all other actions below to test this
#echo "=== Closing CDP  ==="
#cleos push action $CONTRACT shut '["rick", "FUD"]' -p rick

# verify that rick's cdp was in fact shut
#cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# PROPOSE
echo "=== Proposing new CDP type  ==="

cleos push action $CONTRACT propose '["rick", "FUD", "EOS", "USD", 2000, 2000000000, 50, 10, 20, 150, 30, 5, "dick"]' -p rick

#propose global settlement 
#cleos push action $CONTRACT propose '["rick", "FUD", "EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick"]' -p rick

# verify that stats have temporary entry for storing rick's proposed cdp type
cleos get table $CONTRACT rick stat

#=================================================================================#
# VOTE
echo "=== Voting FOR proposal ===" 

cleos push action $CONTRACT vote '["rick", "FUD", false, "0.001 IQ" ]' -p rick

# vote against the proposal, for testing tie
#cleos push action $CONTRACT vote '["rick", "FUD", true, "0.001 IQ" ]' -p rick

# verify that balances have entry for tracking rick's vote
cleos get table $CONTRACT "FUD" accounts
cleos get table $CONTRACT $CONTRACT prop

echo "=== Waiting for referendum... ==="
# wait for referended to get called automatically via deferred transaction
sleep 10

# or call referended manually...
#cleos push action $CONTRACT referended '["rick", "FUD"]' -p daiqcontract

# for tie
#cleos push action $CONTRACT vote '["rick", "FUD", true, "0.002 IQ" ]' -p rick
#sleep 10

# verify that the vote tokens were refunded to rick after referended
cleos get table $CONTRACT "FUD" accounts
# verify that prop entry was deleted
cleos get table $CONTRACT $CONTRACT prop
# verify that stat entry for this cdp type was referended into existence
cleos get table $CONTRACT $CONTRACT stat

#=================================================================================#
# CREATE PRICE FEEDS
echo "=== Creating price feeds ==="

cleos push action $CONTRACT upfeed '["dick", "1.00 USD", "FUD", "EOS"]' -f -p dick
cleos push action $CONTRACT upfeed '["daiqcontract", "1.00 USD", "FUD", "IQ"]' -f -p daiqcontract
cleos push action $CONTRACT upfeed '["daiqcontract", "1.00 USD", "FUD", "USD"]' -f -p daiqcontract

# verify that price was updated
cleos get table $CONTRACT $CONTRACT feed

#=================================================================================#
# GIVE
#echo "=== Giving CDP  to dick==="

#cleos push action $CONTRACT give '["rick", "dick" "FUD"]' -p rick
# verify that rick's cdp was transfered to dick
#cleos get table $CONTRACT "FUD" cdp

#echo "=== Giving CDP back to rick  ==="
#cleos push action $CONTRACT open '["dick", "FUD", "dick"]' -p dick
#cleos push action $CONTRACT give '["dick", "rick" "FUD"]' -p dick

# OPEN
cleos push action $CONTRACT open '["rick", "FUD", "rick"]' -p rick

# verify that dick's cdp was transfered back to rick
cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# LOCK
echo "=== Push collateral ==="

cleos push action $CONTRACT lock '["rick", "FUD", "6.0000 EOS"]' -p rick

# verify that rick's clatrl balance was updated
cleos get table $CONTRACT "EOS" accounts
# verify that clatrl amount was pushed
cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# DRAW
echo "=== Pull Dai ==="

cleos push action $CONTRACT draw '["rick", "FUD", "4.00 USD"]' -p rick

# verify that stabl amount was pulled
cleos get table $CONTRACT "FUD" cdp
# verify that rick's stabl balance was updated
cleos get table $CONTRACT "USD" accounts

#=================================================================================#
# WIPE 
echo "=== Push Dai ==="

cleos push action $CONTRACT wipe '["rick", "FUD", "2.00 USD", "0.001 IQ"]' -p rick

# verify that stabl amount was pushed
cleos get table $CONTRACT "FUD" cdp
#verify that rick's stabl balance was updated
cleos get table $CONTRACT "USD" accounts

#=================================================================================#
#BAIL
echo "=== Pull collateral ==="

cleos push action $CONTRACT bail '["rick", "FUD", "1.0000 EOS"]' -p rick

# verify that clatrl amount was pulled
cleos get table $CONTRACT "FUD" cdp
# verify that rick's clatrl balance was updated
cleos get table $CONTRACT "EOS" accounts

sleep 2

#=================================================================================#
# UPDATE EOS PRICE FEED - DOWN
echo "=== Make EOS price go down ==="

cleos push action $CONTRACT upfeed '["dick", "0.01 USD", "FUD", "EOS"]' -f -p dick

#=================================================================================#
# LIQUIFY
echo "=== Liquifying rick's cdp ==="

cleos get table $CONTRACT "FUD" cdp
#verify rick's balances during liquidation
cleos get table $CONTRACT "USD" accounts


cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "2.25 USD"]' -p dick
sleep 1
cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "2.75 USD"]' -p dick

# wait for round to expire and switch to selling off IQ for the remaining balance
echo "=== First round auction done, waiting for round to expire... ==="
cleos get table $CONTRACT "FUD" cdp
sleep 5

# in order to test round 3, change the amount here to and uncomment below
cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "0.25 USD"]' -p dick
sleep 5

echo "=== Second round auction mid ==="
#cleos get table $CONTRACT "FUD" cdp

#cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "1.00 USD"]' -p dick

# auction covered more than the cdp debt, proceed to take bids in IQ for the diff
# echo "=== Second round auction done, waiting for round to expire... ==="
# cleos get table $CONTRACT "FUD" cdp
# sleep 5

# cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "0.200 IQ"]' -p dick
# sleep 1
# cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "0.400 IQ"]' -p dick
# sleep 1

# echo "=== Third round auction mid ==="
# cleos get table $CONTRACT "FUD" cdp

# cleos push action $CONTRACT liquify '["dick", "rick", "FUD", "0.500 IQ"]' -p dick

# echo "=== Third round auction done, balance closed ==="

#verify that bid entry and cdp were in fact erased
cleos get table $CONTRACT "FUD" cdp
cleos get table $CONTRACT "FUD" bid
#verify rick's balances after liquidation
cleos get table $CONTRACT "USD" accounts
cleos get table $CONTRACT "EOS" accounts
cleos get table $CONTRACT "IQ" accounts

#=================================================================================#
# SETTLE
echo "=== Global settlement ==="

cleos push action $CONTRACT settle '["FUD"]' -p daiqcontract
#verify that FUD cdp type is not live
cleos get table $CONTRACT $CONTRACT stat

#///////////////////////////////////////////////RICH ADDED///////////////////////////////////////////////

echo -e "${CYAN}-----------------------COMPLETE-----------------------${NC}"

# Exit on success
exit 0