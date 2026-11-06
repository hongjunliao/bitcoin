# bitcoin.conf 配置档案解析

这是一份 Bitcoin Core 的**完整配置模板**，由 `contrib/devtools/gen-bitcoin-conf.sh` 自动生成。所有选项默认被注释（`#`），需要时取消注释并保存到数据目录（`-datadir` 指定处，默认如 `~/.bitcoin/bitcoin.conf`）。

下面按官方分组逐类解析核心选项。

---

## 一、Options（通用选项）

| 选项 | 作用 | 默认 |
|---|---|---|
| `alertnotify=<cmd>` | 触发警报时执行命令，`%s` 替换为消息 | — |
| `assumevalid=<hex>` | 指定区块及其祖先视为有效，跳过脚本验证（0 = 全部验证） | 内置哈希 |
| `blockfilterindex=<type>` | 维护 BIP157 紧凑过滤器索引（`basic`/`1`） | 0 |
| `blocknotify=<cmd>` | 最佳区块变化时执行命令（IBD/重建索引期间不触发） | — |
| `blocksonly=1` | 拒绝来自对等节点的交易（仅区块模式），RPC 交易不受影响 | 0 |
| `blocksxor=1` | 对 `blocksdir` 的 `*.dat` 文件应用 XOR 密钥加密 | 1 |
| `coinstatsindex=1` | 维护 `gettxoutsetinfo` 用的币统计索引 | 0 |
| `daemon=1` | 后台守护进程运行 | 0 |
| `daemonwait=1` | 等初始化完成再退出（隐含 `-daemon`） | 0 |
| `datadir=<dir>` | 数据目录 | — |
| `dbcache=<n>` | 数据库缓存 MiB（最小 4，默认 1024），与 mempool 未用内存共享 | 1024 |
| `maxmempool=<n>` | 交易内存池上限 MiB | 300 |
| `mempoolexpiry=<n>` | 交易在内存池最长保留小时数 | 336 |
| `par=<n>` | 脚本验证线程数（0=自动，最大 15，负值=保留核心） | 0 |
| `persistmempool=1` | 关机保存、重启加载内存池 | 1 |
| `prevoutfetchthreads=<n>` | 预取 prevout 线程数（0 禁用，最大 16，负值拒绝） | 8 |
| `prune=<n>` | 裁剪旧区块：0=禁用，1=允许手动 RPC 裁剪，≥550=自动裁剪到目标 MiB。**与 `-txindex` 不兼容**，逆转需重新下载全链 | 0 |
| `reindex=1` | 清空链状态与区块索引，从 `blk*.dat` 重建（含 assumeutxo 快照链状态） | — |
| `reindex-chainstate=1` | 仅清空链状态重建 | — |
| `txindex=1` | 完整交易索引，供 `getrawtransaction` 使用 | 0 |
| `txospenderindex=1` | 交易输出花费者索引，供 `gettxspendingprevout` 使用 | 0 |
| `shutdownnotify` / `startupnotify` | 关机/启动时执行命令 | — |

---

## 二、Connection（连接选项）

- **`addnode=<ip>`**：添加常连节点，可多次指定，独立于 `maxconnections`，上限 8 个。
- **`bind=<addr>[:port][=onion]`**：绑定监听地址（默认 `0.0.0.0`；Tor 入站标记 `=onion`）。
- **`connect=<ip>`**：只连指定节点（`-noconnect` 禁用自动连接）。
- **`listen=1`**：接受外部连接（无 `-proxy`/`-connect`/`maxconnections=0` 时默认 1）。
- **`maxconnections=<n>`**：自动连接上限（默认 200，最大 2000；11 个槽位保留给出站）。
- **`maxuploadtarget=<n>`**：24h 出站流量上限（0=不限，后缀 k/m/g/t，大写 1024 进制）。
- **`onlynet=<net>`**：出站仅限 ipv4/ipv6/onion/i2p/cjdns，可多次指定。
- **`proxy=<ip>[:port][=network]`**：SOCKS5 代理（支持 `unix:` 路径，`=network` 限定网络）。
- **`onion=<ip:port|path>`**：Tor onion 服务专用代理（默认同 `-proxy`）。
- **`torcontrol=<ip>:<port>`** / **`torpassword=<pass>`**：Tor 控制端口（默认 `127.0.0.1:9051`）。
- **`i2psam=<ip:port>`** / **`i2pacceptincoming=1`**：I2P SAM 代理与入站。
- **`cjdnsreachable=1`**：启用 CJDNS（`fc00::/8`）。
- **`dnsseed` / `fixedseeds` / `forcednsseed`**：DNS 种子相关（默认 1/1/0）。
- **`peerblockfilters=1`** / **`peerbloomfilters=1`**：向对等节点提供 BIP157 过滤器 / BIP37 布隆过滤器。
- **`whitebind` / `whitelist`**：为绑定地址/IP 授予权限（`bloomfilter`、`noban`、`forcerelay`、`relay`、`mempool`、`download`、`addr`，默认 `download,noban,mempool,relay`）。
- **`v2transport=1`**：支持 BIP324 v2 传输。
- **`natpmp=1`**：PCP/NAT-PMP 端口映射。

---

## 三、Wallet（钱包选项）

- **`addresstype`**：地址类型 `legacy`/`p2sh-segwit`/`bech32`/`bech32m`（默认 `bech32`）。
- **`changetype`**：找零类型（默认随 `addresstype`）。
- **`avoidpartialspends=1`**：按地址聚合输出，提升隐私但可能增加手续费。
- **`consolidatefeerate=<amt>`**：允许为减少 UTXO 池而多用输入的费率上限（默认 0.0001 BTC/kvB）。
- **`discardfee` / `fallbackfee` / `mintxfee` / `maxfeerate`**：费率相关（默认 0.0001 / 0.00 / 0.00001 / 0.10 BTC/kvB）。
- **`disablewallet=1`**：禁用钱包与钱包 RPC。
- **`keypool=<n>`**：密钥池大小（默认 1000；过小会增大旧备份恢复丢币风险）。
- **`signer=<cmd>`**：外部签名工具（见 `doc/external-signer.md`）。
- **`spendzeroconfchange=1`**：花费未确认找零。
- **`txconfirmtarget=<n>`**：目标平均确认区块数（默认 6）。
- **`wallet=<path>`**：启动时加载钱包，可多次指定（只加载不创建）。
- **`walletdir=<dir>`** / **`walletbroadcast=1`** / **`walletnotify=<cmd>`**（`%s`=TxID，`%w`=钱包名，`%b`=区块哈希，`%h`=高度）。
- **`walletrbf=1`**（已弃用）：全 RBF 选入。

---

## 四、Debugging/Testing（调试/测试）

- **`debug=<category>`**：输出调试日志；`<category>` 可为 `addrman, bench, blockstorage, cmpctblock, coindb, estimatefee, http, i2p, ipc, kernel, leveldb, lock, mempool, mempoolrej, mining, net, privatebroadcast, proxy, prune, qt, rand, reindex, rpc, scan, selectcoins, tor, txpackages, txreconciliation, validation, walletdb, zmq`；`1`/`all` = 全部，`0`/`none` = 忽略其他类别。可多次指定。
- **`debugexclude=<category>`**：排除某类别（优先于 `-debug`）。
- **`logips` / `loglevelalways` / `logratelimit` / `logsourcelocations` / `logthreadnames` / `logtimestamps`**：日志格式与限速（默认 0/0/1/0/0/1）。
- **`printtoconsole=1`**：输出到控制台（无 `-daemon` 时默认 1；`-nodebuglogfile` 可禁用文件日志）。
- **`shrinkdebugfile=1`**：启动时压缩 debug.log（无 `-debug` 时默认 1）。
- **`maxtxfee=<amt>`**：单笔钱包交易总手续费上限（默认 0.10 BTC）。
- **`uacomment=<cmt>`**：追加到 user agent 的注释。

---

## 五、Chain selection（链选择）

- **`chain=<chain>`**：`main`（默认）/`test`/`testnet4`/`signet`/`regtest`。
- **`signet=1`**：等价 `-chain=signet`；配合 `signetchallenge`、`signetseednode`。
- **`testnet=1`**：等价 `-chain=test`，**testnet3 已弃用**，建议迁移 testnet4。
- **`testnet4=1`**：等价 `-chain=testnet4`。

---

## 六、Node relay（节点中继）

- **`bytespersigop=1`**：每 sigop 的等效字节数（默认 20）。
- **`datacarrier=1`** / **`datacarriersize=1`**：是否中继/挖矿数据载体交易及聚合 scriptPubKey 大小上限（默认 100000）。
- **`minrelaytxfee=<amt>`**：低于此费率视为零费（默认 0.000001 BTC/kvB）。
- **`permitbaremultisig=1`**：中继非 P2SH 多签输出。
- **`privatebroadcast=1`**（实验性）：经 Tor/I2P 短连接广播 `sendrawtransaction` 交易，不先进内存池；钱包交易不受影响。
- **`whitelistforcerelay` / `whitelistrelay`**：对白名单节点加 `forcerelay`/`relay` 权限（默认 0/1）。

---

## 七、Block creation（区块创建）

- **`blockmintxfee=<amt>`**：挖矿交易最低费率（默认 0.00000001 BTC/kvB）。
- **`blockreservedweight=<n>`**：区块头 + coinbase 预留空间（默认 8000，仅影响挖矿 RPC 客户端）。

---

## 八、RPC server（RPC 服务器）

- **`server=1`**：接受命令行与 JSON-RPC 命令（RPC 必须项）。
- **`rest=1`**：接受公共 REST 请求。
- **`rpcallowip=<ip>`**：允许的 RPC 来源（单 IP / netmask / CIDR / `0.0.0.0/0` / `::/0`）。
- **`rpcauth=<userpw>`**：`<USERNAME>:<SALT>$<HASH>` 形式的 HMAC-SHA-256 认证（脚本 `share/rpcauth`），可多次指定。
- **`rpcbind=<addr>[:port]`**：RPC 监听地址（默认 `127.0.0.1` 与 `::1`；**勿暴露公网**，需配合 `-rpcallowip`）。
- **`rpcport=<port>`**：默认 8332 / 18332 / 48332 / 38332 / 18443。
- **`rpcuser` / `rpcpassword`**：传统用户名密码认证。
- **`rpccookiefile` / `rpccookieperms=<owner|group|all>`**：认证 cookie 文件位置与权限。
- **`rpcmaxconnections=<n>`**：HTTP 客户端上限（默认 16）。
- **`rpcthreads=<n>`**：RPC 服务线程数（默认 16）。
- **`rpcwhitelist=<USERNAME>:<rpc1>,<rpc2>`** / **`rpcwhitelistdefault=1`**：按用户过滤 RPC 调用。

---

## 九、[Sections]（网络分区）

大部分选项对所有网络生效；要限定网络，放在对应节下：

```ini
[main]      # 主网
[test]      # testnet3
[testnet4]  # testnet4
[signet]    # signet
[regtest]   # regtest
```

**注意**：若未写在网络节下，`addnode`、`connect`、`port`、`bind`、`rpcport`、`rpcbind`、`wallet` **仅作用于主网**。

---

## 十、典型使用建议

1. **普通全节点**：`daemon=1`、`dbcache=2048`、`maxmempool=500`（视内存）。
2. **隐私/轻量**：`blocksonly=1` + `peerbloomfilters=0`。
3. **索引需求**：`txindex=1`（不可与 `prune` 同用）、`blockfilterindex=basic`。
4. **RPC 远程访问**：`server=1` + `rpcbind=0.0.0.0` + `rpcallowip=<受信网段>` + `rpcauth`（**切勿用 `rpcpassword` 明文暴露公网**）。
5. **磁盘紧张**：`prune=550`（注意钱包/索引需保持同步，落后可能需 reindex）。
6. **测试网**：优先 `testnet4=1`（testnet3 已弃用）。
7. **手续费策略**：`fallbackfee=0.0002`（若费率估算数据不足）、`txconfirmtarget=6`。

如需针对具体场景（如交易所节点、闪电网络后端、regtest 开发）给出最小化配置，可以告诉我用途，我帮你裁剪出可直接使用的 `bitcoin.conf`。