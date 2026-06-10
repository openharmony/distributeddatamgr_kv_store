# 代码地图

当需要在设计或编码前定位文件时，读取本页。

## 根目录文件

- `bundle.json`：组件定义、syscap、feature flag、依赖、构建 group、inner kit 和测试目标。
- `kv_store.gni`：共享 GN 变量，含 `dm_service_enable`、`dms_service_enable`、`qemu_disable`。
- `frameworks/libs/distributeddb/distributeddb.gni`：distributeddb 源码聚合与 feature flag。
- `BUILD.gn`：顶层测试 group 定义（`unittest`、`build_native_test`、`distributedtest`、`fuzztest`）。
- `CODEOWNERS`：IPC 接口码评审归属。

## `interfaces/jskits/`

JS 入口和 BUILD.gn 打包层。NAPI C++ 实现不在此处，见 `frameworks/jskitsimpl/`。

- `distributeddata/`：旧版 JS 入口 + `BUILD.gn`。

## `frameworks/jskitsimpl/distributedkvstore/`

现代 `@ohos.distributedKVStore` NAPI C++ 实现。兼容性敏感区域。

关键文件：
- `src/entry_point.cpp`：模块注册。
- `src/js_single_kv_store.cpp`、`src/js_device_kv_store.cpp`：KVStore NAPI 绑定。
- `src/js_query.cpp`：Query NAPI 绑定。
- `src/js_kv_manager.cpp`：KVManager NAPI 绑定。
- `src/js_observer.cpp`：Observer NAPI 绑定。

## `frameworks/jskitsimpl/distributeddata/`

旧版 `@ohos.distributedData` NAPI C++ 实现。兼容区域，除非任务明确要求，不要重构。

## `frameworks/innerkitsimpl/kvdb/`

KV 客户端框架，CRUD 入口。这是最频繁修改的区域。

关键文件：
- `include/single_store_impl.h` / `src/single_store_impl.cpp`：SingleStoreImpl，KVStore 核心实现。
- `include/kvdb_service_client.h` / `src/kvdb_service_client.cpp`：服务端 IPC 代理。
- `include/store_util.h` / `src/store_util.cpp`：工具类，含 `ConvertStatus`（错误码转换）和 `Anonymous`（隐私数据匿名化）。
- `src/security_manager.cpp`：加密管理，`kv_store_crypt` 通过 dlopen 动态加载。
- `include/kvdb_service.h`：IPC 接口定义。
- `include/distributeddata_kvdb_ipc_interface_code.h`：IPC 接口码。

测试：`test/`。

## `frameworks/innerkitsimpl/distributeddatafwk/`

分布式数据管理框架。旧版 JS API 的客户端框架。

关键文件：
- `src/distributed_kv_data_manager.cpp`：分布式数据管理入口。
- `src/kvdb_notifier_client.cpp`：Observer 通知客户端。
- `src/kvstore_service_death_notifier.cpp`：服务死亡通知。

测试：`test/`。

## `frameworks/innerkitsimpl/distributeddatasvc/`

服务端 IPC 代理。IPC 接口码变更需 CODEOWNERS 评审。

关键文件：
- `include/distributeddata_ipc_interface_code.h`：IPC 接口码。
- `include/ikvstore_data_service.h`：IPC 服务接口。
- `include/kvstore_data_service_mgr.h`：服务管理。

## `frameworks/innerkitsimpl/crypt/`

加密/解密工具。`kv_store_crypt` 通过 dlopen 动态加载。

## `frameworks/innerkitsimpl/dm/`

设备管理适配。`dm_service_enable=false` 时 mock 替代。

## `frameworks/innerkitsimpl/dms/`

分布式调度适配。`dms_service_enable=false` 时 mock 替代。

## `frameworks/libs/distributeddb/`

KV 存储引擎 + 数据同步，底层 SQLite 操作核心。

关键子目录：
- `interfaces/include/`：公开头文件，`kv_store_nb_delegate.h`、`kv_store_delegate_manager.h` 等。
- `interfaces/src/kv_store_nb_delegate_impl.cpp`：接口实现，实际存储路径。
- `storage/src/sqlite/`：SQLite 存储实现。
- `syncer/src/`：数据同步实现（设备同步 + 云同步）。
- `communicator/src/`：通信层实现。
- `common/include/db_errno.h`：DB 层 errno 定义。
- `common/include/log_print.h`：distributeddb 日志宏。

测试：`test/`。

## `frameworks/common/`

共享工具库。

关键文件：
- `concurrent_map.h`：ConcurrentMap，Compute 语义：action 返回 false = 删除条目。
- `log_print.h`：innerkitsimpl 日志宏（ZLOGD/ZLOGI/ZLOGW/ZLOGE）与 LogLabel 定义。
- `task_scheduler.h`：任务调度。

测试：`test/`。

## `frameworks/cj/`

CJ FFI 实现。`interfaces/cj/` 仅为 BUILD.gn 打包层。

关键文件：
- `include/distributed_kv_store_ffi.h` / `src/distributed_kv_store_ffi.cpp`：FFI 边界。
- `include/distributed_kv_store_impl.h` / `src/distributed_kv_store_impl.cpp`：FFI 实现类。
- `src/distributed_kv_store_utils.cpp`：FFI 工具。

## `frameworks/ets/taihe/kv_store/`

ANI/Taihe 静态绑定。`distributedkvstore_ani_pack` GN target。

关键文件：
- `idl/ohos.data.distributedkvstore.taihe`：Taihe IDL 源。
- `src/ohos.data.distributedkvstore.impl.cpp`：ANI 实现胶水。
- `src/ani_*.cpp`：ANI 工具实现。

## `databaseutils/`

ACL 权限工具。

- `include/acl.h`。

测试：`test/`。

## `interfaces/innerkits/distributeddata/`

InnerKit 公开头文件。

- `include/store_errno.h`：Status 错误码。
- `include/single_kvstore.h`：SingleKvStore 接口。

## `interfaces/innerkits/distributeddatamgr/`

分布式数据管理 InnerKit。

- `include/distributed_data_mgr.h`。

## `test/`

- `unittest/`：应用层单元测试（distributeddata、distributedKVStore）。
- `fuzztest/`：fuzz 测试。
- `distributedtest/`：分布式跨设备集成测试。

## Where to look

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