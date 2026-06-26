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
import { hilog } from '@kit.PerformanceAnalysisKit';
import { describe, beforeAll, it, expect } from '@ohos/hypium';
import ffi from 'liblibrary.so';
import { dlopen, FFIType, CString, CFunction, AsyncCFunction, JSCallback, Struct } from '../../../main/ets/ffi';

const DOMAIN = 0x0000;
const LIB_NAME = 'libffi_target.so';
const LIB_NAME_TP = 'librvohostest.so';

export default function ffiTest() {
  describe('FFITest', () => {
    beforeAll(() => {
      hilog.info(DOMAIN, 'testTag', '%{public}s', 'FFITest');
    })

    /* ═══════════════════════════════════════
       Raw NAPI bridge tests (所有 7 个 NAPI 方法)
       ═══════════════════════════════════════ */

    it('load_returns_valid_handle', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(h).assertInstanceOf('BigInt');
      expect(h > 0).assertTrue();
      ffi.close(h);
    })

    it('close_multiple_calls_safe', 0, () => {
      let h = ffi.load(LIB_NAME);
      ffi.close(h);
      ffi.close(h);
    })

    it('callMixed_dd_add', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'add', 'dd', 'd', [2.0, 3.0], [])).assertEqual(5.0);
      ffi.close(h);
    })

    it('callMixed_dd_negative', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'multiply', 'dd', 'd', [-2.0, 3.0], [])).assertEqual(-6.0);
      ffi.close(h);
    })

    it('callMixed_dd_divide_by_zero', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'divide', 'dd', 'd', [5.0, 0.0], [])).assertEqual(0.0);
      ffi.close(h);
    })

    it('callMixed_ii_gcd', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'gcd', 'ii', 'i', [12, 18], [])).assertEqual(6);
      ffi.close(h);
    })

    it('callMixed_ii_gcd_coprime', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'gcd', 'ii', 'i', [7, 13], [])).assertEqual(1);
      ffi.close(h);
    })

    it('callMixed_i_factorial', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'factorial', 'i', 'l', [5], [])).assertEqual(120);
      ffi.close(h);
    })

    it('callMixed_i_factorial_zero', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'factorial', 'i', 'l', [0], [])).assertEqual(1);
      ffi.close(h);
    })

    it('callMixed_i_factorial_large', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'factorial', 'i', 'l', [10], [])).assertEqual(3628800);
      ffi.close(h);
    })

    it('callMixed_ss_equal', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'compareStrings', 'ss', 'i', [], ['abc', 'abc'])).assertEqual(0);
      ffi.close(h);
    })

    it('callMixed_ss_less', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'compareStrings', 'ss', 'i', [], ['abc', 'abd'])).assertLess(0);
      ffi.close(h);
    })

    it('callMixed_ids_compute_square', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'compute', 'ids', 'd', [0, 4.0], ['square'])).assertEqual(16.0);
      ffi.close(h);
    })

    it('callMixed_ids_compute_half', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'compute', 'ids', 'd', [1, 10.0], ['half'])).assertEqual(5.0);
      ffi.close(h);
    })

    it('callMixed_idid_weightedSum', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'weightedSum', 'idid', 'd', [3, 2.5, 4, 1.5], [])).assertEqual(13.5);
      ffi.close(h);
    })

    it('callMixed_dd_divide_normal', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callMixed(h, 'divide', 'dd', 'd', [10.0, 3.0], [])).assertLarger(3.0);
      ffi.close(h);
    })

    it('defineFunction_then_callBySig_dd', 0, () => {
      let h = ffi.load(LIB_NAME);
      ffi.defineFunction(h, 'add', 'dd', 'd');
      expect(ffi.callBySig(h, 'add', [2.0, 3.0], [])).assertEqual(5.0);
      ffi.close(h);
    })

    it('defineFunction_then_callBySig_ids', 0, () => {
      let h = ffi.load(LIB_NAME);
      ffi.defineFunction(h, 'compute', 'ids', 'd');
      expect(ffi.callBySig(h, 'compute', [0, 4.0], ['square'])).assertEqual(16.0);
      ffi.close(h);
    })

    it('defineFunction_then_callBySig_reuse', 0, () => {
      let h = ffi.load(LIB_NAME);
      ffi.defineFunction(h, 'multiply', 'dd', 'd');
      expect(ffi.callBySig(h, 'multiply', [2.0, 3.0], [])).assertEqual(6.0);
      expect(ffi.callBySig(h, 'multiply', [4.0, 5.0], [])).assertEqual(20.0);
      ffi.close(h);
    })

    it('callString_getVersion', 0, () => {
      let h = ffi.load(LIB_NAME);
      expect(ffi.callString(h, 'getVersion')).assertEqual('1.0.0');
      ffi.close(h);
    })

    it('readCString_null_ptr_returns_empty', 0, () => {
      expect(ffi.readCString(0)).assertEqual('');
    })

    it('call_unknown_function_throws', 0, () => {
      let h = ffi.load(LIB_NAME);
      let threw = false;
      try {
        ffi.callMixed(h, 'nonExistent', 'dd', 'd', [1.0], []);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertTrue();
      ffi.close(h);
    })

    it('callBySig_not_defined_throws', 0, () => {
      let h = ffi.load(LIB_NAME);
      let threw = false;
      try {
        ffi.callBySig(h, 'neverDefined', [1.0], []);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertTrue();
      ffi.close(h);
    })

    it('multiple_load_close_cycles', 0, () => {
      for (let i = 0; i < 3; i++) {
        let h = ffi.load(LIB_NAME);
        expect(h > 0).assertTrue();
        expect(ffi.callMixed(h, 'add', 'dd', 'd', [i, i], [])).assertEqual(i * 2);
        ffi.close(h);
      }
    })

    it('define_across_multiple_handles', 0, () => {
      let h1 = ffi.load(LIB_NAME);
      let h2 = ffi.load(LIB_NAME);
      ffi.defineFunction(h1, 'add', 'dd', 'd');
      ffi.defineFunction(h2, 'multiply', 'dd', 'd');
      expect(ffi.callBySig(h1, 'add', [1.0, 2.0], [])).assertEqual(3.0);
      expect(ffi.callBySig(h2, 'multiply', [3.0, 4.0], [])).assertEqual(12.0);
      ffi.close(h1);
      ffi.close(h2);
    })

    /* ═══════════════════════════════════════
       BunFFI.ts wrapper tests
       ═══════════════════════════════════════ */

    it('FFIType_constants', 0, () => {
      expect(FFIType.int32).assertEqual('i');
      expect(FFIType.int64).assertEqual('l');
      expect(FFIType.double).assertEqual('d');
      expect(FFIType.CString).assertEqual('s');
      expect(FFIType.char).assertEqual('c');
      expect(FFIType.i8).assertEqual('c');
      expect(FFIType.int8_t).assertEqual('c');
      expect(FFIType.int16_t).assertEqual('i');
      expect(FFIType.int).assertEqual('i');
      expect(FFIType.i32).assertEqual('i');
      expect(FFIType.i64).assertEqual('l');
      expect(FFIType.f64).assertEqual('d');
      expect(FFIType.float).assertEqual('f');
      expect(FFIType.f32).assertEqual('f');
      expect(FFIType.bool).assertEqual('b');
      expect(FFIType.ptr).assertEqual('p');
      expect(FFIType.pointer).assertEqual('p');
      expect(FFIType.u64).assertEqual('l');
      expect(FFIType.i64_fast).assertEqual('l');
      expect(FFIType.function).assertEqual('p');
      expect(FFIType.napi_env).assertEqual('p');
      expect(FFIType.buffer).assertEqual('p');
    })

    it('dlopen_returns_library', 0, () => {
      let lib = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.add(1.0, 2.0)).assertEqual(3.0);
      lib.close();
    })

    it('dlopen_symbols_call_via_ncall', 0, () => {
      let lib = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.add(2.0, 3.0)).assertEqual(5.0);
      lib.close();
    })

    it('dlopen_symbols_gcd', 0, () => {
      let lib = dlopen(LIB_NAME, {
        gcd: { args: [FFIType.int32, FFIType.int32], returns: FFIType.int32 },
      });
      expect(lib.symbols.gcd(12, 18)).assertEqual(6);
      lib.close();
    })

    it('dlopen_symbols_factorial', 0, () => {
      let lib = dlopen(LIB_NAME, {
        factorial: { args: [FFIType.int32], returns: FFIType.int64 },
      });
      expect(lib.symbols.factorial(10)).assertEqual(3628800);
      lib.close();
    })

    it('dlopen_symbols_compute_mixed', 0, () => {
      let lib = dlopen(LIB_NAME, {
        compute: { args: [FFIType.int32, FFIType.double, FFIType.CString], returns: FFIType.double },
      });
      expect(lib.symbols.compute(0, 4.0, 'square')).assertEqual(16.0);
      expect(lib.symbols.compute(1, 10.0, 'half')).assertEqual(5.0);
      lib.close();
    })

    it('dlopen_symbols_compareStrings', 0, () => {
      let lib = dlopen(LIB_NAME, {
        compareStrings: { args: [FFIType.CString, FFIType.CString], returns: FFIType.int32 },
      });
      expect(lib.symbols.compareStrings('abc', 'abc')).assertEqual(0);
      expect(lib.symbols.compareStrings('abc', 'abd')).assertLess(0);
      lib.close();
    })

    it('dlopen_symbols_weightedSum', 0, () => {
      let lib = dlopen(LIB_NAME, {
        weightedSum: { args: [FFIType.int32, FFIType.double, FFIType.int32, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.weightedSum(3, 2.5, 4, 1.5)).assertEqual(13.5);
      lib.close();
    })

    it('dlopen_symbols_divide_by_zero', 0, () => {
      let lib = dlopen(LIB_NAME, {
        divide: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.divide(5.0, 0.0)).assertEqual(0.0);
      lib.close();
    })

    it('dlopen_multiple_functions', 0, () => {
      let lib = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
        multiply: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
        divide: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.add(10.0, 5.0)).assertEqual(15.0);
      expect(lib.symbols.multiply(10.0, 5.0)).assertEqual(50.0);
      expect(lib.symbols.divide(10.0, 5.0)).assertEqual(2.0);
      lib.close();
    })

    it('dlopen_symbols_reuse', 0, () => {
      let lib = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib.symbols.add(1.0, 2.0)).assertEqual(3.0);
      expect(lib.symbols.add(3.0, 4.0)).assertEqual(7.0);
      expect(lib.symbols.add(5.0, 6.0)).assertEqual(11.0);
      lib.close();
    })

    it('dlopen_ffitype_properties', 0, () => {
      let lib = dlopen(LIB_NAME, {
        gcd: { args: [FFIType.int32, FFIType.int32], returns: FFIType.int32 },
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
        compute: { args: [FFIType.int32, FFIType.double, FFIType.CString], returns: FFIType.double },
      });
      expect(lib.symbols.gcd(12, 18)).assertEqual(6);
      expect(lib.symbols.add(2.0, 3.0)).assertEqual(5.0);
      expect(lib.symbols.compute(0, 4.0, 'square')).assertEqual(16.0);
      lib.close();
    })

    it('Library_close_called_multiple_times', 0, () => {
      let lib = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      lib.symbols.add(1.0, 2.0);
      lib.close();
      lib.close();
    })

    it('multiple_dlopen_instances', 0, () => {
      let lib1 = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      let lib2 = dlopen(LIB_NAME, {
        add: { args: [FFIType.double, FFIType.double], returns: FFIType.double },
      });
      expect(lib1.symbols.add(1.0, 2.0)).assertEqual(3.0);
      expect(lib2.symbols.add(3.0, 4.0)).assertEqual(7.0);
      lib1.close();
      lib2.close();
    })

    it('CString_null_ptr', 0, () => {
      let c: CString = new CString(0);
      expect(c.toString()).assertEqual('');
      expect(c.length).assertEqual(0);
    })

    it('CString_read', 0, () => {
      let lib = dlopen(LIB_NAME, {
        getVersion: { args: [], returns: FFIType.int64 },
      });
      let ptr: number = lib.symbols.getVersion();
      let cstr: CString = new CString(ptr);
      expect(cstr.toString()).assertEqual('1.0.0');
      expect(cstr.length).assertEqual(5);
      lib.close();
    })

    it('getSymbolPtr_returns_nonzero', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let ptr = ffi.getSymbolPtr(handle, 'add');
      expect(ptr > 0).assertTrue();
      ffi.close(handle);
    })

    it('getSymbolPtr_unknown_throws', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let threw = false;
      try {
        ffi.getSymbolPtr(handle, 'nonExistent');
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertTrue();
      ffi.close(handle);
    })

    it('CFunction_via_getSymbolPtr', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let ptr = ffi.getSymbolPtr(handle, 'add');
      let add = CFunction({ args: [FFIType.double, FFIType.double], returns: FFIType.double, ptr: ptr });
      expect(add(2.0, 3.0)).assertEqual(5.0);
      expect(add(10.0, 20.0)).assertEqual(30.0);
      add.close();
      ffi.close(handle);
    })

    it('CFunction_gcd', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let ptr = ffi.getSymbolPtr(handle, 'gcd');
      let gcd = CFunction({ args: [FFIType.int32, FFIType.int32], returns: FFIType.int32, ptr: ptr });
      expect(gcd(12, 18)).assertEqual(6);
      gcd.close();
      ffi.close(handle);
    })

    it('CFunction_compute_mixed', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let ptr = ffi.getSymbolPtr(handle, 'compute');
      let compute =
        CFunction({ args: [FFIType.int32, FFIType.double, FFIType.CString], returns: FFIType.double, ptr: ptr });
      expect(compute(0, 4.0, 'square')).assertEqual(16.0);
      compute.close();
      ffi.close(handle);
    })

    it('CFunction_weightedSum', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let ptr = ffi.getSymbolPtr(handle, 'weightedSum');
      let ws = CFunction(
        { args: [FFIType.int32, FFIType.double, FFIType.int32, FFIType.double], returns: FFIType.double, ptr: ptr });
      expect(ws(3, 2.5, 4, 1.5)).assertEqual(13.5);
      ws.close();
      ffi.close(handle);
    })

    it('JSCallback_create_and_ptr', 0, () => {
      let cb = new JSCallback((a: number, b: number): number => a + b, {
        args: [FFIType.int32, FFIType.int32],
        returns: FFIType.int32,
      });
      expect(cb.ptr > 0).assertTrue();
      cb.close();
    })

    it('JSCallback_call_direct', 0, () => {
      let cb = new JSCallback((a: number, b: number): number => a + b, {
        args: [FFIType.int32, FFIType.int32],
        returns: FFIType.int32,
      });
      expect(cb.call(2, 3)).assertEqual(5);
      cb.close();
    })

    it('JSCallback_closure_captures_state', 0, () => {
      let factor: number = 10;
      let cb = new JSCallback((x: number): number => x * factor, {
        args: [FFIType.int32],
        returns: FFIType.int32,
      });
      expect(cb.call(7)).assertEqual(70);
      cb.close();
    })

    it('JSCallback_close_then_call_throws', 0, () => {
      let cb = new JSCallback((x: number): number => x, {
        args: [FFIType.int32],
        returns: FFIType.int32,
      });
      cb.close();
      let threw = false;
      try {
        cb.call(1);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertTrue();
    })

    it('JSCallback_double_args', 0, () => {
      let cb = new JSCallback((a: number, b: number): number => a * b, {
        args: [FFIType.double, FFIType.double],
        returns: FFIType.double,
      });
      expect(cb.call(2.5, 4.0)).assertEqual(10.0);
      cb.close();
    })

    it('JSCallback_string_arg', 0, () => {
      let cb = new JSCallback((s: string): number => s.length, {
        args: [FFIType.CString],
        returns: FFIType.int32,
      });
      expect(cb.call('hello')).assertEqual(5);
      cb.close();
    })

    it('JSCallback_multiple_callbacks', 0, () => {
      let add = new JSCallback((a: number, b: number): number => a + b, {
        args: [FFIType.int32, FFIType.int32],
        returns: FFIType.int32,
      });
      let mul = new JSCallback((a: number, b: number): number => a * b, {
        args: [FFIType.int32, FFIType.int32],
        returns: FFIType.int32,
      });
      expect(add.call(3, 4)).assertEqual(7);
      expect(mul.call(3, 4)).assertEqual(12);
      add.close();
      mul.close();
    })

    it('JSCallback_threadsafe_property', 0, () => {
      let cb = new JSCallback((a: number, b: number): number => a + b, {
        args: [FFIType.int32, FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      expect(cb.threadsafe).assertTrue();
      expect(cb.call(2, 3)).assertEqual(5);
      cb.close();
    })

    it('JSCallback_non_threadsafe_default', 0, () => {
      let cb = new JSCallback((a: number): number => a, {
        args: [FFIType.int32],
        returns: FFIType.int32,
      });
      expect(cb.threadsafe).assertFalse();
      cb.close();
    })

    it('JSCallback_threadsafe_ptr_nonzero', 0, () => {
      let cb = new JSCallback((x: number): number => x * 2, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      expect(cb.ptr > 65536).assertTrue();
      cb.close();
    })

    it('JSCallback_threadsafe_invoke_after_create', 0, () => {
      let cb = new JSCallback((x: number): number => x + 1, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      expect(cb.call(41)).assertEqual(42);
      expect(cb.call(99)).assertEqual(100);
      cb.close();
    })

    it('JSCallback_threadsafe_multiple_calls', 0, () => {
      let sum: number = 0;
      let cb = new JSCallback((x: number): number => {
        sum += x;
        return sum;
      }, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      expect(cb.call(1)).assertEqual(1);
      expect(cb.call(2)).assertEqual(3);
      expect(cb.call(3)).assertEqual(6);
      cb.close();
    })

    it('JSCallback_threadsafe_via_napi_tsfn', 0, () => {
      let cb = new JSCallback((x: number): number => x * 10, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      ffi.callCallbackThreadSafe(cb.getHandle(), 7);
      cb.close();
    })

    it('JSCallback_non_threadsafe_cannot_use_tsfn', 0, () => {
      let cb = new JSCallback((x: number): number => x, {
        args: [FFIType.int32],
        returns: FFIType.int32,
      });
      let threw = false;
      try {
        ffi.callCallbackThreadSafe(cb.ptr, 1);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertTrue();
      cb.close();
    })

    it('JSCallback_threadsafe_ptr_is_real_address', 0, () => {
      let cb = new JSCallback((x: number): number => x * 3, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      let ptr: number = cb.ptr;
      expect(ptr > 65536).assertTrue();
      let fn = CFunction({ args: [FFIType.int32], returns: FFIType.int32, ptr: ptr });
      let result = fn(7);
      expect(result).assertEqual(21);
      fn.close();
      cb.close();
    })

    it('FFIType_callback_constant', 0, () => {
      expect(FFIType.callback).assertEqual('k');
      expect(FFIType.usize).assertEqual('l');
    })

    it('JSCallback_ptr_slot_non_threadsafe', 0, () => {
      let cb = new JSCallback((x: number): number => x, {
        args: [FFIType.int32],
        returns: FFIType.int32,
      });
      expect(cb.ptr).assertEqual(cb.getHandle());
      cb.close();
    })

    it('External Built Library', 0, () => {
      let lib1 = dlopen(LIB_NAME_TP, {
        hello: { args: [], returns: FFIType.int64 },
      });

      const ptr = lib1.symbols.hello();
      const cstr = new CString(ptr);
      expect(cstr.toString()).assertEqual("Hello, World!");
      lib1.close();
    })

    it('ptr_from_ArrayBuffer', 0, () => {
      let buf = new ArrayBuffer(256);
      let p = ffi.ptr(buf);
      expect(p > 0).assertTrue();
    })

    it('CString_with_byteOffset', 0, () => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'getVersion', '', 'l');
      let ptr: number = ffi.callBySig(handle, 'getVersion', [], []);
      let cstr = new CString(ptr, 0);
      expect(cstr.toString()).assertEqual('1.0.0');
      ffi.close(handle);
    })

    it('FFIType_additional_constants', 0, () => {
      expect(FFIType.uint8_t).assertEqual('i');
      expect(FFIType.u8).assertEqual('i');
      expect(FFIType.uint16_t).assertEqual('i');
      expect(FFIType.u16).assertEqual('i');
      expect(FFIType.uint32_t).assertEqual('i');
      expect(FFIType.u32).assertEqual('i');
      expect(FFIType.int64_t).assertEqual('l');
      expect(FFIType.uint64_t).assertEqual('l');
      expect(FFIType.int8_t).assertEqual('c');
      expect(FFIType.napi_value).assertEqual('p');
      expect(FFIType.callback).assertEqual('k');
      expect(FFIType.usize).assertEqual('l');
    })

    it('CFunction_callback_arg_via_apply_callback', 0, () => {
      let handle = ffi.load(LIB_NAME);
      let applyPtr = ffi.getSymbolPtr(handle, 'apply_callback');
      let cb = new JSCallback((x: number): number => x * 2, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      let fn = CFunction({
        args: [FFIType.callback, FFIType.int32],
        returns: FFIType.int32,
        ptr: applyPtr,
      });
      expect(fn(cb, 21)).assertEqual(42);
      fn.close();
      cb.close();
      ffi.close(handle);
    })

    it('dlopen_callback_arg_via_apply_callback', 0, () => {
      let lib = dlopen(LIB_NAME, {
        apply_callback: {
          args: [FFIType.callback, FFIType.int32],
          returns: FFIType.int32,
        },
      });
      let cb = new JSCallback((x: number): number => x * 2, {
        args: [FFIType.int32],
        returns: FFIType.int32,
        threadsafe: true,
      });
      expect(lib.symbols.apply_callback(cb, 21)).assertEqual(42);
      cb.close();
      lib.close();
    })

    it('callAsync_add', 0, () => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'add', 'dd', 'd');
      ffi.callAsync(handle, 'add', 'dd', 'd', [2.0, 3.0], []).then((r: number) => {
        expect(r).assertEqual(5.0);
        ffi.close(handle);
      });
    })

    it('callAsync_factorial', 0, () => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'factorial', 'i', 'l');
      ffi.callAsync(handle, 'factorial', 'i', 'l', [10], []).then((r: number) => {
        expect(r).assertEqual(3628800);
        ffi.close(handle);
      });
    })

    it('dlopen_and_callAsync', 0, (done: Function) => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'multiply', 'dd', 'd');
      ffi.callAsync(handle, 'multiply', 'dd', 'd', [3.0, 4.0], []).then((r: number) => {
        expect(r).assertEqual(12.0);
        ffi.close(handle);
        done();
      });
    })

    it('callAsync_concurrent', 0, (done: Function) => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'add', 'dd', 'd');
      let p1 = ffi.callAsync(handle, 'add', 'dd', 'd', [1.0, 2.0], []);
      let p2 = ffi.callAsync(handle, 'add', 'dd', 'd', [3.0, 4.0], []);
      let p3 = ffi.callAsync(handle, 'add', 'dd', 'd', [5.0, 6.0], []);
      Promise.all([p1, p2, p3]).then((results: number[]) => {
        expect(results[0]).assertEqual(3.0);
        expect(results[1]).assertEqual(7.0);
        expect(results[2]).assertEqual(11.0);
        ffi.close(handle);
        done();
      });
    })

    it('Struct_define_and_create', 0, () => {
      let Point = Struct({
        x: FFIType.int32,
        y: FFIType.int32,
      });
      expect(Point.size).assertEqual(8);
      let buf = Point.create({ x: 10, y: 20 });
      expect(Point.get(buf, 'x')).assertEqual(10);
      expect(Point.get(buf, 'y')).assertEqual(20);
    })

    it('Struct_get_set', 0, () => {
      let Point = Struct({
        x: FFIType.int32,
        y: FFIType.int32,
      });
      let buf = Point.create({ x: 0, y: 0 });
      Point.set(buf, 'x', 42);
      Point.set(buf, 'y', 99);
      expect(Point.get(buf, 'x')).assertEqual(42);
      expect(Point.get(buf, 'y')).assertEqual(99);
    })

    it('Struct_alignment_padding', 0, () => {
      let Mixed = Struct({
        a: FFIType.int8_t,
        b: FFIType.int32,
        c: FFIType.int8_t,
      });
      expect(Mixed.size).assertEqual(12);
      let buf = Mixed.create({ a: 1, b: 2, c: 3 });
      expect(Mixed.get(buf, 'a')).assertEqual(1);
      expect(Mixed.get(buf, 'b')).assertEqual(2);
      expect(Mixed.get(buf, 'c')).assertEqual(3);
    })

    it('Struct_fromPtr', 0, () => {
      let handle = ffi.load(LIB_NAME);
      ffi.defineFunction(handle, 'getVersion', '', 'l');
      let ptr: number = ffi.callBySig(handle, 'getVersion', [], []);
      let buf = ffi.readMemory(ptr, 8);
      let view = new DataView(buf);
      let firstByte = view.getUint8(0);
      expect(firstByte).assertEqual('1'.charCodeAt(0));
      ffi.close(handle);
    })

    it('Struct_pointer_passthrough', 0, () => {
      const Point = Struct({
        x: FFIType.int32,
        y: FFIType.int32,
      });
      let buf = Point.create({ x: 30, y: 40 });
      let p = ffi.ptr(buf);
      expect(p > 0).assertTrue();
      let read = Point.fromPtr(p);
      expect(read.x).assertEqual(30);
      expect(read.y).assertEqual(40);
    })

    it('AsyncCFunction_add', 0, (done: Function) => {
      let handle = ffi.load(LIB_NAME);
      let addPtr = ffi.getSymbolPtr(handle, 'add');
      let fn = AsyncCFunction({ args: [FFIType.double, FFIType.double], returns: FFIType.double, ptr: addPtr });
      fn(2.0, 3.0).then((r: number) => {
        expect(r).assertEqual(5.0);
        fn.close();
        ffi.close(handle);
        done();
      });
    })

    it('AsyncCFunction_factorial', 0, (done: Function) => {
      let handle = ffi.load(LIB_NAME);
      let factPtr = ffi.getSymbolPtr(handle, 'factorial');
      let fn = AsyncCFunction({ args: [FFIType.int32], returns: FFIType.int64, ptr: factPtr });
      fn(10).then((r: number) => {
        expect(r).assertEqual(3628800);
        fn.close();
        ffi.close(handle);
        done();
      });
    })

    it('AsyncCFunction_reuse', 0, (done: Function) => {
      let handle = ffi.load(LIB_NAME);
      let addPtr = ffi.getSymbolPtr(handle, 'add');
      let fn = AsyncCFunction({ args: [FFIType.double, FFIType.double], returns: FFIType.double, ptr: addPtr });
      Promise.all([fn(1.0, 2.0), fn(3.0, 4.0), fn(5.0, 6.0)]).then((results: number[]) => {
        expect(results[0]).assertEqual(3.0);
        expect(results[1]).assertEqual(7.0);
        expect(results[2]).assertEqual(11.0);
        fn.close();
        ffi.close(handle);
        done();
      });
    })
  })
}
