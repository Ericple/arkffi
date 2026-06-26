import { ffi, JSCallback, CFunction, FFIType } from 'arkffi';

const LIB_NAME = 'libffi_target.so';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize($r('app.float.page_text_font_size'))
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
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
    }
    .height('100%')
    .width('100%')
  }
}