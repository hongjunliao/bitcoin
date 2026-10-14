# 使用 bitcoin-cli 创建 2/3 P2SH 多签地址 的方法

以下是使用 `bitcoin-cli` 创建 **2/3 P2SH 多签地址** 的方法：

1. **准备 3 个公钥**（Public Key）  
   可以用以下命令生成新地址并获取公钥（示例）：
   ```bash
   bitcoin-cli getnewaddress
   bitcoin-cli getaddressinfo <地址>
   ```
   从返回结果中复制 `pubkey` 字段。重复三次，得到 3 个不同的公钥。

2. **创建 2-of-3 P2SH 多签地址**
   ```bash
   bitcoin-cli createmultisig 2 '["公钥1","公钥2","公钥3"]'
   ```

### 示例

假设你有三个公钥：
```
02a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456
03b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef12345678
02c3d4e5f6789012345678901234567890abcdef1234567890abcdef1234567890
```

执行命令：
```bash
bitcoin-cli createmultisig 2 '["02a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456","03b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef12345678","02c3d4e5f6789012345678901234567890abcdef1234567890abcdef1234567890"]'
```

返回结果类似：
```json
{
  "address": "3J98t1WpEZ73CNmYviecrnyiWrnqRhWNLy",
  "redeemScript": "5221...53ae",
  "descriptor": "sh(multi(2,...))#checksum"
}
```

- `address` 就是你要的 **2/3 P2SH 多签地址**（以 `3` 开头）。
- `redeemScript` 是赎回脚本，后续签名时需要用到。

### 注意事项
- `createmultisig` **不需要钱包**，只是生成地址和脚本。
- 如果想把地址加入钱包，可以使用 `addmultisigaddress`（需要钱包解锁）：
  ```bash
  bitcoin-cli addmultisigaddress 2 '["公钥1","公钥2","公钥3"]'
  ```
- 公钥顺序会影响生成的地址，建议按固定顺序排列。
- 这是传统的 **P2SH** 多签（地址以 `3` 开头）。如果需要更现代的 P2WSH 或 P2SH-P2WSH，可以使用描述符（descriptor）方式。


# 示例
bitcoind -signet -datadir=/home/jun/bitcoin/datadir/trsignet/
./src/test/boss/p2sh.sh 'bitcoin-cli -signet -datadir=/home/jun/bitcoin/datadir/trsignet/' './src/test/boss/'

#生成含地址及redeemscript的json文件
ls /src/test/boss/multisig_2of3_multisig_2of3_1791944647.json