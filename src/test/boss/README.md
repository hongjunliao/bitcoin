//
// private data
// tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5
// 001 invoice:
// wallet_565,lntbs11970n1p5k202qpp53n3ecycdktgjlyz0prj7gunxcvq0ha0rz40j54kr7acmw832jlusdqswaskcmr9w30n2d34cqzzsxq9yn4qqsp56487dpcg226vj8ux5c7u4c3nyuywdrs5enxgglt5n6eqfej957fq9qxpqysgq6292r3j4zwsldx2z7fz9y5zgg4rk5czfvejv9je9v5x0783gntu8e57lsph7czwtjg3czg0g5j3m0rc34p99jzwe33djysdum6xvugsqtq6u96,8ce39c130db2d12f904f08e5e47266c300fbf5e3155f2a56c3f771b71e2a97f9
//
// 003 invoice:
// wallet_565,lntbs90940n1p5k205rpp58wc7epu92h3fca0quclwdrhqks3q5k4ej306tl04m0prc0xj6s5qdqswaskcmr9w30n2d34cqzzsxq9yn4qqsp52uyx8xrtr9t9e76cvmwullus0t7u83prfjn45hm3jfvplpcv770s9qxpqysgqhaegmug6h6220egrdm4msa4f2r2e7rnu7z2d5ldqja9tdsy4vl9h384lpvuswm9kn9vkh5jd49m8vs2vqrp7x9mwqnm0q409c4k3zrcp8azr6g,3bb1ec878555e29c75e0e63ee68ee0b4220a5ab9945fa5fdf5dbc23c3cd2d428

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
测试"构造一个支付到p2pkh的交易,并输出该交易的16进制格式",通过
0.提供:一个在signet上的的有效descriptor(tprv...)
1.需要解析并展开该descriptor,以得到scriptpubkey,从而搜索到该descriptor的可用余额
2.从signet UTXO集查询可用UTXO,构成交易的输入
3.一个p2pkh地址作为交易的输出
4.另一个地址作为找零输出
5.签名交易,并能通过bitcoin-cli发送到bitcoin -signet网络,如果返回了txid则表明交易已被网络接受
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * 参考答案示例:
 * bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0 scantxoutset start '[{"desc":"tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5","range":2000}]'
 * 输出:
	{
	  "success": true,
	  "txouts": 28669,
	  "height": 1462,
	  "bestblock": "00000085fe74ab6ea2db65da3cd5786ec85b68f3f25b27b5b0e6abe132ea8e75",
	  "unspents": [
		...
		{
		  "txid": "d673db156467bffd024d8bb5bf8728cdc0ab7d549f7395e99fc43b07fdd424f4",
		  "vout": 484,
		  "scriptPubKey": "5120ac551df7866f4715dcbde804ca3b1960d204cb2f3c1542b72041d85828e804be",
		  "desc": "tr([019edbea/86h/1h/0h/0/215]a8e948725f5cadd9df8276566fe43b4e0de8f7f8e40d31bd893fd0b6d2637730)#0fywrl2q",
		  "amount": 0.07143937,
		  "coinbase": false,
		  "height": 285,
		  "blockhash": "000000163026df0a6647af21305a68923d8d0fbde182f50dddbc0e834136fc82",
		  "confirmations": 1178
		}
	  ],
	  "total_amount": 1.00425386
	}

 * */

 实现方案
 1.直接使用core/Descriptor,Descriptor::Expand,得到地址,然后用bitcoin-cli RPC scantxoutset
    bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0 scantxoutset start '[{"desc":"addr(tb1ps4gsq9srvu9d9x34g62mnc8wfxh09cdau49s8rr30nt4wyxswn9qvekk07)"}]'
    参见:balance.cpp/openai_main()
    测试方法:
    bitcoind -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0
    ./build/bin/test_bitcoin -t boss/openai_balance



 2.ai提示
 //写一个完整的bitcoin core c/c++测试：内容为 查询desriptor:tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5
 //中的余额;实现方式:你需要解析该descriptor，得到private key,chaincode,然后derive等等全部流程， 最后通过调用biltcoin-cli rpc scantxoutset，传入你生成的addr，最终输出余额；bitcoind已在本地同步， -signet, range为2000以内


# p2pkh收发测试
AI:
bitcoin core测试:我现在已启动bitcoind -signet,我在这个网络上面有desriptor:tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZn
vMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5,我通过编写 boost test查询到了有余额,
我现在想测试 p2pkh,怎么做

要在 Signet 上测试 P2PKH，核心思路是：你需要手动将现有的 tr 描述符转换成 pkh 描述符，然后导入到一个新的钱包中。

1. 构造 P2PKH 描述符
将你原描述符中的 tr(...) 替换为 pkh(...)，其余部分（包括密钥、派生路径和校验和）保持不变。构造出的新描述符如下（注意，校验和需要重新计算，不能沿用原来的）：
pkh(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)

2. 获取正确的校验和
描述符末尾的 #twn4yrj5 是针对 tr() 函数的校验和，对 pkh() 无效，必须重新生成。使用 getdescriptorinfo 命令来获取：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet getdescriptorinfo "pkh(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)"
{
  "descriptor": "pkh(tpubD6NzVbkrYhZ4XwvNTFJHcGM2zTh2RRiEWUbeZXD28YAxdQch7ebUvof7o9vKZP25eyfvqoESCXG33FTHWmacmroEw5NpbW37sHgYZ1gktJY/86h/1h/0h/0/*)#we4cczug",
  "checksum": "36y5zc7m",
  "isrange": true,
  "issolvable": true,
  "hasprivatekeys": true
}
命令会返回一个包含正确 descriptor 字段的 JSON，其中就带有新的校验和。

3. 创建新钱包并导入描述符

你可以创建一个全新的钱包来测试，避免和你已有的数据混淆。
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet createwallet "p2pkh_wallet"
# 导入刚刚生成并带有正确校验和的 pkh 描述符
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" importdescriptors '[{ "desc": "pkh(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#36y5zc7m", "timestamp": "now", "active": true }]'
[
  {
    "success": true,
    "warnings": [
      "Range not given, using default keypool range"
    ]
  }
]
如果 active 设为 true，这个描述符就会成为该钱包生成新地址时默认使用的方案
Range not given, using default keypool range
它的意思是：你的描述符末尾用了 /*（通配符），但你没有显式指定这个通配符要预生
成多少个地址。Bitcoin Core 会使用默认范围，也就是从 0 到 999（共 1000 个地址）。

4. 验证

4.1确认描述符已激活
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" listdescriptors
{
  "wallet_name": "p2pkh_wallet",
  "descriptors": [
    {
      "desc": "pkh([019edbea/86h/1h/0h]tpubDDeZupEMXixsbn1WHG7CQXKnQVcuZCdj5KgTFbQhuLtX1WkfmrVY4SeAzYubvhNVCJaVTv3zgU8SgNWyhHe7r4gN72JNE8LQVTfcZGsJsLF/0/*)#2hg7gwnr",
      "timestamp": 1774347870,
      "active": true,
      "internal": false,
      "range": [
        0,
        999
      ],
      "next": 1,
      "next_index": 1
    },
    {
      "desc": "pkh([7f1fc34b/44h/1h/0h]tpubDDhyBFQSSUXJdq9dZQ3HxY1wEP4ifPmtnquaFvwJeP9SLztpqXKjceEwbYEFuhraHsKp9F1TLo7jqoJygGUDsrwpboGJZkHgjAT8evhmEKh/0/*)#62naz9rq",
      "timestamp": 1791431675,
      "active": false,
      "range": [
        0,
        999
      ],
      "next": 0,
      "next_index": 0
    },
	...
	{
      "desc": "wpkh([7f1fc34b/84h/1h/0h]tpubDCt3zgFydhEYU4QqZ8tfuKxDWY6mRyVQVzVHtusy226rtiFNdQ39K5C3ZrYpN41EAgPyaAaB7MNttihJt2kRRXZD9F56G4XPrxUsFUQtzAP/1/*)#e2kzpah8",
      "timestamp": 1791431675,
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
你应该能看到这条 pkh(...) 描述符，且 active: true。
4.2. 获取一个 P2PKH 地址

导入成功后，就可以获取一个新地址来确认类型是否正确：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" getnewaddress "" legacy
mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY
返回的地址应该是一个以 m 或 n 开头的传统 Base58 地址（Signet/Testnet 的 P2PKH 格式）

4.3. 验证地址确实由这个描述符派生
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" getaddressinfo mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY
{
  "address": "mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY",
  "scriptPubKey": "76a914afa01974a3e0ba24413d24c116c781808541369f88ac",
  "ismine": true,
  "solvable": true,
  "desc": "pkh([019edbea/86h/1h/0h/0/0]03c7e1f5f4cdd101c593d0991239472669088b63ecc0817304f8a84fd35648da88)#xjcvsgfn",
  "parent_desc": "pkh([019edbea/86h/1h/0h]tpubDDeZupEMXixsbn1WHG7CQXKnQVcuZCdj5KgTFbQhuLtX1WkfmrVY4SeAzYubvhNVCJaVTv3zgU8SgNWyhHe7r4gN72JNE8LQVTfcZGsJsLF/0/*)#2hg7gwnr",
  "iswatchonly": false,
  "isscript": false,
  "iswitness": false,
  "pubkey": "03c7e1f5f4cdd101c593d0991239472669088b63ecc0817304f8a84fd35648da88",
  "iscompressed": true,
  "ischange": false,
  "timestamp": 1774347870,
  "hdkeypath": "m/86h/1h/0h/0/0",
  "hdseedid": "0000000000000000000000000000000000000000",
  "hdmasterfingerprint": "019edbea",
  "labels": [
    ""
  ]
}
4.4. 检查余额
因为你之前提到链上有余额，导入后钱包需要重新扫描区块才能识别到。
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" rescanblockchain 0
{
  "start_height": 0,
  "stop_height": 10516
}
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" getbalances
{
  "mine": {
    "trusted": 0.00000000,
    "untrusted_pending": 0.00000000,
    "immature": 0.00000000
  },
  "lastprocessedblock": {
    "hash": "0000012e14242e0b8955d49c07bc31b9b9fc36f4a4f837a05e28803181da7782",
    "height": 10516
  }
}

5.从tr(...)钱包转钱到pkh(...)
创建或使用一个钱包导入 TR 描述符
如果你希望保持 p2pkh_wallet 纯粹用于 P2PKH 测试，建议新建一个钱包
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet createwallet "tr_wallet"
{
  "name": "tr_wallet"
}
然后导入描述符（时间戳设为 0 或钱包创建时间，以便扫描历史区块找到余额）：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="tr_wallet" importdescriptors '[{"desc":"tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5","timestamp":0,"active":true}]'
[
  {
    "success": true,
    "warnings": [
      "Range not given, using default keypool range"
    ]
  }
]
确认余额并转账
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="tr_wallet" getbalances
{
  "mine": {
    "trusted": 1.00425386,
    "untrusted_pending": 0.00000000,
    "immature": 0.00000000
  },
  "lastprocessedblock": {
    "hash": "0000012e14242e0b8955d49c07bc31b9b9fc36f4a4f837a05e28803181da7782",
    "height": 10516
  }
}

确认有余额后，执行转账：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="tr_wallet" -named sendtoaddress address=mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY amount=0.001 fee_rate=1
f51d90e011db731b4d08707522e85114e473e63e7511c41aee289960eae31711
这条命令会返回一个 txid（交易哈希），记下来。

确认交易已广播
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="tr_wallet" gettransaction f51d90e011db731b4d08707522e85114e473e63e7511c41aee289960eae31711
{
  "amount": -0.00100000,
  "fee": -0.00000137,
  "confirmations": 0,
  "trusted": true,
  "txid": "f51d90e011db731b4d08707522e85114e473e63e7511c41aee289960eae31711",
  "wtxid": "fb223c7be99920a3ab7509eb9012cab400308c51359281abdb6c48ba292adbfb",
  "walletconflicts": [
  ],
  "mempoolconflicts": [
  ],
  "time": 1791436063,
  "timereceived": 1791436063,
  "bip125-replaceable": "yes",
  "details": [
    {
      "address": "mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY",
      "category": "send",
      "amount": -0.00100000,
      "vout": 0,
      "fee": -0.00000137,
      "abandoned": false
    }
  ],
  "hex": "02000000000101d74f0a551cf15772fd14d2a2f5a50e4c6e9cd9b894440ceaee66bc75e25c82160000000000fdffffff02a0860100000000001976a914afa01974a3e0ba24413d24c116c781808541369f88ac3b3b0900000000001976a9144265f3ca1f58d38d26b644b19b5055363d7a3fb288ac014082c0219ce354b0991ed63977c0d658648b15e0abc65932147a7bef9808f350de8b69dd92a8740d22d5ba40c3e49f779a8443d8dd97db89ad46448169820cebb200000000",
  "lastprocessedblock": {
    "hash": "0000012e14242e0b8955d49c07bc31b9b9fc36f4a4f837a05e28803181da7782",
    "height": 10516
  }
}
用 decoderawtransaction 看一下脚本，确认资金确实锁定到了 P2PKH 脚本：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet decoderawtransaction 02000000000101d74f0a551cf15772fd14d2a2f5a50e4c6e9cd9b894440ceaee66bc75e25c82160000000000fdffffff02a0860100000000001976a914afa01974a3e0ba24413d24c116c781808541369f88ac3b3b0900000000001976a9144265f3ca1f58d38d26b644b19b5055363d7a3fb288ac014082c0219ce354b0991ed63977c0d658648b15e0abc65932147a7bef9808f350de8b69dd92a8740d22d5ba40c3e49f779a8443d8dd97db89ad46448169820cebb200000000
{
  "txid": "f51d90e011db731b4d08707522e85114e473e63e7511c41aee289960eae31711",
  "hash": "fb223c7be99920a3ab7509eb9012cab400308c51359281abdb6c48ba292adbfb",
  "version": 2,
  "size": 187,
  "vsize": 136,
  "weight": 544,
  "locktime": 0,
  "vin": [
    {
      "txid": "16825ce275bc66eeea0c4494b8d99c6e4c0ea5f5a2d214fd7257f11c550a4fd7",
      "vout": 0,
      "scriptSig": {
        "asm": "",
        "hex": ""
      },
      "txinwitness": [
        "82c0219ce354b0991ed63977c0d658648b15e0abc65932147a7bef9808f350de8b69dd92a8740d22d5ba40c3e49f779a8443d8dd97db89ad46448169820cebb2"
      ],
      "sequence": 4294967293
    }
  ],
  "vout": [
    {
      "value": 0.00100000,
      "n": 0,
      "scriptPubKey": {
        "asm": "OP_DUP OP_HASH160 afa01974a3e0ba24413d24c116c781808541369f OP_EQUALVERIFY OP_CHECKSIG",
        "desc": "addr(mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY)#awwmcxc4",
        "hex": "76a914afa01974a3e0ba24413d24c116c781808541369f88ac",
        "address": "mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY",
        "type": "pubkeyhash"
      }
    },
    {
      "value": 0.00604987,
      "n": 1,
      "scriptPubKey": {
        "asm": "OP_DUP OP_HASH160 4265f3ca1f58d38d26b644b19b5055363d7a3fb2 OP_EQUALVERIFY OP_CHECKSIG",
        "desc": "addr(mma32V5s1BZqBLoV6aqDkJKQmtq2vnPjN7)#rmnvythu",
        "hex": "76a9144265f3ca1f58d38d26b644b19b5055363d7a3fb288ac",
        "address": "mma32V5s1BZqBLoV6aqDkJKQmtq2vnPjN7",
        "type": "pubkeyhash"
      }
    }
  ]
}
你应该在 vout[0].scriptPubKey.hex 看到：
	76a914afa01974a3e0ba24413d24c116c781808541369f88ac
这正是你 mwXaLXb... 的 P2PKH 脚本（76a914...88ac）。✅

在 p2pkh_wallet 中查看余额
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" getbalances
{
  "mine": {
    "trusted": 0.00000000,
    "untrusted_pending": 0.00100000,
    "immature": 0.00000000
  },
  "lastprocessedblock": {
    "hash": "0000012e14242e0b8955d49c07bc31b9b9fc36f4a4f837a05e28803181da7782",
    "height": 10516
  }
}
由于 confirmations: 0，这笔钱会先出现在 untrusted_pending（0 确认的收款默认不算 trusted）
同时 listunspent 默认只显示有确认的，要加 0 才能看到未确认的：
bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet -rpcwallet="p2pkh_wallet" listunspent 0
[
  {
    "txid": "f51d90e011db731b4d08707522e85114e473e63e7511c41aee289960eae31711",
    "vout": 0,
    "address": "mwXaLXb32xZu4PTyVFRQkvARgG1PnPi6qY",
    "label": "",
    "scriptPubKey": "76a914afa01974a3e0ba24413d24c116c781808541369f88ac",
    "amount": 0.00100000,
    "confirmations": 0,
    "ancestorcount": 1,
    "ancestorsize": 136,
    "ancestorfees": 137,
    "spendable": true,
    "solvable": true,
    "desc": "pkh([019edbea/86h/1h/0h/0/0]03c7e1f5f4cdd101c593d0991239472669088b63ecc0817304f8a84fd35648da88)#xjcvsgfn",
    "parent_descs": [
      "pkh(tpubD6NzVbkrYhZ4XwvNTFJHcGM2zTh2RRiEWUbeZXD28YAxdQch7ebUvof7o9vKZP25eyfvqoESCXG33FTHWmacmroEw5NpbW37sHgYZ1gktJY/86h/1h/0h/0/*)#we4cczug"
    ],
    "safe": false
  }
]

