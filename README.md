# 该项目已并入[bitcoin/src/test/btcc](https://github.com/hongjunliao/bitcoin)

### bitcoin core源码学习

#### 主要目标	

* bitcoin网络通信，挖矿，钱包，交易，脚本验证,本地区块数据保存
* 自行编码，结合bitcoin core的代码,在简化代码的同时实现核心功能
* 参考书籍: <Programming Bitcoin Learn How to Program Bitcoin from Scratch (Jimmy Song) > 
	<Mastering the Lightning Network (Andreas M. Antonopoulos, Rene Pickhardt etc.) >
	
#### 构建与测试

使用cmake构建：

```code
mkdir build && cd build && cmake .. && make
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 #debug构建
```

#### run
./cmake-build-debug/btcc --run_test=block/genesis --  -datadir=/home/jun/bitcoin/datadir/boss

# 进度
	1.p2p:解析并得到mainet的version等消息
	2.block:通过libbitcoinkernel提取genesis block的coinbase,成功显示"The Times 03/Jan/2009 Chancellor on brink of second bailout for banks"
	3.网络：用3个节点,模拟gossip协议广播到全网