import { ffi, JSCallback, CFunction, FFIType, AsyncCFunction, Struct, dlopen } from 'arkffi';

const LIB_NAME = 'libffi_target.so';
const LIB_NAME_FFT = 'libfft.so';

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
      Button('fft')
        .onClick(() => {
          let Complex = Struct({
            real: FFIType.double,
            imag: FFIType.double,
          });
          let lib = dlopen(LIB_NAME_FFT, {
            complex_new: { args: [FFIType.double, FFIType.double], returns: Complex },
            complex_add: { args: [Complex, Complex], returns: Complex },
            complex_mul: { args: [Complex, Complex], returns: Complex },
          });

          let newPtr: number = lib.symbols.complex_new(3.0, 2.0);
          let newVal = Complex.fromPtr(newPtr);
          console.log('complex_new:', newVal.real, newVal.imag);

          let a = Complex.create({ real: 4, imag: 1 });
          let b = Complex.create({ real: 3, imag: 3 });
          let addPtr: number = lib.symbols.complex_add(a, b);
          let addVal = Complex.fromPtr(addPtr);
          console.log('complex_add:', addVal.real, addVal.imag);
          let mulPtr: number = lib.symbols.complex_mul(a, b);
          let mulVal = Complex.fromPtr(mulPtr);
          console.log('complex_mul', mulVal.real, mulVal.imag);
          lib.close();
        })
    }
    .height('100%')
    .width('100%')
  }
}