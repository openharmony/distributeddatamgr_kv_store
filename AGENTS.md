# distributeddatamgr_kv_store — Agent 指令

本文件是 Agent 行为护栏，不是项目 README。Agent 能自己探索出来的少写；Agent 猜不准、猜错代价高、团队必须统一执行的内容要写。

## Meta 规则

- 用户只要求调研、检视或对比时，禁止直接实现；必须等用户明确要求改代码 `[S1]`
- 优先选择最小改动，不要顺手重构、加注释、调格式 `[E4]`
- 引用构建或测试命令前，必须从 `BUILD.gn` 核对真实目标名 `[E5]`
- AGENTS.md 只记录 Agent 无法从代码推导出的约束；长解释应沉淀到 `docs/`
- 修改时机：代码修改完成后再更新 AGENTS.md；同一 PR 同步更新条件：目录/模块增删重命名、构建/测试命令变更、核心依赖增删、踩坑≥2次、探索求证≥3次、跨模块依赖增删；纯实现细节改动不更新
- 禁止在本文档中写入密钥、Token、真实服务端点或生产专用变量 `[C4]`

## 1. Code map

本 AGENTS.md 适用于仓库根目录。当前无子目录 AGENTS.md。

本仓库实现 OpenHarmony 分布式键值数据库组件（`@ohos/kv_store` v3.1.0，子系统 `distributeddatamgr`），为设备应用提供键值对数据管理能力。主要语言：C++17、ArkTS/ETS、JavaScript 绑定代码、GN。对外接口层：InnerKit、NAPI、CJ FFI、Taihe/ANI。

最重要的架构边界：innerkitsimpl（客户端框架，返回 `Status` enum）与 distributeddb（存储引擎，返回 `int` errno）之间的错误码转换边界。跨层调用必须经 `StoreUtil::ConvertStatus` 转换，不可直接混用。

Key areas:

| 目录 | 职责与风险 | 依赖 | 被谁依赖 |
|---|---|---|---|
| `interfaces/jskits/distributedkvstore/` | NAPI JS API 层，兼容性敏感 | kvdb | 应用层 |
| `interfaces/jskits/distributeddata/` | 旧版 NAPI JS API 层，兼容性敏感 | distributeddatafwk | 应用层 |
| `frameworks/innerkitsimpl/kvdb/` | KV 客户端框架，CRUD 入口，锁模式和 Status 转换边界 | common, distributeddb | jskits/distributedkvstore |
| `frameworks/innerkitsimpl/distributeddatafwk/` | 分布式数据管理框架 | common, kvdb | jskits/distributeddata |
| `frameworks/innerkitsimpl/distributeddatasvc/` | 服务端 IPC 代理，IPC 接口码变更需 CODEOWNERS 评审 | common, ipc | datamgr_service |
| `frameworks/innerkitsimpl/crypt/` | 加密/解密工具，`kv_store_crypt` 通过 dlopen 动态加载 | common, openssl | kvdb |
| `frameworks/innerkitsimpl/dm/` | 设备管理适配，dm_service_enable=false 时 mock 替代 | common, device_manager | kvdb, distributeddatafwk |
| `frameworks/innerkitsimpl/dms/` | 分布式调度适配，dms_service_enable=false 时 mock 替代 | common, dmsfwk | distributeddatafwk |
| `frameworks/libs/distributeddb/` | KV 存储引擎 + 数据同步，底层 SQLite 操作核心 | common, sqlite, openssl | innerkitsimpl/kvdb |
| `frameworks/common/` | 日志、类型转换、任务调度、ConcurrentMap | hilog | innerkitsimpl/*, distributeddb |
| `databaseutils/` | ACL 权限工具 | c_utils | innerkitsimpl/kvdb |
| `frameworks/ets/taihe/kv_store/` + `interfaces/cj/` | Taihe/ANI + CJ FFI 接口层 | — | — |

当前未记录有意设计的循环依赖。若发现循环依赖，先创建 issue 并在此记录。

Where to look:

| 任务类型 | 先看哪里 |
|---|---|
| 公共 API / SDK 行为变更 | `interfaces/jskits/`、`store_errno.h` |
| 存储引擎行为变更 | `frameworks/libs/distributeddb/`、`db_errno.h` |
| IPC 接口码变更 | `frameworks/innerkitsimpl/distributeddatasvc/`、`CODEOWNERS` |
| 加密行为变更 | `frameworks/innerkitsimpl/crypt/`、`security_manager.cpp` |
| 日志 / DFX 变更 | `frameworks/common/log_print.h` |
| 数据写入路径追踪 | `single_store_impl.cpp` → `kv_store_nb_delegate_impl.cpp` |
| 并发容器逻辑 | `concurrent_map.h` |
| 构建配置变更 | `kv_store.gni`、`distributeddb.gni` |
| 新增/删除依赖 | `bundle.json` |
| 测试变更 | `frameworks/*/test/`、`test/unittest/`、`test/fuzztest/`；先看附近测试模式 |

## 2. Knowledge routing

不要把以下文档当作可选背景。任务命中触发条件时，必须在规划前读取对应权威文档。

### Task-based routing

- 公共 API / SDK 行为变更 → 读 `store_errno.h`（错误码边界）+ `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md`（兼容性自检）
- IPC 接口码变更 → 读 `CODEOWNERS`（必须通知指定评审人）
- 错误处理变更 → 读 `store_errno.h` + `db_errno.h`（两层 errno）
- 日志 / DFX 变更 → 读 `log_print.h` + `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md`（日志安全自检）
- 并发容器逻辑变更 → 读 `concurrent_map.h`（Compute：action 返回 false = 删除条目）
- 构建配置变更 → 读 `kv_store.gni` + `distributeddb.gni`（特性开关）
- 依赖增删 → 读 `bundle.json`

### Path-based routing

- `interfaces/jskits/` → 读 `store_errno.h`
- `frameworks/innerkitsimpl/distributeddatasvc/` → 读 `CODEOWNERS`
- `frameworks/innerkitsimpl/kvdb/` → 读 `concurrent_map.h`
- `frameworks/libs/distributeddb/` → 读 `db_errno.h`
- `frameworks/innerkitsimpl/crypt/` → 读 `security_manager.cpp`

### Vocabulary-based routing

任务、issue、日志、API、变更文件中出现以下术语时，在规划前读取对应文档：

| 术语 | 风险提示 | 读 |
|---|---|---|
| Status / 错误码 | innerkitsimpl `Status` enum，distributeddb `int` errno，不可混用 | `store_errno.h` + `db_errno.h` |
| Compute / ConcurrentMap | action 返回 `false` = 删除条目，不是失败 | `concurrent_map.h` |
| IPC 接口码 | 变更必须通知 CODEOWNERS 指定评审人 | `CODEOWNERS` |
| OMIT_MULTI_VER | 永久禁用多版本路径，`KVStoreDelegateImpl` 是死代码 | `kv_store.gni` + `distributeddb/BUILD.gn` |
| dlopen / kv_store_crypt | 加密插件动态加载，缺失时静默失败，不是链接依赖 | `security_manager.cpp` |
| shared_lock / unique_lock | CRUD 用 shared_lock，仅 Close 用 unique_lock | `single_store_impl.cpp` |
| NAPI 回调 | 回调参数必须是最后一个，禁止放中间 | `interfaces/jskits/distributedkvstore/` |
| IPC_SEND | 按引用捕获，禁止传入临时或已 move 对象 | `kvdb_service_client.cpp` |
| Anonymous | udid/uuid/ip/mac/密钥/路径必须用 `StoreUtil::Anonymous()` 匿名化 | `store_util.cpp` |
| sanitize / branch_protector | 主共享库必须完整配置 ubsan+boundary_sanitize+cfi+cfi_cross_dso+branch_protector_ret | 各 `BUILD.gn` |
| dms/dm_service_enable | 分布式调度/设备管理降级开关 | `kv_store.gni` |

规划阶段声明：开始编辑前，在 plan 中说明任务类别、已读文档、发现约束、是否需要使用 skill/workflow。

## 3. Constraints and boundaries

### Architecture/domain invariants

- innerkitsimpl `Status` enum ↔ distributeddb `int` errno；跨层必须经 `StoreUtil::ConvertStatus` 转换，不可混用。
- `OMIT_MULTI_VER` 永久禁用多版本路径。`KVStoreDelegateImpl` 是死代码，不可作为实际路径。
- CRUD 用 `shared_lock`，仅 Close 用 `unique_lock`；禁止给 Put/Delete 加写锁。
- `kv_store_crypt` 通过 `dlopen` 动态加载；缺失时加密静默失败。
- IPC 接口码变更必须通知 CODEOWNERS。
- NAPI 回调参数必须最后一个，禁止放中间。
- 公共 API 表达稳定能力意图，不是内部实现细节。

### Do not

- NEVER 明文打印 udid / uuid / ip / mac / 密钥 / 数据库路径 — 必须用 `StoreUtil::Anonymous()` 匿名化 `[C4]`
- NEVER 在锁内发送 IPC `[C4]`
- NEVER 声称"已完成"而不运行测试 `[E1]`
- NEVER 用户说"调研一下"时直接实现 `[S1]`
- NEVER 编造不存在的 API 签名或 GN 目标名 `[E5]`
- NEVER 为通过测试删除日志、事件、错误码或诊断信息 `[E1]`
- NEVER 改公共 API 签名/错误码/权限行为/生命周期语义，除非任务明确要求 `[C4]`
- NEVER 修改 IPC 接口码而不通知 CODEOWNERS 指定评审人 `[C4]`

Always（强约束，非铁律）：
- Always 将捕获栈变量引用的 lambda 异步到其它线程时，确认引用生命周期安全
- Always 将外部传入裸指针先检查再使用，禁止直接构造为智能指针
- Always 用安全替代方案替代 `realloc` / `alloca`
- Always 引用多版本代码前检查 `OMIT_MULTI_VER` 已禁用，实际路径是 `KvStoreNbDelegateImpl`

### Known Pitfalls

**P1: Compute 返回 false = 删除条目** `[E5]` — Agent 误认为"操作失败"导致静默删除；正确写法：保留条目时 action 返回 true；引入：2024 年

**P2: KVStoreDelegateImpl 是死代码** `[E5]` — OMIT_MULTI_VER 永久禁用多版本路径；实际 Put 路径：`SingleStoreImpl::Put` → `KvStoreNbDelegateImpl::Put`；引入：2025 年

### Ask before

新增生产依赖 / 改公共 API 语义 / 改权限模型或信任边界 / 改协议兼容性或持久化数据格式 / 删除兼容性 shim 或迁移逻辑 / 运行可能影响连接设备的命令。

### 编码约定

- 不可派生加 `final`；共享库 `-fvisibility=hidden` 仅导出必要符号。单函数超 80 行应拆分。
- 错误处理：innerkitsimpl `Status` enum / distributeddb `int` errno；跨层必须用 `StoreUtil::ConvertStatus` 转换。
- 日志：`.cpp` 定义 `LOG_TAG`；敏感数据 MUST 用 `StoreUtil::Anonymous()` 匿名化；innerkitsimpl `%{public}s`/`%{private}s`，distributeddb `s{private}`。
- 命名：PascalCase 方法/类、`camelCase_` 尾下划线成员变量、`UPPER_SNAKE_CASE` 常量；文件 `snake_case`，mock 加 `_mock`。
- 导出：主共享库必须配置 `sanitize = { ubsan, boundary_sanitize, cfi, cfi_cross_dso }` + `branch_protector_ret = "pac_ret"`；dm/dms/crypt/taihe 尚未全量覆盖，新增目标须补齐。
- 命名空间：innerkitsimpl `OHOS::DistributedKv`；distributeddb `DistributedDB`。include guard：innerkitsimpl `OHOS_DISTRIBUTED_DATA_*_H`；distributeddb `DISTRIBUTEDDB_*_H`。
- `IPC_SEND` 按引用捕获，禁止传入临时或已 move 对象。版权头：Apache 2.0 14 行。Mock: `kvdb/test/mock/include/`+`src/`，`B*`→`*Mock`→`MOCK_METHOD`。

## 4. Verification

构建/测试命令必须从根 `BUILD.gn` 或 `bundle.json` 核对真实 GN 目标名后再执行。

| 目标 | 用途 | 命令 |
|---|---|---|
| `kv_store_test` | 部件级全量单测（由构建框架从 `bundle.json` test 条目聚合） | `./build.sh --product-name <product> --ccache --build-target kv_store_test` |
| `unittest` | innerkitsimpl + common 单测 | `./build.sh --product-name <product> --build-target unittest` |
| `build_native_test` | distributeddatafwk + distributeddb 原生单测 | `./build.sh --product-name <product> --build-target build_native_test` |
| `fuzztest` | 全量 fuzz 测试 | `./build.sh --product-name <product> --build-target fuzztest` |
| `distributedtest` | 分布式跨设备集成测试 | `./build.sh --product-name <product> --build-target distributedtest` |

- 提交前必跑：`./build.sh --product-name <product> --ccache --build-target kv_store_test`（如可本地执行）
- CI 门禁本地等价：`./build.sh --product-name <product> --gn-args use_thin_lto=false --ccache --build-target kv_store_test`
- 构建单个模块：`./build.sh --product-name <product> --build-target <gn_target>`（先确认最近的 `BUILD.gn` 中存在该目标）
- Lint/static check：无独立 lint 工具；编译期 `-Werror=vla` + sanitize + `-fvisibility=hidden` 集成在构建中
- 禁止运行宽泛产品级全量构建，除非用户明确要求

单元测试约定：C++ GTest+GMock (`ohos_unittest`) / JS `ohos_js_unittest`；文件 `_test.cpp` / `*JsTest.js`；TDD: RED (MUST 看到失败 `[E1]`) → GREEN → IMPROVE。

Task-specific checks:

| 任务类型 | 验证要求 |
|---|---|
| 公共 API 变更 | 构建 + 跑单测 + 检查 `store_errno.h` 错误码兼容性 + 更新 API 文档 |
| C++ 存储引擎变更 | 构建 `distributeddb` + 跑 `build_native_test` + 跑附近单测 |
| IPC 接口码变更 | 构建 + 跑单测 + 通知 CODEOWNERS 评审人 |
| 日志 / DFX 变更 | 构建 + 跑附近单测 + 不可删除已有日志/事件/错误码 |
| 加密行为变更 | 构建 `kv_store_crypt` + 跑 security_manager 测试 + 验证 dlopen fallback |
| 跨设备行为变更 | 跑 `distributedtest`（如可执行）+ 跑 `unittest` |
| 仅测试变更 | 跑变更的测试 + 至少一个附近相关测试 |
| NAPI JS API 变更 | 构建 jskits 模块 + 跑 JS 单测 + 验证回调参数位置 |

Done definition：任务完成仅当 — 行为已实现；相关构建/测试已运行或已说明无法运行原因；最终回复包含变更摘要、变更文件、验证结果、剩余风险；不包含无关格式化/重构/顺手改动。

Final response format：完成非平凡任务时回复包含 — 变更摘要 / 变更文件列表 / 验证命令与结果 / 兼容性/权限/DFX/跨设备影响（如相关） / 剩余风险或后续事项。

环境变量：`<product>` — 传给 `./build.sh --product-name` 的产品占位符（示例 `rk3568`）。

## 外部依赖

| 组件 | 用途 | 降级策略 |
|---|---|---|
| SQLite | 底层存储引擎 | 无降级 |
| OpenSSL | 数据库加密 | 无加密模式 fallback |
| IPC/SAMGR/HiLog/C Utils/NAPI/HUKS/libuv/Bounds Checking | 服务调用/日志/工具/绑定/密钥/IO/安全字符串 | 无降级 |
| Device Manager | 设备管理 | dm_service_enable=false 时 mock |
| DMS Framework | 分布式调度 | dms_service_enable=false 时 mock |

## 文档索引

新增核心文档时应放入 `docs/`。当前高频仓库文档如下：

| 路径 | 标题 | 加载场景 | 稳定性 |
|---|---|---|---|
| `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md` | PR 稳定性/日志/安全自检清单 | 提交 PR 前 | 稳定 |
| `interfaces/innerkits/distributeddata/include/store_errno.h` | Status 错误码定义 | 修改 innerkitsimpl 错误处理 | 稳定 |
| `frameworks/libs/distributeddb/common/include/db_errno.h` | DB 层 errno 定义 | 修改 distributeddb 错误处理 | 稳定 |
| `frameworks/common/log_print.h` | 日志宏与 LogLabel 定义 | 新增日志或修改日志 domain | 稳定 |
| `frameworks/common/concurrent_map.h` | ConcurrentMap API 与 Compute 语义 | 修改并发容器逻辑 | 稳定 |
| `CODEOWNERS` | IPC 接口码评审归属 | 修改 IPC 接口码 | 稳定 |
| `kv_store.gni` / `distributeddb.gni` | GN 编译参数与特性开关 | 修改构建配置 | 稳定 |
| `bundle.json` | 部件声明与依赖列表 | 新增/删除依赖 | 中等 |
| `README_zh.md` | 项目简介与目录说明 | 新人了解项目 | 稳定 |

新增、删除或重命名文档时，必须在同一 PR 中更新本索引。