# AGENTS.md

本文件是 AI Agent 处理本仓库任务时的轻量入口。先读本文件，再按任务类型只加载匹配的详细文档页。

## 阅读策略

不要一开始就读取 `docs/agents/` 下的所有文件。

默认只读本文件。涉及需求设计或代码开发时，最多按需加载：

1. 如果影响范围不清楚，读取 `docs/agents/architecture.md` 和 `docs/agents/code-map.md`。
2. 读取一个与任务领域匹配的专题页。
3. 规划验证时，读取 `docs/agents/build-and-test.md`。

本仓库内容较多，一次性加载全部背景知识会浪费上下文，也会降低后续实现的精度。

## 仓库定位

`distributeddatamgr_kv_store` 是 OpenHarmony 分布式键值数据库组件。在 OpenHarmony 源码树中的位置：

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

## 知识路由

按任务类型决定下一步读取哪个文档：

| 任务或问题                 | 读取 |
|-----------------------| --- |
| 仓库背景、分层、错误码边界、约束、已知陷阱 | `docs/agents/architecture.md` |
| 按目录理解职责和关键入口文件        | `docs/agents/code-map.md` |
| 构建、或做最小验证             | `docs/agents/build-and-test.md` |

## 硬约束

- IPC 接口码变更必须通知 CODEOWNERS。
- NAPI 回调参数必须最后一个，禁止放中间。
- NEVER 明文打印 udid/uuid/ip/mac/密钥/数据库路径 — 必须用 `StoreUtil::Anonymous()` 匿名化。
- NEVER 在锁内发送 IPC。
- NEVER 为通过测试删除日志、事件、错误码或诊断信息。
- NEVER 改公共 API 签名/错误码/权限行为/生命周期语义，除非任务明确要求。
- 用户只要求调研、检视或对比时，禁止直接实现。
- 优先选择最小改动，不要顺手重构、加注释、调格式。

## 最小验证习惯

声明完成前，至少在仓库根目录运行结构检查：

涉及代码修改时，读取 `docs/agents/build-and-test.md`，选择覆盖改动区域的最小 GN 构建或单元测试目标。

