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
import ffi from 'liblibrary.so';

type LooseFFIDef = {
  args: string[];
  returns: string;
};

type ConvertFns<Fns extends Record<string, LooseFFIDef>> = {
  [K in keyof Fns]: (...args: any[]) => number;
};

export class FFIType {
  static readonly int32: 'i' = 'i';
  static readonly int64: 'l' = 'l';
  static readonly double: 'd' = 'd';
  static readonly CString: 's' = 's';

  static readonly char: 'c' = 'c';
  static readonly int8_t: 'c' = 'c';
  static readonly i8: 'c' = 'c';

  static readonly uint8_t: 'i' = 'i';
  static readonly u8: 'i' = 'i';

  static readonly int16_t: 'i' = 'i';
  static readonly i16: 'i' = 'i';

  static readonly uint16_t: 'i' = 'i';
  static readonly u16: 'i' = 'i';

  static readonly int: 'i' = 'i';
  static readonly i32: 'i' = 'i';
  static readonly uint32_t: 'i' = 'i';
  static readonly u32: 'i' = 'i';

  static readonly i64: 'l' = 'l';
  static readonly int64_t: 'l' = 'l';
  static readonly uint64_t: 'l' = 'l';
  static readonly u64: 'l' = 'l';

  static readonly f64: 'd' = 'd';
  static readonly float: 'f' = 'f';
  static readonly f32: 'f' = 'f';

  static readonly bool: 'b' = 'b';

  static readonly ptr: 'p' = 'p';
  static readonly pointer: 'p' = 'p';

  static readonly void: 'i' = 'i';

  static readonly i64_fast: 'l' = 'l';
  static readonly u64_fast: 'l' = 'l';
  static readonly function: 'p' = 'p';
  static readonly napi_env: 'p' = 'p';
  static readonly napi_value: 'p' = 'p';
  static readonly buffer: 'p' = 'p';
}

export class CString {
  private ptr: number;

  constructor(ptr: number) {
    this.ptr = ptr;
  }

  get length(): number {
    return this.toString().length;
  }

  toString(): string {
    return ffi.readCString(this.ptr);
  }
}

export function CFunction(def: { args: string[]; returns: string; ptr: number }): {
  (...args: any[]): number;
  close(): void;
} {
  let typeStr: string = joinTypes(def.args);
  let wrapper: any = (...rawArgs: any[]): number => {
    let numArgs: number[] = [];
    let strArgs: string[] = [];
    for (let j = 0; j < def.args.length; j++) {
      if (def.args[j] == 's') {
        strArgs.push(rawArgs[j] as string);
      } else {
        numArgs.push(rawArgs[j] as number);
      }
    }
    return ffi.callPtr(def.ptr, typeStr, def.returns, numArgs, strArgs);
  };
  wrapper.close = (): void => {
  };
  return wrapper;
}

export class JSCallback {
  readonly threadsafe: boolean;
  private handle: number;

  constructor(callback: (...args: any[]) => any, def: { args: string[]; returns: string; threadsafe?: boolean }) {
    this.handle = ffi.createCallback(callback, joinTypes(def.args), def.returns, !!def.threadsafe);
    this.threadsafe = !!def.threadsafe;
  }

  get ptr(): number {
    return this.handle;
  }

  call(...args: any[]): any {
    return ffi.invokeCallback(this.handle, ...args);
  }

  close(): void {
    if (this.handle !== 0) {
      ffi.destroyCallback(this.handle);
      this.handle = 0;
    }
  }
}

function joinTypes(types: string[]): string {
  let result: string = '';
  for (let i = 0; i < types.length; i++) {
    result += types[i];
  }
  return result;
}

export class Library<Fns extends Record<string, LooseFFIDef>> {
  readonly symbols: ConvertFns<Fns>;
  private handle: bigint;

  constructor(handle: bigint, symbols: ConvertFns<Fns>) {
    this.handle = handle;
    this.symbols = symbols;
  }

  close(): void {
    ffi.close(this.handle);
  }
}

export function dlopen<Fns extends Record<string, LooseFFIDef>>(
  libName: string,
  defs: Fns,
): Library<Fns> {
  let handle: bigint = ffi.load(libName);
  let symbols: Record<string, (...args: any[]) => any> = {};

  let keys: string[] = Object.keys(defs);
  for (let i = 0; i < keys.length; i++) {
    let name: string = keys[i];
    let def: LooseFFIDef = defs[name];
    let typeStr: string = joinTypes(def.args);
    ffi.defineFunction(handle, name, typeStr, def.returns);

    Object.defineProperty(symbols, name, {
      value: (...rawArgs: any[]): any => {
        let numArgs: number[] = [];
        let strArgs: string[] = [];
        for (let j = 0; j < def.args.length; j++) {
          if (def.args[j] == 's') {
            strArgs.push(rawArgs[j] as string);
          } else {
            numArgs.push(rawArgs[j] as number);
          }
        }
        return ffi.callBySig(handle, name, numArgs, strArgs);
      },
      writable: true,
      enumerable: true,
      configurable: true,
    });
  }

  return new Library<Fns>(handle, symbols as ConvertFns<Fns>);
}
