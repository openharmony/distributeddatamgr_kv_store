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
- `kv_store.gni`：共享 GN 变量，含 `dm_service_enable`、`dms_service_enable` 降级开关。
- `frameworks/libs/distributeddb/distributeddb.gni`：distributeddb 源码聚合与 `kv_store_cloud`/`kv_store_device` feature flag。
- `interfaces/jskits/`：JS 入口和 BUILD.gn（打包层）；NAPI C++ 实现见 `frameworks/jskitsimpl/`。
- `frameworks/jskitsimpl/distributedkvstore/`：最新版 `@ohos.distributedKVStore` NAPI C++ 实现。
- `frameworks/jskitsimpl/distributeddata/`：旧版 `@ohos.distributedData` NAPI C++ 实现(所有问题不需要更改)。
- `frameworks/innerkitsimpl/kvdb/`：KV 客户端框架，CRUD 入口，锁模式与 Status 转换边界。
- `frameworks/innerkitsimpl/distributeddatafwk/`：分布式数据管理框架。
- `frameworks/innerkitsimpl/distributeddatasvc/`：服务端 IPC 代理，IPC 接口码变更需 CODEOWNERS 评审。
- `frameworks/innerkitsimpl/crypt/`：加密/解密工具，`kv_store_crypt` 通过 dlopen 动态加载。
- `frameworks/innerkitsimpl/dm/`/`dms/`：设备管理/分布式调度适配，`dm/dms_service_enable=false` 时 mock 替代。
- `frameworks/libs/distributeddb/`：KV 存储引擎 + 数据同步，底层 SQLite 操作核心。
- `frameworks/common/`：日志、类型转换、任务调度、ConcurrentMap。
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

### 路径触发

- `frameworks/innerkitsimpl/distributeddatasvc/` → IPC 接口码变更必须通知 CODEOWNERS 指定评审人
- `interfaces/innerkits/distributeddata/include/store_errno.h` → 注意 Status enum 与 distributeddb errno 的转换边界
- `frameworks/innerkitsimpl/kvdb/src/security_manager.cpp` → 注意 dlopen 动态加载加密模块的降级行为
- `kv_store.gni` → 注意 `dm_service_enable` / `dms_service_enable` 降级开关

### 术语触发

当任务、issue、log、API、变更文件中出现以下术语时，先理解风险再规划：

| Term | Risk hint | Action |
|---|---|---|
| Status / errno / ConvertStatus | innerkitsimpl 用 Status enum，distributeddb 用 int errno，跨层必须经 ConvertStatus 转换，直接混用会导致语义错误或静默丢错 | 对照 `store_errno.h` 与 `db_errno.h`，确认转换正确 |
| IPC / 接口码 / interface code | IPC 接口码变更必须通知 CODEOWNERS 指定评审人；IPC_SEND 按引用捕获，禁止传入临时或已 move 对象 | 读 `CODEOWNERS`，确认评审流程 |
| dlopen / kv_store_crypt / 加密降级 | 加密模块通过 dlopen 动态加载，缺失时 GetDelegate 返回 nullptr、GetDBPassword 返回空密码、加密数据库打开失败 | 检查 `security_manager.cpp` 降级路径 |
| shared_lock / unique_lock / 锁模式 | CRUD 用 shared_lock（读锁），仅 Close 用 unique_lock（写锁）；禁止给 Put/Delete 加写锁 | 确认锁模式与并发设计一致 |
| Anonymous / StoreUtil::Anonymous / 隐私 | udid/uuid/ip/mac/密钥/数据库路径必须匿名化后输出 | 检查日志是否有 `%{public}` 泄露隐私数据 |
| NAPI / 回调参数 | NAPI 回调参数必须最后一个，禁止放中间 | 检查 NAPI 绑定函数签名 |
| dm_service / dms_service / 降级开关 | `dm_service_enable=false` 时 dm mock 替代，`dms_service_enable=false` 时 dms mock 替代 | 读 `kv_store.gni` 确认开关状态 |
| ConcurrentMap / Compute | Compute 语义：action 返回 false = 删除条目 | 读 `frameworks/common/concurrent_map.h` |

在计划中声明：task category、documents read、constraints found、whether a skill/workflow should be used。

## 硬约束

### 架构/领域不变量

- innerkitsimpl 返回 `Status` enum（`store_errno.h`），distributeddb 返回 `int` errno（`db_errno.h`）。跨层调用必须经 `StoreUtil::ConvertStatus` 转换，不可直接混用。违反会导致语义错误、跨层传播不一致、静默丢错。
- CRUD 操作用 `shared_lock`（读锁），仅 Close 用 `unique_lock`（写锁）。禁止给 Put/Delete 加写锁。
- `kv_store_crypt` 通过 `dlopen` 动态加载（`security_manager.cpp`），缺失时 `GetDelegate()` 返回 nullptr、`GetDBPassword` 返回空密码、加密数据库打开失败。
- IPC 接口码变更必须通知 CODEOWNERS 指定评审人。`IPC_SEND` 按引用捕获，禁止传入临时或已 move 对象（见 `kvdb_service_client.cpp`）。
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
- NAPI 回调参数必须最后一个，禁止放中间。
- 优先选择最小改动，不要顺手重构、加注释、调格式。

### 必须先问人

- IPC 接口码变更。
- 公共 API 签名、错误码、权限行为或生命周期语义变更。
- 新增/删除依赖。
- 加密行为或降级路径变更。
- 锁模式变更。
- 用户只要求调研、检视或对比时，禁止直接实现 — 先确认意图。

### 影响面分析清单

实现前先回答：
- 改动影响哪个 API 面：NAPI JS、ANI/Taihe、CJ FFI、InnerKit，还是仅内部？
- 是否涉及两层错误码（Status vs errno）的转换？
- 是否涉及 IPC 接口码变更（需要 CODEOWNERS 评审）？
- 是否涉及锁模式变更（shared_lock vs unique_lock）？
- 是否涉及隐私数据日志？
- 哪个 GN target 是覆盖该改动的最小有效构建或测试目标？

## 验证习惯

涉及代码修改时，选择单元测试目标，进行完整单元运行测试。

### 最小文本验证

在本仓库根目录运行：

```powershell
git status --short
git diff -- AGENTS.md
```

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
- IPC 接口码变更 → 通知 CODEOWNERS 评审，构建全量部件镜像
- 加密行为变更 → 构建 `kv_store`，检查 dlopen 降级路径
- 锁模式变更 → 构建并运行并发相关单元测试
- 日志/DFX 变更 → 运行隐私检查命令，确认无 `%{public}` 泄露
- 测试变更 → 运行变更的测试和至少一个附近相关测试

### Done 定义

任务完成仅当：
- 请求的行为已实现。
- 相关构建/测试/安全自检/兼容性检查已执行，或已说明无法执行的原因。
- 最终回复包含：变更摘要、变更文件列表、验证命令与结果、兼容性/权限/DFX 影响（如相关）、剩余风险。
- 不包含无关的格式化、重构或附带变更。
