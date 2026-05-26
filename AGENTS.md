# distributeddatamgr_kv_store — Agent 指令

OpenHarmony 分布式键值数据库组件，为设备应用提供键值对数据管理能力。本文档是 Agent 行为护栏，不是项目 README。

## 核心原则

- 用户只要求调研、检视或对比时，禁止直接实现；必须等用户明确要求改代码。
- 优先选择最小改动，不要顺手重构、加注释、调格式。
- 引用构建或测试命令前，必须从 `BUILD.gn` 核对真实目标名。
- `AGENTS.md` 只记录 Agent 无法从代码推导出的约束；长解释应沉淀到 `docs/`。

## 修改时机

- 必须在代码修改完成之后更新 AGENTS.md，禁止先改文档再改代码
- 同一 PR 同步更新条件：一级目录/模块增删重命名、构建/测试/CI 命令变更、核心依赖增删、踩坑第2次出现、探索总结第3次求证、跨模块依赖增删
- 仅涉及实现细节不影响接口/构建/依赖的改动，不应修改 AGENTS.md

## 仓库指引

| 项目 | 内容 |
|---|---|
| 主要语言 | C++17、ArkTS/ETS、JavaScript 绑定代码、GN |
| 构建系统 | OpenHarmony `./build.sh` 驱动 GN |
| 包管理器 | 本组件无独立包管理器；依赖来自 OpenHarmony bundle 组件 |
| 组件信息 | `@ohos/kv_store`，版本 `3.1.0`，子系统 `distributeddatamgr` |
| 对外接口层 | InnerKit、NAPI、CJ FFI、Taihe/ANI |
| 编译特性 | `kv_store_cloud` / `kv_store_device` (`kv_store.gni` `declare_args`) |

关键依赖：

| 依赖 | 用途 |
|---|---|
| `sqlite` | 底层 KV/关系存储引擎 |
| `openssl` | 数据库加密 (HUKS 密钥管理) |
| `ipc` / `samgr` | 进程间通信与服务发现 |
| `device_manager` | 设备发现与认证 |
| `hilog` | 日志输出基础设施 |
| `c_utils` | 基础工具类 (RefBase, Utils) |
| `napi` / `ets_frontend` | JS/ArkTS API 暴露与运行时集成 |
| `bounds_checking_function` | 安全字符串操作 (memcpy_s 等) |

构建与测试命令：

| 命令 | 用途 | 备注 |
|---|---|---|
| `./build.sh --product-name <product> --build-target kv_store_test` | 单测 | 提交前必跑 |
| `./build.sh --product-name <product> --build-target <gn_target>` | 构建单个目标 | 先确认最近的 `BUILD.gn` 中存在该目标 |

禁止在 Agent 会话中运行宽泛的 OpenHarmony 产品级全量构建，除非用户明确要求承担该成本。

单元测试约定：

| 项目 | 规则 |
|---|---|
| 测试框架 | C++ 用 GTest+GMock (`ohos_unittest`)；JS 用 `ohos_js_unittest` |
| 文件命名 | 测试文件以 `_test.cpp` 或 `*JsTest.js` 结尾 |
| 存放目录 | `framework/*/test/`、`test/unittest/`、`test/fuzztest/` |
| Mock 目录 | `test/mock/include/` + `test/mock/src/`，基类 `B*` → Mock 类 `*Mock` → `MOCK_METHOD` |
| TDD | RED：先写或调整测试并必须看到失败；GREEN：最小实现；IMPROVE：不改行为地重构 |

## CI 要求

CI 平台：OHOS CI；流水线配置：`build/sh/build.sh`

| 门禁 | 本地等价命令 |
|---|---|
| 全部单测 | `./build.sh --product-name <product> --gn-args use_thin_lto=false --ccache --build-target kv_store_test` |

提交前必跑：本地 `kv_store_test`（如可本地执行）

## 环境变量

本组件没有 kv_store 专属环境变量。

| 名称 | 必需 | 用途 | 示例值 | 来源 |
|---|---|---|---|---|
| `<product>` | 是 | 传给 `./build.sh --product-name` 的产品占位符 | `rk3568` | OpenHarmony 工作区或产品选择 |

禁止在本文档中写入密钥、Token、真实服务端点或生产专用变量。
## 探索总结

### KVStore 数据写入完整路径
- 问题：put 操作从 JS API 到 SQLite 存储经过哪些模块？
- 结论：`jskits/distributedkvstore` → NAPI → `SingleStoreImpl::Put` → `KVStoreDelegateImpl::Put` → `SQLiteSingleVerNaturalStore`
- 关键路径：`interfaces/jskits/distributedkvstore/`、`frameworks/innerkitsimpl/kvdb/src/single_store_impl.cpp`、`frameworks/libs/distributeddb/interfaces/src/kv_store_delegate_impl.cpp`
- 失效条件：NAPI 绑定重构 / delegate 接口变更 / 存储引擎替换

### ConcurrentMap::Compute 返回值语义
- 问题：`ConcurrentMap::Compute` lambda 返回 `false` 是什么意思？
- 结论：返回 `false` 表示"从 map 中删除该条目"，不是"操作失败"。Agent 误认为"失败"会导致条目被静默删除。
- 关键路径：`frameworks/common/concurrent_map.h`
- 失效条件：ConcurrentMap API 变更

### CloudSyncer 锁顺序
- 问题：CloudSyncer 的两个锁有顺序约束吗？
- 结论：隐式锁顺序 `syncMutex_` > `dataLock_`；`DoSync` 先拿 `syncMutex_` 再拿 `dataLock_`，反序会死锁。
- 关键路径：`frameworks/libs/distributeddb/syncer/src/cloud/cloud_syncer.cpp`
- 失效条件：锁结构重构

## 编码约定

- 不可变性：不可派生加 `final`；共享库 `-fvisibility=hidden` 仅导出必要符号。
- 规模：单函数超过 80 行应拆分；仅当抽取函数有稳定职责和可测试边界时才拆分。
- 错误处理：innerkitsimpl 返回 `Status` enum；distributeddb 返回 `int` errno；跨层 Required 用 `StoreUtil::ConvertStatus` 转换。
- 日志：需要打日志的 `.cpp` 定义 `LOG_TAG`；敏感数据 MUST 用 `StoreUtil::Anonymous()` 匿名化；innerkitsimpl 用 `%{public}s`/`%{private}s`，distributeddb 用 `s{private}`。
- 命名：沿用 PascalCase 方法/类、`camelCase_` 尾下划线成员变量、`UPPER_SNAKE_CASE` 常量；文件 `snake_case`，mock 加 `_mock`。
- 导出：共享库 MUST 配置 `sanitize = { ubsan, boundary_sanitize, cfi, cfi_cross_dso }` + `branch_protector_ret = "pac_ret"`。
- 命名空间：innerkitsimpl: `OHOS::DistributedKv`；distributeddb: `DistributedDB`。
- include guard：innerkitsimpl `OHOS_DISTRIBUTED_DATA_*_H`；distributeddb `DISTRIBUTEDDB_*_H`。
- IPC 码：变更 Required 通知 CODEOWNERS 指定评审人。
- 版权头：Apache 2.0 14 行版权头，年份取文件创建年份。
- 锁模式：CRUD 操作用 `shared_lock`，仅 `Close` 用 `unique_lock`；禁止给 Put/Delete 加写锁。
- NAPI 回调：NAPI 函数签名中回调必须是最后一个参数，禁止放在中间位置。
- IPC 宏：`IPC_SEND` 按引用捕获参数，禁止传入临时变量或已 move 的对象。
- 编译宏：`OMIT_MULTI_VER` 永久禁用多版本代码路径，多版本源码虽存在但不可用。
- 加密插件：`kv_store_crypt` 通过 `dlopen` 动态加载，不是链接依赖；缺失时加密静默失败。

## 反模式

语法级：
- NEVER 明文打印 udid / uuid / ip / mac / 密钥 / 数据库路径 — 必须匿名化。
- NEVER 在锁内发送 IPC。
- NEVER 将捕获栈变量引用的 lambda 异步到其他线程。
- NEVER 将外部传入的裸指针直接构造为智能指针。
- NEVER `ConcurrentMap::Compute` lambda 返回 `false` 表示"删除该条目"，不是"操作失败"。
- 禁止使用 `realloc` / `alloca`，用安全替代方案。

认知级：
- NEVER 声称"已完成"而不运行测试。
- NEVER 用户说"调研一下"时直接实现。
- NEVER 编造不存在的 API 签名或 GN 目标名。
- 看到问题必须推回，不要讨好式同意。

合理化对照表：

| Agent 借口 | 现实 |
|---|---|
| "太简单不需要测试" | 单代码也会坏，写测试只要 30 秒 |
| "我先写完再补测试" | 事后补的测试什么也证明不了 |
| "手动测过了" | 没有记录、不可重复的测试不算测试 |
| "目标名很明显" | 根 `BUILD.gn` 才是真实来源 |
| "Compute 返回 false 表示失败" | 返回 false 表示删除条目 |

## 内部模块依赖

| 模块 | 能力 | 依赖 | 被谁依赖 |
|---|---|---|---|
| `common` | 日志、类型转换、任务调度 | hilog | innerkitsimpl/*, distributeddb |
| `distributeddb` | KV 存储引擎 + 数据同步 | common, sqlite, openssl | innerkitsimpl/kvdb, distributeddatafwk |
| `innerkitsimpl/kvdb` | KV 数据库客户端框架 | common, distributeddb | jskits/distributedkvstore |
| `innerkitsimpl/distributeddatafwk` | 分布式数据管理框架 | common, kvdb | jskits/distributeddata |
| `innerkitsimpl/distributeddatasvc` | 服务端 IPC 代理 | common, ipc | datamgr_service |
| `innerkitsimpl/crypt` | 加密/解密工具 | common, openssl | kvdb |
| `innerkitsimpl/dm` | 设备管理适配 | common, device_manager | kvdb, distributeddatafwk |
| `innerkitsimpl/dms` | 分布式调度适配 | common, dmsfwk | distributeddatafwk |
| `databaseutils` | ACL 权限工具 | c_utils | innerkitsimpl/kvdb |
| `jskits/distributedkvstore` | JS API 层 (NAPI) | kvdb | 应用层 |
| `jskits/distributeddata` | JS API 层 (旧版 NAPI) | distributeddatafwk | 应用层 |

当前未记录有意设计的循环依赖。若发现循环依赖，先创建 issue 并在此记录，再继续扩大依赖。

## 外部依赖

| 组件 | 用途 | 版本范围 | 关键路径 | 降级策略 |
|---|---|---|---|---|
| SQLite | 底层存储引擎 | 3.x | 是 | 无降级 |
| OpenSSL | 数据库加密 | 1.1.x | 是 | 无加密模式 fallback |
| OpenHarmony IPC/SAMGR | 服务调用与发现 | 平台提供 | 是 | 无降级 |
| Device Manager | 设备发现与认证 | 平台提供 | 是 | dms_service_enable=false 时禁用同步 |
| DMS Framework | 分布式调度 | 平台提供 | 是 | dm_service_enable=false 时禁用 |
| HiLog | 日志基础设施 | 平台提供 | 是 | 无降级 |
| C Utils | 基础工具类 | 平台提供 | 是 | 无降级 |
| NAPI/ETS Runtime | JS/ArkTS 绑定 | 平台提供 | 是 | 无降级 |
| Bounds Checking Function | 安全字符串函数 | 平台提供 | 是 | 无降级 |
| HUKS | 密钥管理 | 平台提供 | 是 | 无降级 |
| libuv | 异步 I/O | 平台提供 | 否 | 无降级 |
| MCP Server | 本仓不要求 | N/A | 否 | N/A |

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