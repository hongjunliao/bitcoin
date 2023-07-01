### 在现代Linux/Ubuntu平台(如Ubuntu22.04)上重新构建Bitcoin-v0.1.5版本

1. [github/bitcoin](https://github.com/bitcoin/bitcoin/tags)对应的Tag为v0.1.5
2. 改为使用[cmake](https://cmake.org/)构建
3. 添加子目录[src/](/src),原有的代码文件不做修改， 所有的更改在这里
4. 添加测试目录[test/](/test)

#### 主要目标	

1. 主要移植核心挖矿模块，包含钱包，交易，脚本验证,本地区块数据保存； 网络及UI部分靠后
	
#### 构建与测试

1. 添加依赖,参见 [deps/README.md](/deps/README.md)
2. 使用cmake构建：`mkdir build && cd build && cmake .. && make`

#### 进度

1. 参见[src/](/src)目录查看移植情况，在新平台能编译通过的文件通常在src/目录会有对应同名文件
2. 参见[test/](/test)目录以查看测试情况