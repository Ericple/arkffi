# arkffi

![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/Ericple/arkffi/build.yml?branch=nightly&style=flat-square)
![GitHub commit activity](https://img.shields.io/github/commit-activity/y/Ericple/arkffi?style=flat-square)
![GitHub contributors](https://img.shields.io/github/contributors/Ericple/arkffi?style=flat-square)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/Ericple/arkffi?style=flat-square)
![GitHub License](https://img.shields.io/github/license/Ericple/arkffi?style=flat-square)

arkffi 是一个 HarmonyOS / ArkTS 的外部函数接口（FFI）库，支持从 ArkTS 直接调用 `.so` 共享库中的 C 函数。

[点此](https://docs.arkffi.hmbill.cn/)查看文档以快速开始！

## 特性

- **声明式 `dlopen`**：通过字典式定义加载 `.so` 并获取类型化函数符号
- **FFIType 类型系统**：支持 `int32`、`int64`、`double`、`float`、`bool`、`CString`、`pointer` 等 C 类型
- **混合参数类型**：`int` + `double` + `string` 可在一次调用中混合传递
- **CFunction**：将原始 C 函数指针包装为可调用的 JavaScript 函数
- **JSCallback**：将 JavaScript 函数包装为 C 回调，支持 `threadsafe` 线程安全模式
- **CString**：从原始指针安全读取 C 字符串
- **外部预构建库**：支持集成 CLion / CMake 独立编译的 `.so` 库
- **IDE 自动补全**：TypeScript 泛型推导，`lib.symbols.fnName()` 自动补全

## 快速开始

```typescript
import { dlopen, FFIType, CString } from 'library';

const lib = dlopen('libffi_target.so', {
  add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
  getVersion: { args: [], returns: FFIType.int64 },
});

lib.symbols.add(2.0, 3.0); // → 5.0

const ptr = lib.symbols.getVersion();
new CString(ptr).toString(); // → "1.0.0"

lib.close();
```

## 项目结构

```
arkffi/
├── library/                          # HAR 模块（核心库）
│   ├── Index.ets                     # 模块导出入口
│   ├── src/main/cpp/
│   │   ├── napi_init.cpp             # NAPI 桥接层（C++）
│   │   ├── CMakeLists.txt            # 原生构建配置
│   │   ├── third_party/              # 示例第三方 .so 源码
│   │   └── types/liblibrary/         # NAPI 类型声明
│   ├── src/main/ets/
│   │   └── ffi.ts                    # TypeScript 封装层
│   ├── libs/arm64-v8a/               # 外部预构建 .so 存放目录
│   ├── src/ohosTest/ets/test/
│   │   └── FFI.test.ets              # 测试用例（55+ 条）
│   ├── build-profile.json5
│   └── oh-package.json5
├── entry/                            # 示例入口应用
│   └── src/main/ets/
│       └── pages/Index.ets           # 使用示例
└── docs/                             # Mintlify 文档站
    ├── zh/                           # 简体中文（默认语系）
    ├── zh-Hant/                      # 繁体中文
    └── docs.json                     # 文档配置
```

## 安装

```bash
ohpm install arkffi
```

## API 一览

| API                                        | 说明                                                 |
|--------------------------------------------|----------------------------------------------------|
| `dlopen(path, defs)`                       | 加载共享库，返回带类型化符号的 `Library`                          |
| `Library.symbols.fn()`                     | 调用原生函数                                             |
| `Library.close()`                          | 释放库句柄                                              |
| `FFIType.*`                                | C 类型常量（int32 / int64 / double / float / CString 等） |
| `CString(ptr)`                             | 从原始指针读取 C 字符串                                      |
| `CFunction({args, returns, ptr})`          | 包装 C 函数指针为可调用函数                                    |
| `JSCallback(fn, def)`                      | 包装 JS 函数为 C 回调                                     |
| `ffi.load / close / callMixed / callBySig` | 原始 NAPI 桥接                                         |

## 文档

完整文档站位于 `docs/` 目录，使用 [Mintlify](https://mintlify.com) 构建：

```bash
cd docs && npx mint dev
```

支持简体中文（默认）和繁体中文。

## 运行测试

```bash
# 构建测试 HAP
hvigorw --mode module -p module=library@ohosTest genOnDeviceTestHap

# 安装到设备
hdc install library/build/default/outputs/ohosTest/library-ohosTest-signed.hap

# 运行
hdc shell aa test -b <bundleName> -m library_test -s unittest OpenHarmonyTestRunner
```

## 外部预构建库

项目支持集成外部独立的 CMake 项目编译的 `.so`：

1. 使用 HarmonyOS NDK 交叉编译出 `librvohostest.so`
2. 放入 `library/libs/arm64-v8a/`
3. 在 `CMakeLists.txt` 中以 `IMPORTED` 方式引入
4. 通过 `dlopen` 加载调用

详见 `docs/zh/guides/external-library.mdx`。

## 许可证

Apache-2.0
