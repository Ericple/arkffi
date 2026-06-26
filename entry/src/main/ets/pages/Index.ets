import { ffi, JSCallback, CFunction, FFIType, AsyncCFunction } from 'arkffi';

const LIB_NAME = 'libffi_target.so';

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
    }
    .height('100%')
    .width('100%')
  }
}