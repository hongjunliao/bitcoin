# bitcoin 自定义本地signet网络, miner使用contrib/signet/miner,给出完整流程

## 1.定义 Signet 需要一个私钥来控制出块权。先用临时 Regtest 环境生成密钥：
bitcoind -regtest -datadir=/tmp/signettmp/
bitcoin-cli -regtest -datadir=/tmp/signettmp/ createwallet ""
{
  "name": ""
}
bitcoin-cli -regtest -datadir=/tmp/signettmp/ listdescriptors true
{
  "wallet_name": "",
  "descriptors": [
    {
      "desc": "pkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/44h/1h/0h/0/*)#lzr6nts9",
      "timestamp": 1791771532,
      "active": true,
      "internal": false,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "pkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/44h/1h/0h/1/*)#wkxmw7qa",
      "timestamp": 1791771532,
      "active": true,
      "internal": true,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "sh(wpkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/49h/1h/0h/0/*))#e47y9vlw",
      "timestamp": 1791771532,
      "active": true,
      "internal": false,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "sh(wpkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/49h/1h/0h/1/*))#lkkp7p56",
      "timestamp": 1791771532,
      "active": true,
      "internal": true,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "tr(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/86h/1h/0h/0/*)#juvftts5",
      "timestamp": 1791771532,
      "active": true,
      "internal": false,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "tr(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/86h/1h/0h/1/*)#rgfgk7qv",
      "timestamp": 1791771532,
      "active": true,
      "internal": true,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "wpkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/84h/1h/0h/0/*)#cuq9mzr5",
      "timestamp": 1791771532,
      "active": true,
      "internal": false,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
    {
      "desc": "wpkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/84h/1h/0h/1/*)#fg9yxhnv",
      "timestamp": 1791771532,
      "active": true,
      "internal": true,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    }
  ]
}
### 从你提供的 listdescriptors 输出中，选取一个包含 tprv 私钥的外部（internal: false）描述符。wpkh 或 pkh 类型均可，推荐使用 wpkh 以获得更好的兼容性。
DESC="wpkh(tprv8ZgxMBicQKsPeGRGcwMJh2jT4vCWJqZshKqSjzYwkU1A6F3TJJctVf2oLb4U1zLghnuuqCvg6vRX5hPJUs2HpPu2dWarzVwM7gQZGSnVSQt/84h/1h/0h/0/*)#cuq9mzr5"

## 导入描述符到 Signet 钱包
bitcoind -signet -datadir=/home/jun/bitcoin/datadir/defsignet/
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ createwallet ""
{
"name": ""
}
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ importdescriptors "[{\"desc\":\"$DESC\",\"timestamp\":\"now\",\"active\":true,\"range\":[0,100],\"internal\":false}]"
[
  {
    "success": true
  }
]
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ getdescriptorinfo $DESC
{
  "descriptor": "wpkh(tpubD6NzVbkrYhZ4XjT4Wb1u6SPZdwiSUAknGdSE2WbFAjoYvjJDvhSUg9efWhfjE2xdYjbWfsacmMwYYtuPBb8MhGa7ENAX4WSTNpVEFvPDvB7/84h/1h/0h/0/*)#5fjpuqjl",
  "checksum": "cuq9mzr5",
  "isrange": true,
  "issolvable": true,
  "hasprivatekeys": true
}

bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ deriveaddresses $DESC "[0,0]"
[
  "tb1qa4c0588sn2h0plapsm7lcude3pzuhqps9t66qq"
]

bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ getaddressinfo tb1qa4c0588sn2h0plapsm7lcude3pzuhqps9t66qq
{
  "address": "tb1qa4c0588sn2h0plapsm7lcude3pzuhqps9t66qq",
  "scriptPubKey": "0014ed70fa1cf09aaef0ffa186fdfc71b98845cb8030",
  "ismine": true,
  "solvable": true,
  "desc": "wpkh([68ce5802/84h/1h/0h/0/0]03b310949035ca716b15db45f343fcb4047f33b997da13e75942347e54207bb947)#y877akzz",
  "parent_desc": "wpkh([68ce5802/84h/1h/0h]tpubDDTE27m4E2bphEM2WVToSJkwo36FMk2rFfwcCVnQDELpm19qfCvhAULpqn78H9yoXbZjHeuJKG8L6DD5jXet5FhKFYxtLHCwcFrRe29sHLV/0/*)#9vw0zpmh",
  "iswatchonly": false,
  "isscript": false,  
  "iswitness": true,
  "witness_version": 0,
  "witness_program": "ed70fa1cf09aaef0ffa186fdfc71b98845cb8030",
  "pubkey": "03b310949035ca716b15db45f343fcb4047f33b997da13e75942347e54207bb947",
  "ischange": true,
  "timestamp": 1732349202,
  "hdkeypath": "m/84h/1h/0h/0/0",
  "hdseedid": "0000000000000000000000000000000000000000",
  "hdmasterfingerprint": "68ce5802",
  "labels": [
  ]
}

## 构造bitcoin.conf/signetchallenge, signetchallenge=0014<20字节公钥哈希>
#直接设置为scriptPubKey
signetchallenge=0014ed70fa1cf09aaef0ffa186fdfc71b98845cb8030

## 使用 contrib/signet/miner 挖矿
./contrib/signet/miner  calibrate --grind-cmd="bitcoin-util grind"
nbits=1d0d8962 for 25s average mining time
./contrib/signet/miner --cli="bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/"   generate   --grind-cmd="bitcoin-util grind"   --descriptor=$DESC   --nbits=1d0d8962
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ getbalances
{
"mine": {
"trusted": 150.00000000,
"untrusted_pending": 0.00000000,
"immature": 5000.00000000
},
"lastprocessedblock": {
"hash": "000002d74dc11fdc42867e4a0f999d1425ab1140afd1fd4c776a8b9bc8be5d97",
"height": 103
}
}

### 试着发送
bitcoind -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ -fallbackfee=0.00001
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ sendtoaddress tb1p3q6jlepr4ka27hau30hnsqhwh0ktzyueww5ek7g4pyxan786cf8qn8vreq 0.1 -fee_rate=1
#### 再次看余额
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ getbalances
{
"mine": {
"trusted": 449.99999835,
"untrusted_pending": 0.00000000,
"immature": 5000.00000165
},
"lastprocessedblock": {
"hash": "0000018d2747a5222193267fb3817dd26eba51095697660c5091375686097973",
"height": 109
}
}


# tr版本的signetchallenge
DESC="tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5"
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/trsignet/ importdescriptors "[{\"desc\":\"$DESC\",\"timestamp\":\"now\",\"active\":true,\"range\":[0,100],\"internal\":false}]"
[
{
"success": true
}
]
bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ deriveaddresses $DESC "[0,0]"
[
"tb1ptu6leqwf55d2vv6n82xhfqnpf3rgfcfhgzlntk9mf7z4cfs7q9aspk8f5q"
]
jun@mint:~/bitcoin/bitcoin$ bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/defsignet/ getaddressinfo tb1ptu6leqwf55d2vv6n82xhfqnpf3rgfcfhgzlntk9mf7z4cfs7q9aspk8f5q
{
"address": "tb1ptu6leqwf55d2vv6n82xhfqnpf3rgfcfhgzlntk9mf7z4cfs7q9aspk8f5q",
"scriptPubKey": "51205f35fc81c9a51aa633533a8d7482614c4684e13740bf35d8bb4f855c261e017b",
"ismine": true,
"solvable": true,
"desc": "tr([019edbea/86h/1h/0h/0/0]c7e1f5f4cdd101c593d0991239472669088b63ecc0817304f8a84fd35648da88)#fk63qgx9",
"parent_desc": "tr([019edbea/86h/1h/0h]tpubDDeZupEMXixsbn1WHG7CQXKnQVcuZCdj5KgTFbQhuLtX1WkfmrVY4SeAzYubvhNVCJaVTv3zgU8SgNWyhHe7r4gN72JNE8LQVTfcZGsJsLF/0/*)#mmmk034n",
"iswatchonly": false,
"isscript": true,
"iswitness": true,
"witness_version": 1,
"witness_program": "5f35fc81c9a51aa633533a8d7482614c4684e13740bf35d8bb4f855c261e017b",
"ischange": true,
"timestamp": 1791731911,
"hdkeypath": "m/86h/1h/0h/0/0",
"hdseedid": "0000000000000000000000000000000000000000",
"hdmasterfingerprint": "019edbea",
"labels": [
]
}
signetchallenge=51205f35fc81c9a51aa633533a8d7482614c4684e13740bf35d8bb4f855c261e017b
## 使用 contrib/signet/miner 挖矿
./contrib/signet/miner  calibrate --grind-cmd="bitcoin-util grind"
./contrib/signet/miner --cli="bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/trsignet/"   generate   --grind-cmd="bitcoin-util grind"   --descriptor=$DESC   --nbits=1d0da64d
