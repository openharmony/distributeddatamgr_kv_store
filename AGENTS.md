# distributeddatamgr_kv_store — Agent Instructions

OpenHarmony 分布式键值数据库组件，为应用提供 KV 存储、设备协同、端端同步、云同步能力。C++ 原生引擎 + JS/ArkTS API 双层架构。

## Core Principles <!-- [S1,E4] -->

1. **意图优先**：用户说"调研一下"时不应直接实现 `[S1]`
2. **验证驱动**：声称"已完成"前应先运行测试 `[E1]`
3. **最小改动**：只改要求改的，不要顺手重构、加注释、调格式 `[E4]`

## Modification Timing <!-- [E4] -->

- 应该在代码修改完成之后更新 AGENTS.md，不要先改文档再改代码
- 同一 PR 同步更新条件：一级目录/模块增删重命名、构建/测试/CI 命令变更、核心依赖增删、踩坑第2次出现、探索总结第3次求证、跨模块依赖增删
- 仅涉及实现细节不影响接口/构建/依赖的改动，不应修改 AGENTS.md

## Repository Guide <!-- [C1,E5] -->

| 项 | 值 |
|---|---|
| 语言 | C++17 / JavaScript(ArkTS) |
| 构建系统 | GN + Ninja (OHOS `//build/ohos.gni`) |
| 包定义 | `bundle.json` (name: `@ohos/kv_store`, version: `3.1.0`) |
| 子系统 | `distributeddatamgr` / part: `kv_store` |
| 编译特性 | `kv_store_cloud` / `kv_store_device` |

### 目录结构

```
├── frameworks/          # 框架层 (common, innerkitsimpl, libs/distributeddb, ets, cj, jskitsimpl)
├── interfaces/          # 接口声明 (innerkits, jskits, inner_api)
├── databaseutils/       # ACL 工具 (acl.h)
├── kvstoremock/         # JS API mock 层
├── test/                # unittest / fuzztest / distributedtest
└── kv_store.gni         # GN 编译参数 (dms/dm_service_enable)
```

### 核心依赖（卸载后系统无法工作）

| 依赖 | 用途 |
|---|---|
| sqlite | 底层 KV/关系存储引擎 |
| openssl | 数据库加密 (HUKS 密钥管理) |
| hilog | 日志输出基础设施 |
| ipc | 进程间通信 (客户端-服务端代理) |
| device_manager | 设备发现与认证 |
| c_utils | 基础工具类 (RefBase, Utils) |
| napi / ets_frontend | JS/ArkTS 绑定与运行时 |
| bounds_checking_function | 安全字符串操作 (memcpy_s 等) |

### 构建 & 测试命令

| 命令 | 用途 | 备注 |
|---|---|---|
| `./build.sh --product-name rk3568 --gn-args use_thin_lto=false --ccache --build-target kv_store_test` | 单测 | 提交前必跑 |

测试框架：C++ 单测 **GTest+GMock** (`ohos_unittest`)；JS 单测 **ohos_js_unittest**。
Mock：基类 `B*` → Mock 类 `*Mock` → `MOCK_METHOD`，存放 `test/mock/include/` + `test/mock/src/`
TDD：1) RED MUST see test fail `[E1]` → 2) GREEN 最小实现 → 3) IMPROVE 重构

## CI Requirements <!-- [E1] -->

CI 平台：OHOS CI；流水线配置：`build/sh/build.sh`

| CI 任务 | 本地等价命令 |
|---|---|
| 全部单测 | `./build.sh --product-name rk3568 --gn-args use_thin_lto=false --ccache --build-target kv_store_test` |

CI 命令：`hb build kv_store -t`
提交前必跑：本地 `kv_store_test`（如可本地执行）

## Environment Variables <!-- [E5,C4] -->

本项目不使用环境变量；构建依赖 OHOS 全量编译环境 `out/<product>/`。
Required: 不硬编码路径、UDID、UUID、密钥在测试或示例中 `[C4]`

## Exploration Notes <!-- [C1] -->

### 探索总结：KVStore 数据写入完整路径
- 问题：put 操作从 JS API 到 SQLite 存储经过哪些模块？
- 结论：`jskits/distributedkvstore` → NAPI → `SingleStoreImpl::Put` → `KVStoreDelegateImpl::Put` → `SQLiteSingleVerNaturalStore`
- 关键路径：`interfaces/jskits/distributedkvstore/`、`frameworks/innerkitsimpl/kvdb/src/single_store_impl.cpp`、`frameworks/libs/distributeddb/interfaces/src/kv_store_delegate_impl.cpp`
- 失效条件：NAPI 绑定重构 / delegate 接口变更 / 存储引擎替换

### 探索总结：日志系统选择
- 问题：innerkitsimpl 和 distributeddb 日志宏为什么不同？
- 结论：innerkitsimpl 用 `ZLOGD/ZLOGI/ZLOGW/ZLOGE`（domain `0xD001610-0xD001620`）；distributeddb 用 `LOGD/LOGI/LOGW/LOGE/LOGF`（domain `0xD001630`）
- 关键路径：`frameworks/common/log_print.h`、`frameworks/libs/distributeddb/common/include/log_print.h`
- 失效条件：日志基础设施统一

## Known Pitfalls <!-- [E1,E5,S4] -->

### 踩坑：Status 与 DBStatus 混用
- 现象：修改 innerkitsimpl 代码时误用 `DistributedDB::E_OK` 判断返回值
- 根因：innerkitsimpl 用 `Status` enum（`store_errno.h`），distributeddb 用 `E_*` errno（`db_errno.h`），两套体系不互通
- 反模式：NEVER 在 innerkitsimpl 层判断 `E_OK`，必须用 `Status::SUCCESS`
- 正确写法：`StoreUtil::ConvertStatus(dbStatus)` 转换后再判断
- 引入时机：2026-05

### 踩坑：LOG_TAG 未定义导致日志异常
- 现象：日志输出 tag 为空或乱码
- 根因：.cpp 顶部 Required 定义 `#define LOG_TAG "ClassName"`，缺少则 HiLog 无法归类
- 反模式：NEVER 省略 `LOG_TAG` 定义
- 正确写法：紧接版权头后 `#define LOG_TAG "SingleStoreImpl"`
- 引入时机：2026-05

### 踩坑：IPC 接口码变更未走 CODEOWNERS
- 现象：修改 IPC interface code 后未通知评审人，导致合入冲突
- 根因：`CODEOWNERS` 规定 IPC 接口码文件变更需 @leonchan5 评审
- 反模式：NEVER 修改 IPC 接口码文件而不通知 CODEOWNERS 评审人
- 正确写法：修改前先与 `liubao6@huawei.com` / `@leonchan5` 沟通
- 引入时机：2026-05

## Coding Conventions <!-- [C2] -->

| 维度 | 规则 |
|---|---|
| 不可变性 | 不可派生加 `final`；共享库 `-fvisibility=hidden` 仅导出必要符号 |
| 规模上限 | 单函数超过 80 行应拆分 |
| 错误处理 | innerkitsimpl 返回 `Status` enum；distributeddb 返回 `int` errno；跨层 Required 用 `StoreUtil::ConvertStatus` 转换 `[C1]` |
| 日志 | Required 定义 `LOG_TAG`；敏感数据 MUST 用 `StoreUtil::Anonymous()` 匿名化；`%{public}s` 公开、`%{private}s` 私有 `[C4]` |
| 文件命名 | `snake_case`（`single_store_impl.h/.cpp`）；mock 加 `_mock`；guard: innerkitsimpl `OHOS_DISTRIBUTED_DATA_*_H`，distributeddb `DISTRIBUTEDDB_*_H` |
| 模块导出 | 共享库必须配置 `sanitize = { ubsan, boundary_sanitize, cfi, cfi_cross_dso }` + `branch_protector_ret = "pac_ret"` |
| 命名-类/方法 | `PascalCase`（`SingleStoreImpl`、`GetStoreId`）；成员变量 `camelCase_`（`dbStore_`）；常量 `UPPER_SNAKE_CASE` |
| 命名-命名空间 | innerkitsimpl: `OHOS::DistributedKv`；distributeddb: `DistributedDB` |
| 版权头 | Apache 2.0 14 行版权头，年份取文件创建年份 |
| IPC 码 | Required: 变更必须通知 CODEOWNERS 指定评审人 |

## Anti-Patterns <!-- [C2,E5,S4] -->

### 语法级
- NEVER print plaintext udid / uuid / ip / mac / key / db path — MUST anonymize `[C4]`
- NEVER send IPC inside a lock `[C4]`
- NEVER pass stack-reference lambda to async thread `[C4]`
- NEVER construct smart pointer from raw pointer passed from outside `[C4]`
- Should avoid `realloc` / `alloca`，prefer safe alternatives `[C4]`

### 认知级
- NEVER claim "done" without running tests `[E1]`
- NEVER implement when user said "调研一下" `[S1]`
- NEVER fabricate API signatures `[E5]`
- Should push back when seeing problems，不要讨好式同意 `[S4]`

### 合理化对照表
| Agent 会说的借口 | 现实 |
|---|---|
| "太简单不需要测试" | 简单代码也会坏，写测试只要 30 秒 |
| "我先写完再补测试" | 事后补的测试什么也证明不了 |
| "手动测过了" | 没有记录、不可重复的测试不算测试 |
| "这次例外" | 没有例外 |

## Internal Module Dependencies <!-- [C1] — 无循环依赖 -->

| 模块 | 能力 | 依赖 | 被谁依赖 |
|---|---|---|---|
| common | 日志/类型转换/任务调度 | hilog | innerkitsimpl/*, distributeddb |
| distributeddb | KV 存储引擎 + 数据同步 | common, sqlite, openssl | innerkitsimpl/kvdb, distributeddatafwk |
| innerkitsimpl/kvdb | KV 数据库客户端框架 | common, distributeddb | jskits/distributedkvstore |
| innerkitsimpl/distributeddatafwk | 分布式数据管理框架 | common, kvdb | jskits/distributeddata |
| innerkitsimpl/distributeddatasvc | 服务端 IPC 代理 | common, ipc | datamgr_service |
| innerkitsimpl/crypt | 加密/解密工具 | common, openssl | kvdb |
| innerkitsimpl/dm | 设备管理适配 | common, device_manager | kvdb, distributeddatafwk |
| innerkitsimpl/dms | 分布式调度适配 | common, dmsfwk | distributeddatafwk |
| databaseutils | ACL 权限工具 | c_utils | innerkitsimpl/kvdb |
| jskits/distributedkvstore | JS API 层 (NAPI) | kvdb | 应用层 |
| jskits/distributeddata | JS API 层 (旧版 NAPI) | distributeddatafwk | 应用层 |

## External Dependencies <!-- [C1,E5] -->

| 组件 | 用途 | 版本范围 | 关键 | 降级策略 |
|---|---|---|---|---|
| sqlite | 底层存储引擎 | 3.x | 是 | 无降级 |
| openssl | 数据库加密 | 1.1.x | 是 | 无加密模式 fallback |
| hilog | 日志基础设施 | OHOS内置 | 是 | 无降级 |
| ipc | 进程间通信 | OHOS内置 | 是 | 无降级 |
| device_manager | 设备发现/认证 | OHOS内置 | 是 | dms_service_enable=false 时禁用同步 |
| dmsfwk | 分布式调度框架 | OHOS内置 | 是 | dm_service_enable=false 时禁用 |
| c_utils | 基础工具类 | OHOS内置 | 是 | 无降级 |
| napi | JS 绑定框架 | OHOS内置 | 是 | 无降级 |
| bounds_checking_function | 安全字符串函数 | OHOS内置 | 是 | 无降级 |
| huks | 密钥管理 | OHOS内置 | 是 | 无降级 |
| libuv | 异步 I/O | OHOS内置 | 否 | 无降级 |

## Docs Index <!-- 按命中频率排序 -->

| 路径 | 标题 | 加载场景 | 稳定性 |
|---|---|---|---|
| `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md` | PR 稳定性/日志/安全自检清单 | 提交 PR 前 | 稳定 |
| `interfaces/innerkits/distributeddata/include/store_errno.h` | Status 错误码定义 | 修改 innerkitsimpl 错误处理 | 稳定 |
| `frameworks/libs/distributeddb/common/include/db_errno.h` | DB 层 errno 定义 | 修改 distributeddb 错误处理 | 稳定 |
| `frameworks/common/log_print.h` | 日志宏与 LogLabel 定义 | 新增日志或修改日志 domain | 稳定 |
| `CODEOWNERS` | IPC 接口码评审归属 | 修改 IPC 接口码 | 稳定 |
| `kv_store.gni` / `distributeddb.gni` | GN 编译参数与特性开关 | 修改构建配置 | 稳定 |
| `bundle.json` | 部件声明与依赖列表 | 新增/删除依赖 | 稳定 |
| `README_zh.md` | 项目简介与目录说明 | 新人了解项目 | 稳定 |