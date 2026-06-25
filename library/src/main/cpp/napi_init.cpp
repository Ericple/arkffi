/*   Copyright [2026] [Guo Tingjin dev@peercat.cn]
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http:*www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/
#include <dlfcn.h>
#include <cstring>
#include <string>
#include <map>
#include "napi/native_api.h"

struct FuncEntry {
    void* funcPtr;
    std::string argTypes;
    char returnType;
};

struct CallbackSlot {
    napi_ref ref;
    bool active;
    bool threadsafe;
};

static const int MAX_CALLBACK_SLOTS = 16;
static CallbackSlot g_callbackSlots[MAX_CALLBACK_SLOTS];

static std::map<std::pair<uint64_t, std::string>, FuncEntry> g_registry;

static napi_value Load(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    size_t bufSize = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &bufSize);
    char* libName = new char[bufSize + 1];
    napi_get_value_string_utf8(env, args[0], libName, bufSize + 1, &bufSize);

    void* handle = dlopen(libName, RTLD_LAZY);
    delete[] libName;

    if (!handle) {
        napi_throw_error(env, nullptr, dlerror());
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    napi_value result;
    napi_create_bigint_uint64(env, reinterpret_cast<uint64_t>(handle), &result);
    return result;
}

static napi_value Close(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);

    void* handle = reinterpret_cast<void*>(handleValue);
    if (handle) {
        g_registry.erase(std::pair<uint64_t, std::string>(handleValue, ""));
        dlclose(handle);
    }

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value DefineFunction(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value args[4] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);
    void* handle = reinterpret_cast<void*>(handleValue);

    size_t bufSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &bufSize);
    char* funcName = new char[bufSize + 1];
    napi_get_value_string_utf8(env, args[1], funcName, bufSize + 1, &bufSize);

    size_t typeSize = 0;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &typeSize);
    char* argTypes = new char[typeSize + 1];
    napi_get_value_string_utf8(env, args[2], argTypes, typeSize + 1, &typeSize);

    size_t retSize = 0;
    napi_get_value_string_utf8(env, args[3], nullptr, 0, &retSize);
    char* retType = new char[retSize + 1];
    napi_get_value_string_utf8(env, args[3], retType, retSize + 1, &retSize);

    void* func = dlsym(handle, funcName);
    if (!func) {
        std::string msg = "dlsym failed: ";
        msg += dlerror();
        napi_throw_error(env, nullptr, msg.c_str());
        delete[] funcName; delete[] argTypes; delete[] retType;
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    FuncEntry entry;
    entry.funcPtr = func;
    entry.argTypes = argTypes;
    entry.returnType = retType[0];

    g_registry[std::pair<uint64_t, std::string>(handleValue, funcName)] = entry;

    delete[] funcName; delete[] argTypes; delete[] retType;

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value DispatchCallFromArrays(napi_env env,
                                          napi_value numArgs,
                                          napi_value strArgs,
                                          const FuncEntry& entry)
{
    uint32_t numLen = 0, strLen = 0;
    napi_get_array_length(env, numArgs, &numLen);
    napi_get_array_length(env, strArgs, &strLen);

    std::string sig = entry.argTypes;
    void* func = entry.funcPtr;
    char ret = entry.returnType;

    for (size_t k = 0; k < sig.size(); k++) {
        if (sig[k] == 'l' || sig[k] == 'b' || sig[k] == 'c' || sig[k] == 'p') {
            sig[k] = 'i';
        } else if (sig[k] == 'f') {
            sig[k] = 'd';
        }
    }

    int intBuf[4] = {0};
    int64_t int64Buf[4] = {0};
    double dblBuf[4] = {0.0};
    char* strBuf[4] = {nullptr};
    int ni = 0, nd = 0, ns = 0;
    int numIdx = 0, strIdx = 0;

    for (size_t k = 0; k < sig.size(); k++) {
        napi_value e;
        if (sig[k] == 'i' || sig[k] == 'l' || sig[k] == 'b' || sig[k] == 'c' || sig[k] == 'p') {
            napi_get_element(env, numArgs, numIdx, &e);
            int32_t val;
            napi_get_value_int32(env, e, &val);
            if (sig[k] == 'l' || sig[k] == 'p') {
                int64Buf[ni] = static_cast<int64_t>(val);
            } else {
                intBuf[ni] = val;
            }
            ni++;
            numIdx++;
        } else if (sig[k] == 'd' || sig[k] == 'f') {
            napi_get_element(env, numArgs, numIdx, &e);
            napi_get_value_double(env, e, &dblBuf[nd]);
            nd++;
            numIdx++;
        } else if (sig[k] == 's') {
            size_t sl = 0;
            napi_get_element(env, strArgs, strIdx, &e);
            napi_get_value_string_utf8(env, e, nullptr, 0, &sl);
            strBuf[ns] = new char[sl + 1];
            napi_get_value_string_utf8(env, e, strBuf[ns], sl + 1, &sl);
            ns++;
            strIdx++;
        }
    }

    napi_value result;
    napi_get_undefined(env, &result);

    if (sig == "dd") {
        typedef double (*F)(double, double);
        double r = reinterpret_cast<F>(func)(dblBuf[0], dblBuf[1]);
        napi_create_double(env, r, &result);
    } else if (sig == "d") {
        typedef double (*F)(double);
        double r = reinterpret_cast<F>(func)(dblBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "") {
        if (ret == 'i') {
            typedef int32_t (*F)();
            int32_t r = reinterpret_cast<F>(func)();
            napi_create_int32(env, r, &result);
        } else if (ret == 'l') {
            typedef int64_t (*F)();
            int64_t r = reinterpret_cast<F>(func)();
            napi_create_int64(env, r, &result);
        } else {
            typedef double (*F)();
            double r = reinterpret_cast<F>(func)();
            napi_create_double(env, r, &result);
        }
    } else if (sig == "ii") {
        if (ret == 'i') {
            typedef int32_t (*F)(int32_t, int32_t);
            int32_t r = reinterpret_cast<F>(func)(intBuf[0], intBuf[1]);
            napi_create_int32(env, r, &result);
        } else if (ret == 'l') {
            typedef int64_t (*F)(int32_t, int32_t);
            int64_t r = reinterpret_cast<F>(func)(intBuf[0], intBuf[1]);
            napi_create_int64(env, r, &result);
        } else {
            typedef double (*F)(int32_t, int32_t);
            double r = reinterpret_cast<F>(func)(intBuf[0], intBuf[1]);
            napi_create_double(env, r, &result);
        }
    } else if (sig == "i") {
        if (ret == 'l') {
            typedef int64_t (*F)(int32_t);
            int64_t r = reinterpret_cast<F>(func)(intBuf[0]);
            napi_create_int64(env, r, &result);
        } else if (ret == 'i') {
            typedef int32_t (*F)(int32_t);
            int32_t r = reinterpret_cast<F>(func)(intBuf[0]);
            napi_create_int32(env, r, &result);
        } else {
            typedef double (*F)(int32_t);
            double r = reinterpret_cast<F>(func)(intBuf[0]);
            napi_create_double(env, r, &result);
        }
    } else if (sig == "id") {
        typedef double (*F)(int32_t, double);
        double r = reinterpret_cast<F>(func)(intBuf[0], dblBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "di") {
        typedef double (*F)(double, int32_t);
        double r = reinterpret_cast<F>(func)(dblBuf[0], intBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "ids") {
        typedef double (*F)(int32_t, double, const char*);
        double r = reinterpret_cast<F>(func)(intBuf[0], dblBuf[0], strBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "sid") {
        typedef double (*F)(const char*, int32_t, double);
        double r = reinterpret_cast<F>(func)(strBuf[0], intBuf[0], dblBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "isd") {
        typedef double (*F)(int32_t, const char*, double);
        double r = reinterpret_cast<F>(func)(intBuf[0], strBuf[0], dblBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "iid") {
        typedef double (*F)(int32_t, int32_t, double);
        double r = reinterpret_cast<F>(func)(intBuf[0], intBuf[1], dblBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "idi") {
        typedef double (*F)(int32_t, double, int32_t);
        double r = reinterpret_cast<F>(func)(intBuf[0], dblBuf[0], intBuf[1]);
        napi_create_double(env, r, &result);
    } else if (sig == "s") {
        typedef double (*F)(const char*);
        double r = reinterpret_cast<F>(func)(strBuf[0]);
        napi_create_double(env, r, &result);
    } else if (sig == "ss") {
        if (ret == 'i') {
            typedef int32_t (*F)(const char*, const char*);
            int32_t r = reinterpret_cast<F>(func)(strBuf[0], strBuf[1]);
            napi_create_int32(env, r, &result);
        } else {
            typedef double (*F)(const char*, const char*);
            double r = reinterpret_cast<F>(func)(strBuf[0], strBuf[1]);
            napi_create_double(env, r, &result);
        }
    } else if (sig == "idid") {
        typedef double (*F)(int32_t, double, int32_t, double);
        double r = reinterpret_cast<F>(func)(intBuf[0], dblBuf[0], intBuf[1], dblBuf[1]);
        napi_create_double(env, r, &result);
    } else {
        std::string msg = "Unsupported signature: '";
        msg += sig;
        msg += "'. Add it to DispatchCallFromArrays.";
        napi_throw_error(env, nullptr, msg.c_str());
    }

    for (int k = 0; k < ns; k++) {
        delete[] strBuf[k];
    }

    return result;
}

static napi_value CallBySig(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value args[4] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);

    size_t bufSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &bufSize);
    char* funcName = new char[bufSize + 1];
    napi_get_value_string_utf8(env, args[1], funcName, bufSize + 1, &bufSize);

    std::pair<uint64_t, std::string> key(handleValue, funcName);
    auto it = g_registry.find(key);
    if (it == g_registry.end()) {
        std::string msg = "Function '";
        msg += funcName;
        msg += "' not defined. Call defineFunction first.";
        napi_throw_error(env, nullptr, msg.c_str());
        delete[] funcName;
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    delete[] funcName;
    return DispatchCallFromArrays(env, args[2], args[3], it->second);
}

static napi_value CallMixed(napi_env env, napi_callback_info info)
{
    size_t argc = 6;
    napi_value args[6] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);
    void* handle = reinterpret_cast<void*>(handleValue);

    size_t fnSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &fnSize);
    char* funcName = new char[fnSize + 1];
    napi_get_value_string_utf8(env, args[1], funcName, fnSize + 1, &fnSize);

    size_t typeSize = 0;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &typeSize);
    char* argTypes = new char[typeSize + 1];
    napi_get_value_string_utf8(env, args[2], argTypes, typeSize + 1, &typeSize);

    size_t retSize = 0;
    napi_get_value_string_utf8(env, args[3], nullptr, 0, &retSize);
    char* retType = new char[retSize + 1];
    napi_get_value_string_utf8(env, args[3], retType, retSize + 1, &retSize);

    void* func = dlsym(handle, funcName);
    if (!func) {
        std::string msg = "dlsym failed: ";
        msg += dlerror();
        napi_throw_error(env, nullptr, msg.c_str());
        delete[] funcName; delete[] argTypes; delete[] retType;
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    FuncEntry entry;
    entry.funcPtr = func;
    entry.argTypes = argTypes;
    entry.returnType = retType[0];

    delete[] funcName; delete[] argTypes; delete[] retType;

    return DispatchCallFromArrays(env, args[4], args[5], entry);
}

static napi_value CallString(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);
    void* handle = reinterpret_cast<void*>(handleValue);

    size_t bufSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &bufSize);
    char* funcName = new char[bufSize + 1];
    napi_get_value_string_utf8(env, args[1], funcName, bufSize + 1, &bufSize);

    typedef const char* (*StringFunc)();
    void* func = dlsym(handle, funcName);
    delete[] funcName;

    if (!func) {
        napi_throw_error(env, nullptr, dlerror());
        napi_value result;
        napi_create_string_utf8(env, "", 0, &result);
        return result;
    }

    const char* str = reinterpret_cast<StringFunc>(func)();

    napi_value result;
    napi_create_string_utf8(env, str, strlen(str), &result);
    return result;
}

static napi_value ReadCString(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    int64_t ptrValue = 0;
    napi_get_value_int64(env, args[0], &ptrValue);

    const char* str = reinterpret_cast<const char*>(ptrValue);
    if (str == nullptr) {
        napi_value result;
        napi_create_string_utf8(env, "", 0, &result);
        return result;
    }

    napi_value result;
    napi_create_string_utf8(env, str, strlen(str), &result);
    return result;
}

static napi_value GetSymbolPtr(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);
    void* handle = reinterpret_cast<void*>(handleValue);

    size_t bufSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &bufSize);
    char* funcName = new char[bufSize + 1];
    napi_get_value_string_utf8(env, args[1], funcName, bufSize + 1, &bufSize);

    void* func = dlsym(handle, funcName);
    delete[] funcName;

    if (!func) {
        napi_throw_error(env, nullptr, dlerror());
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    napi_value result;
    napi_create_double(env, static_cast<double>(reinterpret_cast<uint64_t>(func)), &result);
    return result;
}

static napi_value CallPtr(napi_env env, napi_callback_info info)
{
    size_t argc = 5;
    napi_value args[5] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    int64_t ptrValue = 0;
    napi_get_value_int64(env, args[0], &ptrValue);

    size_t typeSize = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &typeSize);
    char* argTypes = new char[typeSize + 1];
    napi_get_value_string_utf8(env, args[1], argTypes, typeSize + 1, &typeSize);

    size_t retSize = 0;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &retSize);
    char* retType = new char[retSize + 1];
    napi_get_value_string_utf8(env, args[2], retType, retSize + 1, &retSize);

    FuncEntry entry;
    entry.funcPtr = reinterpret_cast<void*>(ptrValue);
    entry.argTypes = argTypes;
    entry.returnType = retType[0];

    napi_value result = DispatchCallFromArrays(env, args[3], args[4], entry);

    delete[] argTypes;
    delete[] retType;

    return result;
}

static napi_value CreateCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value args[4] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);
    if (valuetype != napi_function) {
        napi_throw_error(env, nullptr, "first argument must be a function");
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    bool threadsafe = false;
    if (argc >= 4) {
        napi_get_value_bool(env, args[3], &threadsafe);
    }

    int slotIdx = -1;
    for (int i = 0; i < MAX_CALLBACK_SLOTS; i++) {
        if (!g_callbackSlots[i].active) {
            slotIdx = i;
            break;
        }
    }
    if (slotIdx < 0) {
        napi_throw_error(env, nullptr, "no available callback slots");
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    napi_ref cbRef = nullptr;
    napi_create_reference(env, args[0], 1, &cbRef);
    g_callbackSlots[slotIdx].ref = cbRef;
    g_callbackSlots[slotIdx].active = true;
    g_callbackSlots[slotIdx].threadsafe = threadsafe;

    napi_value result;
    napi_create_double(env, static_cast<double>(slotIdx + 1), &result);
    return result;
}

static napi_value DestroyCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    int64_t handleValue = 0;
    napi_get_value_int64(env, args[0], &handleValue);

    int slotIdx = static_cast<int>(handleValue) - 1;
    if (slotIdx >= 0 && slotIdx < MAX_CALLBACK_SLOTS && g_callbackSlots[slotIdx].active) {
        napi_delete_reference(env, g_callbackSlots[slotIdx].ref);
        g_callbackSlots[slotIdx].ref = nullptr;
        g_callbackSlots[slotIdx].active = false;
    }

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value InvokeCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 5;
    napi_value args[5] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);


    int64_t handleValue = 0;
    napi_get_value_int64(env, args[0], &handleValue);
    int slotIdx = static_cast<int>(handleValue) - 1;
    if (slotIdx < 0 || slotIdx >= MAX_CALLBACK_SLOTS || !g_callbackSlots[slotIdx].active) {
        napi_throw_error(env, nullptr, "invalid or closed callback handle");
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    napi_value callbackFn;
    napi_get_reference_value(env, g_callbackSlots[slotIdx].ref, &callbackFn);

    napi_value global;
    napi_get_global(env, &global);

    size_t cbArgc = argc - 1;
    napi_value* cbArgs = cbArgc > 0 ? &args[1] : nullptr;

    napi_value result;
    napi_call_function(env, global, callbackFn, cbArgc, cbArgs, &result);
    return result;
}

static napi_value GetCallbackThreadsafe(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int64_t handleValue = 0;
    napi_get_value_int64(env, args[0], &handleValue);
    int slotIdx = static_cast<int>(handleValue) - 1;

    bool threadsafe = false;
    if (slotIdx >= 0 && slotIdx < MAX_CALLBACK_SLOTS && g_callbackSlots[slotIdx].active) {
        threadsafe = g_callbackSlots[slotIdx].threadsafe;
    }

    napi_value result;
    napi_get_boolean(env, threadsafe, &result);
    return result;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"load", nullptr, Load, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"close", nullptr, Close, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"defineFunction", nullptr, DefineFunction, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callBySig", nullptr, CallBySig, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callMixed", nullptr, CallMixed, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callString", nullptr, CallString, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"readCString", nullptr, ReadCString, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callPtr", nullptr, CallPtr, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getSymbolPtr", nullptr, GetSymbolPtr, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createCallback", nullptr, CreateCallback, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyCallback", nullptr, DestroyCallback, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"invokeCallback", nullptr, InvokeCallback, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getCallbackThreadsafe", nullptr, GetCallbackThreadsafe, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "library",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterLibraryModule(void)
{
    napi_module_register(&demoModule);
}
