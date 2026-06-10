# 架构说明

当任务需要仓库背景、错误码边界、约束或影响面分析时，读取本页。

## 领域角色

`kv_store` 是 OpenHarmony 分布式键值数据库组件。它为应用提供键值对数据管理能力：
CRUD、数据同步、云同步、Observer、ResultSet、加密、ACL 权限。

本组件不是独立桌面库。它预期放在 OpenHarmony 源码树中构建：

```text
//foundation/distributeddatamgr/kv_store
```

许多实现依赖 OpenHarmony 服务：ability runtime、bundle manager、access token、
device manager、dmsfwk、data share、hilog、hisysevent、hitrace、ipc、napi、
ANI/Taihe 和 system ability 基础设施。

## 错误码转换

innerkitsimpl 返回 `Status` enum（定义在 `store_errno.h`），
distributeddb 返回 `int` errno（定义在 `db_errno.h`）。
跨层调用必须经 `StoreUtil::ConvertStatus` 转换，不可直接混用。

这是本仓库最容易出错的边界。违反会导致：
- 语义错误（enum 值 ≠ errno 值）
- 跨层传播不一致
- 静默丢错

## 锁模式

CRUD 操作用 `shared_lock`（读锁），仅 Close 用 `unique_lock`（写锁）。
禁止给 Put/Delete 加写锁。这是并发性能的关键设计决策。

## 加密降级

`kv_store_crypt` 通过 `dlopen` 动态加载（`security_manager.cpp`），不是链接依赖。
缺失时 `GetDelegate()` 返回 nullptr，`GetDBPassword` 返回空密码，加密数据库打开失败。

## 降级开关

`kv_store.gni` 定义三个降级开关：

- `dm_service_enable`：设备管理适配，`false` 时 mock 替代。
- `dms_service_enable`：分布式调度适配，`false` 时 mock 替代。


## IPC 安全

- IPC 接口码变更必须通知 CODEOWNERS 指定评审人。
- `IPC_SEND` 按引用捕获，禁止传入临时或已 move 对象（见 `kvdb_service_client.cpp`）。
- NEVER 在锁内发送 IPC。

## 日志与隐私

- `.cpp` 定义 `LOG_TAG`。
- udid/uuid/ip/mac/密钥/数据库路径 MUST 用 `StoreUtil::Anonymous()` 匿名化后再输出。
- NEVER 明文打印隐私数据。

## 编码约定

- 稳定性排查、日志打印排查、安全编码自检的完整清单见 `.gitee/PULL_REQUEST_TEMPLATE.zh-CN.md`。
- 命名：PascalCase 方法/类、`camelCase_` 尾下划线成员变量、`UPPER_SNAKE_CASE` 常量；文件 `snake_case`，mock 加 `_mock`。
- 共享库 `-fvisibility=hidden` 仅导出必要符号。

## 影响面分析清单

实现前先回答这些问题：

- 改动影响哪个 API 面：NAPI JS、ANI/Taihe、CJ FFI、InnerKit，还是仅内部？
- 是否涉及两层错误码（Status vs errno）的转换？
- 是否涉及 IPC 接口码变更（需要 CODEOWNERS 评审）？
- 是否涉及锁模式变更（shared_lock vs unique_lock）？
- 是否涉及隐私数据日志？
- 哪个 GN target 是覆盖该改动的最小有效构建或测试目标？