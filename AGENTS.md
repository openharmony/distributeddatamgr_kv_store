# AGENTS.md

本文件是 AI Agent 处理本仓库任务时的轻量入口。所有架构背景、约束边界、构建与验证知识已整合在本文件中，无需再读取其他文档页。

## 阅读策略

本文件已整合所有必要知识。按任务类型对照对应章节即可：

1. 需要改某个模块，先对照快速代码地图的 Where to look 表定位。
2. 涉及错误码、锁模式、加密、IPC 等边界问题，对照知识路由中的术语触发表。
3. 规划验证，对照验证习惯章节。

## 仓库定位

`distributeddatamgr_kv_store` 是 OpenHarmony 分布式键值数据库组件。

在 OpenHarmony 源码树中的位置：

```text
//foundation/distributeddatamgr/kv_store
```

## 快速代码地图

- `bundle.json`：组件元数据、feature flag、构建 group、inner kit 和测试目标。
- `frameworks/libs/distributeddb/distributeddb.gni`：distributeddb 源码聚合与 `kv_store_cloud`/`kv_store_device` feature flag。
- `interfaces/jskits/`：JS 入口和 BUILD.gn（打包层）；NAPI C++ 实现见 `frameworks/jskitsimpl/`。
- `frameworks/jskitsimpl/distributedkvstore/`：最新版 `@ohos.distributedKVStore` NAPI C++ 实现。
- `frameworks/jskitsimpl/distributeddata/`：旧版 `@ohos.distributedData` NAPI C++ 实现(所有问题不需要更改)。
- `frameworks/innerkitsimpl/kvdb/`：KV 客户端框架，CRUD 入口，锁模式与 Status 转换边界。**高频修改**：`single_store_impl.cpp`、`security_manager.cpp`。
- `frameworks/innerkitsimpl/distributeddatafwk/`：分布式数据管理框架。
- `frameworks/innerkitsimpl/distributeddatasvc/`：服务端 IPC 代理，`DataMgrServiceProxy` 通过 `SendRequest()` 与 kv_store 服务进程通信。
- `frameworks/innerkitsimpl/crypt/`：加密/解密工具，`kv_store_crypt` 通过 dlopen 动态加载（`KVDBCryptoImpl`，AES-256-GCM）。
- `frameworks/innerkitsimpl/dm/`/`dms/`：设备管理/分布式调度适配，`dm/dms_service_enable=false` 时 mock 替代。
- `frameworks/libs/distributeddb/`：KV 存储引擎 + 数据同步，底层 SQLite 操作核心。**高频修改**：`kv_store_nb_delegate_impl.cpp`、`sqlite_local_kvdb_connection.cpp`。
- `frameworks/common/`：日志、类型转换、任务调度、ConcurrentMap。**稳定**：改动较少。
- `frameworks/cj/`：CJ FFI 实现。
- `frameworks/ets/taihe/kv_store/`：ANI/Taihe 静态绑定。
- `databaseutils/`：ACL 权限工具。
- `interfaces/innerkits/distributeddata/`：InnerKit 公开头文件（`store_errno.h`、`single_kvstore.h` 等）。
- `interfaces/innerkits/distributeddatamgr/`：分布式数据管理 InnerKit（`distributed_data_mgr.h`）。

Where to look:

| 任务类型 | 先看哪里 |
|---|---|
| 公共 API / SDK 行为变更 | `frameworks/jskitsimpl/distributedkvstore/`、`interfaces/innerkits/distributeddata/include/store_errno.h` |
| 存储引擎行为变更 | `frameworks/libs/distributeddb/`、`frameworks/libs/distributeddb/common/include/db_errno.h` |
| IPC 接口码变更 | `frameworks/innerkitsimpl/distributeddatasvc/`、`CODEOWNERS` |
| 加密行为变更 | `frameworks/innerkitsimpl/crypt/`、`frameworks/innerkitsimpl/kvdb/src/security_manager.cpp` |
| 日志 / DFX 变更 | `frameworks/common/log_print.h` |
| 数据写入路径追踪 | `frameworks/innerkitsimpl/kvdb/src/single_store_impl.cpp` → `frameworks/libs/distributeddb/interfaces/src/kv_store_nb_delegate_impl.cpp` |
| 并发容器逻辑 | `frameworks/common/concurrent_map.h` |
| 构建配置变更 | `kv_store.gni`、`frameworks/libs/distributeddb/distributeddb.gni` |
| 新增/删除依赖 | `bundle.json` |
| 测试变更 | `frameworks/*/test/`、`test/unittest/`、`test/fuzztest/`；先看附近测试模式 |

## 知识路由

按任务类型、修改路径、或领域术语决定下一步动作：

### 任务触发

| 任务或问题 | 动作 |
|---|---|
| 影响范围不清楚 | 先对照快速代码地图的 Where to look 表定位 |
| 数据写入/读取行为异常 | 跟数据写入路径：`js_single_kv_store.cpp` → `single_store_impl.cpp` → `kv_store_nb_delegate_impl.cpp` → `sqlite_local_kvdb_connection.cpp` |
| 数据同步失败或回调不触发 | 看同步侧路：`kv_store_nb_delegate_impl.cpp` 中 Pragma(PRAGMA_SYNC_DEVICES) → `syncer_proxy.h` → `generic_syncer.cpp` → `sync_engine.cpp` |
| 云同步行为异常 | 看云同步侧路：`icloud_syncer.h` → `cloud_syncer.h` → `cloud_sync_state_machine.h` → `CloudMergeStrategy` |
| IPC 通信失败 / 服务端不通 | 看 IPC 侧路：`datamgr_service_proxy.cpp` → `distributeddata_ipc_interface_code.h`；确认 SAID=1301 |
| 加密/解密异常 / 密钥生成失败 | 看加密侧路：`security_manager.cpp` → dlopen(`libkv_store_crypt.z.so`) → `kv_store_crypt.cpp`；确认 asm 符号名匹配 |
| 设备管理异常 / UUID 转换错误 | 看 `dev_manager.h` → dlopen(dm_adapter) → `device_adapter.cpp`；检查 `dm_service_enable` feature flag |
| 数据库创建/打开失败 | 看 `single_store_impl.cpp` 的 `OpenKvStore` 流程 + `store_factory.cpp`；检查 bundleId 和 ACL 权限 |
| 备份/恢复行为异常 | 看 `backup_manager.h` + `single_store_impl.cpp` 中 Backup/Restore 路径；检查备份目录 ACL 权限 |


### 术语触发

当任务、issue、log、API、变更文件中出现以下术语时，先理解风险再规划：

| Term | Risk hint                                                                                     | Action                                   |
|---|-----------------------------------------------------------------------------------------------|------------------------------------------|
| Status / errno / ConvertStatus | innerkitsimpl 用 Status enum，distributeddb 用 int errno，跨层必须经 ConvertStatus 转换，直接混用会导致语义错误或静默丢错 | 对照 `store_errno.h` 与 `db_errno.h`，确认转换正确 |
| IPC / 接口码 / interface code | IPC 接口码变更必须接收到评审通过指令才能继续处理；IPC_SEND 按引用捕获，禁止传入临时或已 move 对象                                    | 读 `CODEOWNERS`，确认是哪些改动需要确认是否通过           |
| shared_lock / unique_lock / 锁模式 | CRUD 用 shared_lock（读锁），仅 Close 用 unique_lock（写锁）；禁止给 Put/Delete 加写锁                           | 确认锁模式与并发设计一致                             |
| Anonymous / StoreUtil::Anonymous / 隐私 | udid/uuid/ip/mac/密钥/数据库路径必须匿名化后输出                                                             | 检查日志是否有 `%{public}` 泄露隐私数据               |
| NAPI / 回调参数 | NAPI 回调参数必须最后一个，禁止放中间                                                                         | 检查 NAPI 绑定函数签名                           |
| ConcurrentMap / Compute | Compute 语义：action 返回 false = 删除条目                                                             | 读 `frameworks/common/concurrent_map.h`   |
| Delegate / nb_delegate / KvStoreNbDelegateImpl | `KvStoreNbDelegateImpl` 是 innerkitsimpl 到 distributeddb 的桥接层，持有 `IKvDBConnection* conn_`；`KvStoreNbDelegate` 是 `SingleStoreImpl` 中 `DBStore` 的类型别名。混淆 delegate 与 connection 会导致修改错误的抽象层 | 确认修改的是接口层 delegate 还是内部 connection |
| Feature flag (kv_store_cloud / kv_store_device) | `distributeddb.gni` 中 `kv_store_cloud=true` / `kv_store_device=true`，映射到 BUILD.gn 中 `USE_DISTRIBUTEDDB_CLOUD` / `USE_DISTRIBUTEDDB_DEVICE`。改错 flag 导致同步代码静默不编译 | 修改同步相关代码时必须同时检查 gni flag 和 BUILD.gn define 映射 |
| kv_store_crypt / dlopen / asm 符号 | 加密模块通过 `dlopen("libkv_store_crypt.z.so")` 动态加载，符号名 `CreateKvdbCryptoDelegate` / `GenerateKvdbRandomNum` 必须匹配。符号不匹配时加密静默失败，`SecurityManager` 无限重试 | 修改加密代码时确认 asm 符号名在 `kv_store_crypt.cpp` 与 `security_manager.cpp` 之间一致 |
| dm_service_enable / dms_service_enable / mock | feature flag 控制真实 dm/dms 适配器还是 mock。mock 在 `kvstoremock/`。flag 状态错误意味着设备管理静默失败 | 修改设备管理代码前检查 feature flag；确保 mock 和真实适配器接口保持对齐 |
| ICloudSyncer / CloudSyncer / ISyncer | 云同步走 `ICloudSyncer` → `CloudSyncer` → `CloudSyncStateMachine`，设备同步走 `ISyncer` → `GenericSyncer` → `SyncStateMachine`。两条路径参数和回调不同，混用会导致错误 | 确认修改的是云同步还是设备同步路径；`CloudTaskInfo` 与 `SyncParam` 字段不同 |
| Checkpoint / EXEC_CHECKPOINT / RetryWithCheckPoint | `RetryWithCheckPoint()` 在 WAL 溢出（`LOG_OVER_LIMITS`）时执行 checkpoint 后重试。跳过 checkpoint 会导致数据操作永久失败 | NEVER 移除 RetryWithCheckPoint；理解 LOG_OVER_LIMITS 是可通过 checkpoint 恢复的 |
| Pragma / PragmaCmd | distributeddb 内部控制接口。`Sync()` 实际通过 `Pragma(PRAGMA_SYNC_DEVICES)` 执行。`g_pragmaMap[]` 在 `kv_store_nb_delegate_impl.cpp` 映射外部→内部 PragmaCmd。混淆两者导致命令路由错误 | 新增 Pragma 命令时必须同时更新 `store_types.h` 的 PragmaCmd 和 `kvdb_pragma.h` 的内部 PragmaCmd，并在 `g_pragmaMap[]` 中添加映射 |
| Rekey / CipherPassword | 数据库重加密会修改 cipher password。重密钥期间禁用手动同步（`DisableManualSync()`/`EnableManualSync()`）。错误的重密钥流程导致数据库损坏 | 重密钥期间禁止发起同步；必须验证 RekeyRecover 路径 |
| SyncOperation / OP_BUSY_FAILURE / E_BUSY | `SyncOperation` 跟踪每设备同步状态。`OP_BUSY_FAILURE` 映射自 `-E_BUSY`。SQLite `BUSY_TIMEOUT_MS=3000`（3秒）。移除 busy 处理会导致操作失败被静默丢弃 | NEVER 移除 E_BUSY 处理；BUSY_TIMEOUT 可通过 Pragma 配置；db close 期间同步引擎返回 -E_BUSY |
| ObserverBridge / StoreObserver | `ObserverBridge` 将 innerkitsimpl 的 `KvStoreObserver` 桥接到 distributeddb 的 `StoreObserver`，用 `Convertor` 转换变更通知。桥接方向错误会丢失通知 | 修改通知格式时验证 ObserverBridge 中的 Convertor 转换方向 |
| Snapshot / KvStoreSnapshotDelegate | 只读快照。`KvStoreSnapshotDelegateImpl` 包装 `IKvDBSnapshot`。快照必须用后释放。快照数据与实时数据分离 | NEVER 通过快照修改数据；必须通过 `ReleaseKvStoreSnapshot()` 释放快照 |
| BackupManager / autoBackup | `BackupManager` 单例处理自动数据库备份。`autoBackup_` flag 控制是否初始化。`Backup()/Restore()/DeleteBackup()` 为公共 API。备份目录 ACL 权限错误会导致备份失败 | 验证备份目录 ACL 权限；确认 autoBackup_ flag 状态 |
| Convertor / DeviceConvertor | innerkitsimpl Key/Value 与 distributeddb DBKey/DBValue 之间的转换器。设备存储和本地存储用不同实现。错误的 Convertor 导致 key 格式不匹配 | 修改 key 格式或前缀逻辑时验证 Convertor 转换方向 |

## 硬约束

### 架构/领域不变量

- `.cpp` 定义 `LOG_TAG`；udid/uuid/ip/mac/密钥/数据库路径 MUST 用 `StoreUtil::Anonymous()` 匿名化后再输出。NEVER 明文打印隐私数据。
- 稳定性排查、日志打印排查、安全编码自检的完整清单见 `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md`。
- 命名：PascalCase 方法/类、`camelCase_` 尾下划线成员变量、`UPPER_SNAKE_CASE` 常量；文件 `snake_case`，mock 加 `_mock`。
- 共享库 `-fvisibility=hidden` 仅导出必要符号。

### 禁止事项

- NEVER 在锁内发送 IPC。
- NEVER 明文打印 udid/uuid/ip/mac/密钥/数据库路径 — 必须用 `StoreUtil::Anonymous()` 匿名化。
- NEVER 为通过测试删除日志、事件、错误码或诊断信息。
- NEVER 改公共 API 签名/错误码/权限行为/生命周期语义，除非任务明确要求。
- NEVER 直接混用 Status enum 与 int errno，必须经 ConvertStatus 转换。
- NEVER 给 Put/Delete 加写锁。
- NEVER 重构旧版 `@ohos.distributedData`（`frameworks/jskitsimpl/distributeddata/`），除非任务明确要求。
- NEVER 在 NAPI 回调参数中间位置放回调，回调必须最后一个参数。

### 必须先问人

- IPC 接口码变更。
- 公共 API 签名、错误码、权限行为或生命周期语义变更。
- 新增/删除依赖。
- 加密行为变更。
- 锁模式变更。
- 用户只要求调研、检视或对比时，禁止直接实现 — 先确认意图。

### 影响面分析清单

实现前先回答：
- 改动影响哪个 API 面：NAPI JS、ANI/Taihe、CJ FFI、InnerKit，还是仅内部？
- 是否涉及两层错误码（Status vs errno）的转换？
- 是否涉及 IPC 接口码变更（需要 CODEOWNERS 评审）？
- 是否涉及锁模式变更（shared_lock vs unique_lock）？
- 是否涉及隐私数据日志？

## 验证习惯

涉及代码修改时，选择单元测试目标，进行完整单元运行测试。

### 最小文本验证

在本仓库根目录运行：

代码改动还应检查是否误用 public 日志打印隐私数据：

```powershell
rg -n "ZLOG[IWE].*%{public}.*(udid|uuid|ip|mac|path|passwrod|pwd)" frameworks interfaces
```

### 测试与构建目标

| 目标 | 用途 | 命令 |
|---|---|---|
| `kv_store` | 构建全量部件镜像 | `./build.sh --product-name <product> --build-target kv_store` |
| `kv_store_test` | 构建部件镜像和测试用例 | `./build.sh --product-name <product> --build-target kv_store_test` |

环境变量：`<product>` — 传给 `./build.sh --product-name` 的产品占位符（示例 `rk3568`）。

如果工作区使用 `hb`，等价模式是：

| 目标 | 用途 | 命令 |
|---|---|---|
| `kv_store` | 独立编译镜像 | `hb build kv_store` |
| `kv_store_test` | 独立编译镜像和测试用例 | `hb build kv_store -t` |

### 任务级验证

- 公共 API 变更 → 构建受影响模块，运行附近单元测试，检查 API 兼容性
- NAPI C++ 变更 → 构建 `kv_store`，运行 `frameworks/jskitsimpl/` 附近测试
- 锁模式变更 → 构建并运行并发相关单元测试
- 日志/DFX 变更 → 运行隐私检查命令，确认无 `%{public}` 泄露
- 测试变更 → 运行变更的测试和至少一个附近相关测试

### Done 定义

任务完成仅当：
- 请求的行为已实现。
- 相关构建/测试/安全自检/兼容性检查已执行，或已说明无法执行的原因。
- 最终回复包含：变更摘要、变更文件列表、验证命令与结果、兼容性/权限/DFX 影响（如相关）、剩余风险。
- 不包含无关的格式化、重构或附带变更。
