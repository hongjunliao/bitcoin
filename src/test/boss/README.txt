给descriptor,查询余额
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
