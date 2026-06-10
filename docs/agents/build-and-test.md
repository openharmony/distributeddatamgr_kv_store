# 构建与测试命令

选择最小验证方式时读取本页。

## 环境边界

本仓库是 OpenHarmony 组件。完整编译和运行验证需要 OpenHarmony 源码树，并且本仓库位于：

```text
foundation/distributeddatamgr/kv_store
```

## 最小文本验证

在本仓库根目录运行：

```powershell
git status --short
git diff -- AGENTS.md docs/agents
rg -n "docs/agents/" AGENTS.md
```

代码改动还应检查是否误用 public 日志打印隐私数据：

```powershell
rg -n "ZLOG[IWE].*%{public}.*(udid|uuid|ip|mac|path|passwrod|pwd)" frameworks interfaces
```

## 测试与构建目标

| 目标 | 用途          | 命令 |
|---|-------------|---|
| `kv_store` | 构建全量部件镜像    | `./build.sh --product-name <product> --build-target kv_store` |
| `kv_store_test` | 构建部件镜像和测试用例 | `./build.sh --product-name <product> --build-target kv_store_test` |

如果工作区使用 `hb`，等价模式是：

```bash
| 目标 | 用途 | 命令 |
|---|---|---|
| `kv_store` | 独立编译镜像 | `hb build kv_store` |
| `kv_store_test` | 独立编译镜像和测试用例 | `hb build kv_store -t ` |

```

## 验证报告要求

报告验证时要明确：

- 运行了哪些命令。
- 如果无法运行完整构建，明确说明问题原因，并给出解决方案，但是不要直接进行问题解决。

环境变量：`<product>` — 传给 `./build.sh --product-name` 的产品占位符（示例 `rk3568`）。