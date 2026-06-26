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
    napi_threadsafe_function tsfn;
    napi_env env;
};

static const int MAX_CALLBACK_SLOTS = 16;
static CallbackSlot g_callbackSlots[MAX_CALLBACK_SLOTS];

static const int MAX_TRAMPOLINES = 16;
static void* g_trampolines[MAX_TRAMPOLINES];
static bool g_trampolinesReady = false;

static std::map<std::pair<uint64_t, std::string>, FuncEntry> g_registry;

static napi_env g_mainEnv = nullptr;

extern "C" void CallGeneric(void* func, uint64_t* intRegs, double* floatRegs,
                            int numInts, int numFloats, void* result, int retEncoding);

static int RetEncoding(char returnType) {
    switch (returnType) {
        case 'i': return 0;
        case 'l': case 'p': case 'k': case 's': return 1;
        case 'd': case 'f': return 2;
        case '2': return 3;
        default: return 2;
    }
}

static bool IsFloatType(char c) {
    return c == 'd' || c == 'f';
}

static void NormalizeSig(std::string& sig) {
    for (size_t i = 0; i < sig.size(); i++) {
        if (sig[i] == 'l' || sig[i] == 'b' || sig[i] == 'c' || sig[i] == 'p') {
            sig[i] = 'i';
        } else if (sig[i] == 'f') {
            sig[i] = 'd';
        }
    }
}

static void ExtractArgs(napi_env env, napi_value numArgs, napi_value strArgs,
                        const std::string& sig,
                        uint64_t* intRegs, int& numInts,
                        double* floatRegs, int& numFloats,
                        char** strBuf, int& ns)
{
    numInts = 0;
    numFloats = 0;
    ns = 0;
    uint32_t numLen = 0, strLen = 0;
    napi_get_array_length(env, numArgs, &numLen);
    napi_get_array_length(env, strArgs, &strLen);

    int numIdx = 0, strIdx = 0;
    for (size_t i = 0; i < sig.size(); i++) {
        if (IsFloatType(sig[i])) {
            napi_value e;
            napi_get_element(env, numArgs, numIdx++, &e);
            napi_get_value_double(env, e, &floatRegs[numFloats++]);
        } else if (sig[i] == 's') {
            napi_value e;
            napi_get_element(env, strArgs, strIdx++, &e);
            size_t sl = 0;
            napi_get_value_string_utf8(env, e, nullptr, 0, &sl);
            strBuf[ns] = new char[sl + 1];
            napi_get_value_string_utf8(env, e, strBuf[ns], sl + 1, &sl);
            intRegs[numInts++] = reinterpret_cast<uint64_t>(strBuf[ns]);
            ns++;
        } else {
            napi_value e;
            napi_get_element(env, numArgs, numIdx++, &e);
            int64_t val;
            napi_get_value_int64(env, e, &val);
            intRegs[numInts++] = static_cast<uint64_t>(val);
        }
    }
}

static napi_value ResultToNAPI(napi_env env, char returnType, uint8_t* resultBuf) {
    napi_value result;
    switch (returnType) {
        case 'i': {
            int32_t val;
            memcpy(&val, resultBuf, sizeof(val));
            napi_create_int32(env, val, &result);
            break;
        }
        case 'l':
        case 'p':
        case 'k':
        case 's': {
            int64_t val;
            memcpy(&val, resultBuf, sizeof(val));
            napi_create_int64(env, val, &result);
            break;
        }
        case 'd':
        case 'f': {
            double val;
            memcpy(&val, resultBuf, sizeof(val));
            napi_create_double(env, val, &result);
            break;
        }
        case '2': {
            void* data;
            napi_value buf;
            napi_create_arraybuffer(env, 16, &data, &buf);
            memcpy(data, resultBuf, 16);
            return buf;
        }
        default:
            napi_get_undefined(env, &result);
            break;
    }
    return result;
}

static int32_t TrampolineDispatcher(int slotIdx, int32_t arg)
{
    if (slotIdx < 0 || slotIdx >= MAX_CALLBACK_SLOTS || !g_callbackSlots[slotIdx].active) {
        return 0;
    }

    napi_env env = g_callbackSlots[slotIdx].env;
    if (env == nullptr) return 0;

    napi_value jsCb;
    napi_status status = napi_get_reference_value(env, g_callbackSlots[slotIdx].ref, &jsCb);
    if (status != napi_ok || jsCb == nullptr) return 0;

    napi_value jsArg;
    napi_create_int32(env, arg, &jsArg);

    napi_value resultVal;
    status = napi_call_function(env, nullptr, jsCb, 1, &jsArg, &resultVal);
    if (status != napi_ok) return 0;

    int32_t result = 0;
    napi_get_value_int32(env, resultVal, &result);
    return result;
}

#define DEFINE_TRAMPOLINE(n) \
    static int32_t Trampoline_##n(int32_t a) { return TrampolineDispatcher(n, a); }

DEFINE_TRAMPOLINE(0)
DEFINE_TRAMPOLINE(1)
DEFINE_TRAMPOLINE(2)
DEFINE_TRAMPOLINE(3)
DEFINE_TRAMPOLINE(4)
DEFINE_TRAMPOLINE(5)
DEFINE_TRAMPOLINE(6)
DEFINE_TRAMPOLINE(7)
DEFINE_TRAMPOLINE(8)
DEFINE_TRAMPOLINE(9)
DEFINE_TRAMPOLINE(10)
DEFINE_TRAMPOLINE(11)
DEFINE_TRAMPOLINE(12)
DEFINE_TRAMPOLINE(13)
DEFINE_TRAMPOLINE(14)
DEFINE_TRAMPOLINE(15)

static void InitTrampolines()
{
    if (g_trampolinesReady) return;
    void* tbl[] = {
        reinterpret_cast<void*>(Trampoline_0),
        reinterpret_cast<void*>(Trampoline_1),
        reinterpret_cast<void*>(Trampoline_2),
        reinterpret_cast<void*>(Trampoline_3),
        reinterpret_cast<void*>(Trampoline_4),
        reinterpret_cast<void*>(Trampoline_5),
        reinterpret_cast<void*>(Trampoline_6),
        reinterpret_cast<void*>(Trampoline_7),
        reinterpret_cast<void*>(Trampoline_8),
        reinterpret_cast<void*>(Trampoline_9),
        reinterpret_cast<void*>(Trampoline_10),
        reinterpret_cast<void*>(Trampoline_11),
        reinterpret_cast<void*>(Trampoline_12),
        reinterpret_cast<void*>(Trampoline_13),
        reinterpret_cast<void*>(Trampoline_14),
        reinterpret_cast<void*>(Trampoline_15),
    };
    for (int i = 0; i < MAX_TRAMPOLINES; i++) {
        g_trampolines[i] = tbl[i];
    }
    g_trampolinesReady = true;
}

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

    std::string sig(argTypes);
    NormalizeSig(sig);

    FuncEntry entry;
    entry.funcPtr = func;
    entry.argTypes = sig;
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
    uint64_t intRegs[8] = {0};
    double floatRegs[8] = {0.0};
    char* strBuf[4] = {nullptr};
    int numInts = 0, numFloats = 0, ns = 0;
    uint8_t resultBuf[16] = {0};

    ExtractArgs(env, numArgs, strArgs, entry.argTypes,
                intRegs, numInts, floatRegs, numFloats, strBuf, ns);

    int retEnc = RetEncoding(entry.returnType);
    CallGeneric(entry.funcPtr, intRegs, floatRegs, numInts, numFloats, resultBuf, retEnc);

    for (int k = 0; k < ns; k++) {
        delete[] strBuf[k];
    }

    return ResultToNAPI(env, entry.returnType, resultBuf);
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

    std::string sig(argTypes);
    if (sig == "pi") {
        sig = "ki";
    } else {
        NormalizeSig(sig);
    }

    FuncEntry entry;
    entry.funcPtr = func;
    entry.argTypes = sig;
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

    std::string sig(argTypes);
    NormalizeSig(sig);

    FuncEntry entry;
    entry.funcPtr = reinterpret_cast<void*>(ptrValue);
    entry.argTypes = sig;
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
    g_callbackSlots[slotIdx].tsfn = nullptr;
    g_callbackSlots[slotIdx].env = env;

    if (threadsafe) {
        InitTrampolines();
        napi_value asyncName;
        napi_create_string_utf8(env, "JSCallback", NAPI_AUTO_LENGTH, &asyncName);
        napi_create_threadsafe_function(
            env, args[0], nullptr, asyncName,
            0, 1, nullptr, nullptr, nullptr,
            [](napi_env tsfnEnv, napi_value jsCb, void* context, void* data) {
                napi_value arg;
                napi_create_int32(tsfnEnv, *static_cast<int32_t*>(data), &arg);
                napi_value result;
                napi_call_function(tsfnEnv, nullptr, jsCb, 1, &arg, &result);
                delete static_cast<int32_t*>(data);
            },
            &g_callbackSlots[slotIdx].tsfn);
    }

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
        if (g_callbackSlots[slotIdx].threadsafe && g_callbackSlots[slotIdx].tsfn != nullptr) {
            napi_release_threadsafe_function(g_callbackSlots[slotIdx].tsfn,
                                              napi_tsfn_release);
            g_callbackSlots[slotIdx].tsfn = nullptr;
        }
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

struct AsyncWorkData {
    napi_deferred deferred;
    void* funcPtr;
    std::string sig;
    char returnType;
    uint64_t intRegs[8];
    double floatRegs[8];
    int numInts;
    int numFloats;
    char* strBuf[4];
    int ns;
    uint8_t resultBuf[16];
    napi_async_work work;
    napi_threadsafe_function resolveTSFN;
};

static void AsyncExecuteCB(napi_env env, void* data)
{
    auto* w = static_cast<AsyncWorkData*>(data);
    int retEnc = RetEncoding(w->returnType);
    CallGeneric(w->funcPtr, w->intRegs, w->floatRegs,
                w->numInts, w->numFloats, w->resultBuf, retEnc);
}

static void AsyncResolveCB(napi_env env, napi_value jsCb, void* context, void* data)
{
    auto* w = static_cast<AsyncWorkData*>(data);
    napi_value result = ResultToNAPI(env, w->returnType, w->resultBuf);
    napi_resolve_deferred(env, w->deferred, result);
    for (int i = 0; i < w->ns; i++) delete[] w->strBuf[i];
    if (w->resolveTSFN != nullptr) {
        napi_release_threadsafe_function(w->resolveTSFN, napi_tsfn_release);
    }
    napi_async_work work = w->work;
    delete w;
    napi_delete_async_work(env, work);
}

static void AsyncCompleteCB(napi_env env, napi_status status, void* data)
{
    auto* w = static_cast<AsyncWorkData*>(data);
    if (w->resolveTSFN != nullptr) {
        napi_call_threadsafe_function(w->resolveTSFN, w, napi_tsfn_blocking);
    }
}

static napi_value CallAsync(napi_env env, napi_callback_info info)
{
    size_t argc = 6;
    napi_value args[6] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    bool lossless = false;
    uint64_t handleValue = 0;
    napi_get_value_bigint_uint64(env, args[0], &handleValue, &lossless);

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

    std::pair<uint64_t, std::string> key(handleValue, funcName);
    auto it = g_registry.find(key);
    if (it == g_registry.end()) {
        std::string msg = "Function '";
        msg += funcName;
        msg += "' not defined. Call defineFunction first.";
        napi_throw_error(env, nullptr, msg.c_str());
        delete[] funcName; delete[] argTypes; delete[] retType;
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    delete[] funcName;

    auto* w = new AsyncWorkData();
    w->funcPtr = it->second.funcPtr;
    w->sig = argTypes;
    NormalizeSig(w->sig);
    w->returnType = retType[0];
    w->ns = 0;

    memset(w->intRegs, 0, sizeof(w->intRegs));
    memset(w->floatRegs, 0, sizeof(w->floatRegs));
    memset(w->strBuf, 0, sizeof(w->strBuf));

    ExtractArgs(env, args[4], args[5], w->sig,
                w->intRegs, w->numInts, w->floatRegs, w->numFloats, w->strBuf, w->ns);

    napi_value resourceName;
    napi_create_string_utf8(env, "CallAsync", NAPI_AUTO_LENGTH, &resourceName);

    napi_value promise;
    napi_create_promise(env, &w->deferred, &promise);

    w->resolveTSFN = nullptr;
    napi_value tsfnName;
    napi_create_string_utf8(env, "AsyncResolve", NAPI_AUTO_LENGTH, &tsfnName);
    napi_create_threadsafe_function(env, nullptr, nullptr, tsfnName,
        0, 1, nullptr, nullptr, nullptr,
        AsyncResolveCB, &w->resolveTSFN);

    napi_async_work work;
    napi_create_async_work(env, nullptr, resourceName,
        AsyncExecuteCB, AsyncCompleteCB, w, &work);
    napi_queue_async_work(env, work);

    delete[] argTypes;
    delete[] retType;

    return promise;
}

static napi_value CallPtrAsync(napi_env env, napi_callback_info info)
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

    std::string sig(argTypes);
    NormalizeSig(sig);

    auto* w = new AsyncWorkData();
    w->funcPtr = reinterpret_cast<void*>(ptrValue);
    w->sig = sig;
    w->returnType = retType[0];
    w->ns = 0;

    memset(w->intRegs, 0, sizeof(w->intRegs));
    memset(w->floatRegs, 0, sizeof(w->floatRegs));
    memset(w->strBuf, 0, sizeof(w->strBuf));

    ExtractArgs(env, args[3], args[4], w->sig,
                w->intRegs, w->numInts, w->floatRegs, w->numFloats, w->strBuf, w->ns);

    napi_value resourceName;
    napi_create_string_utf8(env, "CallPtrAsync", NAPI_AUTO_LENGTH, &resourceName);

    napi_value promise;
    napi_create_promise(env, &w->deferred, &promise);

    w->resolveTSFN = nullptr;
    napi_value tsfnName;
    napi_create_string_utf8(env, "AsyncResolve", NAPI_AUTO_LENGTH, &tsfnName);
    napi_create_threadsafe_function(env, nullptr, nullptr, tsfnName,
        0, 1, nullptr, nullptr, nullptr,
        AsyncResolveCB, &w->resolveTSFN);

    napi_async_work work;
    napi_create_async_work(env, nullptr, resourceName,
        AsyncExecuteCB, AsyncCompleteCB, w, &work);
    napi_queue_async_work(env, work);

    delete[] argTypes;
    delete[] retType;

    return promise;
}

static napi_value PtrFromTypedArray(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    bool isTypedArray = false;
    napi_is_typedarray(env, args[0], &isTypedArray);

    void* data = nullptr;
    size_t byteLength = 0;
    napi_value buffer;
    size_t byteOffset = 0;

    if (isTypedArray) {
        napi_get_typedarray_info(env, args[0], nullptr, &byteLength, &data, &buffer, &byteOffset);
    } else {
        napi_get_arraybuffer_info(env, args[0], &data, &byteLength);
        byteOffset = 0;
    }

    uint64_t ptrValue = reinterpret_cast<uint64_t>(static_cast<uint8_t*>(data) + byteOffset);

    napi_value result;
    napi_create_double(env, static_cast<double>(ptrValue), &result);
    return result;
}

static napi_value ReadMemory(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int64_t ptrValue = 0;
    napi_get_value_int64(env, args[0], &ptrValue);

    int32_t byteLength = 0;
    napi_get_value_int32(env, args[1], &byteLength);

    void* src = reinterpret_cast<void*>(ptrValue);

    napi_value buffer;
    void* data = nullptr;
    napi_create_arraybuffer(env, byteLength, &data, &buffer);
    memcpy(data, src, byteLength);

    return buffer;
}

static napi_value GetCallbackPtr(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int64_t handleValue = 0;
    napi_get_value_int64(env, args[0], &handleValue);
    int slotIdx = static_cast<int>(handleValue) - 1;

    double ptrValue;
    if (slotIdx >= 0 && slotIdx < MAX_CALLBACK_SLOTS && g_callbackSlots[slotIdx].active) {
        if (g_callbackSlots[slotIdx].threadsafe) {
            ptrValue = static_cast<double>(reinterpret_cast<uint64_t>(g_trampolines[slotIdx]));
        } else {
            ptrValue = static_cast<double>(slotIdx + 1);
        }
    } else {
        ptrValue = 0.0;
    }

    napi_value result;
    napi_create_double(env, ptrValue, &result);
    return result;
}

static napi_value CallCallbackThreadSafe(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
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

    if (!g_callbackSlots[slotIdx].threadsafe || g_callbackSlots[slotIdx].tsfn == nullptr) {
        napi_throw_error(env, nullptr, "callback is not threadsafe or TSFN is null");
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    }

    napi_value cbArg = args[1];
    napi_status status = napi_call_threadsafe_function(
        g_callbackSlots[slotIdx].tsfn,
        &cbArg,
        napi_tsfn_blocking);

    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "napi_call_threadsafe_function failed");
    }

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    g_mainEnv = env;
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
        {"getCallbackPtr", nullptr, GetCallbackPtr, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"ptr", nullptr, PtrFromTypedArray, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"readMemory", nullptr, ReadMemory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callCallbackThreadSafe", nullptr, CallCallbackThreadSafe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callAsync", nullptr, CallAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"callPtrAsync", nullptr, CallPtrAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
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
