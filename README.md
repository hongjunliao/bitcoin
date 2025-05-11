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
