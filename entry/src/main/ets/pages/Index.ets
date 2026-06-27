import { ffi, JSCallback, CFunction, FFIType, AsyncCFunction, Struct, dlopen } from 'arkffi';

const LIB_NAME = 'libffi_target.so';
const LIB_NAME_FFT = 'libfft.so';

@Builder
function test(a: number, b: string) {
}

let k = wrapBuilder(test);

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Column() {
      Button('apply callback')
        .onClick(() => {
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
          console.log('result =', fn(cb, 21));
          fn.close();
          cb.close();
          ffi.close(handle);
        })
      Button('async')
        .onClick(() => {
          let handle = ffi.load(LIB_NAME);
          let addPtr = ffi.getSymbolPtr(handle, 'add');
          let fn = AsyncCFunction({ args: [FFIType.double, FFIType.double], returns: FFIType.double, ptr: addPtr });
          fn(2.0, 3.0).then((r: number) => {
            console.log('r =', r);
            fn.close();
            ffi.close(handle);
          });
        })
      Button('Complex')
        .onClick(() => {
          const Complex = Struct({
            real: FFIType.double,
            imag: FFIType.double,
          });
          const LibFFT = dlopen(LIB_NAME_FFT, {
            complex_new: { args: [FFIType.double, FFIType.double], returns: Complex },
            complex_add: { args: [Complex, Complex], returns: Complex },
            complex_sub: { args: [Complex, Complex], returns: Complex },
            complex_mul: { args: [Complex, Complex], returns: Complex },
            fft: { args: [FFIType.ptr, FFIType.int32], returns: FFIType.void },
            hann_window: { args: [FFIType.ptr, FFIType.uint32_t], returns: FFIType.void },
            hamming_window: { args: [FFIType.ptr, FFIType.uint32_t], returns: FFIType.void }
          });

          const N = 12
          const ab = new ArrayBuffer(4 * N);
          const f32a = new Float32Array(ab).fill(0.5);
          const ptr = ffi.ptr(ab);
          console.log('before u8', f32a.toString());
          LibFFT.symbols.hann_window(ptr, N);
          console.log('after u8', f32a.toString())

          let newPtr: number = LibFFT.symbols.complex_new(3.0, 2.0);
          let newVal = Complex.fromPtr(newPtr);
          console.log('complex_new:', newVal.real, newVal.imag);

          let a = Complex.create({ real: 4, imag: 1 });
          let b = Complex.create({ real: 3, imag: 3 });
          let addPtr: number = LibFFT.symbols.complex_add(a, b);
          let addVal = Complex.fromPtr(addPtr);
          console.log('complex_add:', addVal.real, addVal.imag);
          LibFFT.symbols.complex_mul.async(a, b)
            .then((mulPtr) => {
              let mulVal = Complex.fromPtr(mulPtr);
              console.log('complex_mul', mulVal.real, mulVal.imag);
            })
            .finally(() => {
              LibFFT.close();
            });
          console.log('LibFFT.symbols.complex_mul.async');
        })
      Button('complex')
        .onClick(() => {
          const Complex = Struct({ real: FFIType.double, imag: FFIType.double });
          let lib = dlopen(LIB_NAME_FFT, {
            fft: { args: [FFIType.ptr, FFIType.int32], returns: FFIType.void }
          });
          // 1. 分配 n 个 complex_t 的连续 buffer
          // 1. 分配 n 个 complex_t 的连续 buffer
          let n = 4;
          let buf = new ArrayBuffer(Complex.size * n);
          let view = new DataView(buf);
          // 2. 填入输入数据
          // complex_t[0] = {1, 2}, [1] = {3, 4}, [2] = {5, 6}, [3] = {7, 8}
          for (let i = 0; i < n; i++) {
            view.setFloat64(i * Complex.size, i * 2 + 1, true); // real
            view.setFloat64(i * Complex.size + 8, i * 2 + 2, true); // imag
          }
          // 3. 获取指针并调用 fft
          let dataPtr = ffi.ptr(buf);
          lib.symbols.fft(dataPtr, n);
          // 4. 读取结果（fft 原地改写）
          for (let i = 0; i < n; i++) {
            let result = Complex.fromPtr(dataPtr, i * Complex.size);
            console.log('result[' + i + ']:', result.real, result.imag);
          }
        })
    }
    .height('100%')
    .width('100%')
  }
}