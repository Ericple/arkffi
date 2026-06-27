import ffi from 'liblibrary.so';

type LooseFFIDef = {
  args: (string | StructSchema)[];
  returns: string | StructSchema;
};

interface AsyncableFunction {
  (...args: any[]): any;
  async: (...args: any[]) => Promise<number>;
}

type ConvertFns<Fns extends Record<string, LooseFFIDef>> = {
  [K in keyof Fns]: AsyncableFunction;
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

  static readonly callback: 'k' = 'k';
  static readonly usize: 'l' = 'l';
}

export class CString {
  private ptr: number;
  private byteOffset: number;
  private byteLength: number;

  constructor(ptr: number, byteOffset?: number, byteLength?: number) {
    this.ptr = ptr;
    this.byteOffset = byteOffset ?? 0;
    this.byteLength = byteLength ?? -1;
  }

  get length(): number {
    return this.toString().length;
  }

  toString(): string {
    if (this.byteLength >= 0) {
      return ffi.readCString(this.ptr + this.byteOffset);
    }
    return ffi.readCString(this.ptr + this.byteOffset);
  }
}

let g_structResults: Map<number, ArrayBuffer> = new Map();

export function releasePtr(ptr: number): void {
  g_structResults.delete(ptr);
}

function expandFields(schema: StructSchema): string[] {
  let result: string[] = [];
  for (let i = 0; i < schema.fieldTypes.length; i++) {
    let t = schema.fieldTypes[i];
    if (typeof t === 'string') {
      result.push(t);
    } else {
      expandFields(t).forEach(f => result.push(f));
    }
  }
  return result;
}

function returnEncoding(ret: string | StructSchema): string {
  if (typeof ret === 'string') {
    return ret;
  }
  let ft = ret.fieldTypes;
  if (ft.length === 2 && ft[0] === FFIType.double && ft[1] === FFIType.double) {
    return '2';
  }
  if (ft.length === 1 && ft[0] === FFIType.double) {
    return 'd';
  }
  return '2';
}

function unpackStructBuf(buf: ArrayBuffer, schema: StructSchema): number[] {
  let view = new DataView(buf);
  let result: number[] = [];
  for (let i = 0; i < schema.fieldTypes.length; i++) {
    let off = schema.fieldOffsets[i];
    let t = schema.fieldTypes[i];
    if (typeof t !== 'string') {
      let subBuf = buf.slice(off, off + t.size);
      unpackStructBuf(subBuf, t).forEach(v => result.push(v));
    } else if (t === FFIType.double || t === FFIType.float) {
      result.push(view.getFloat64(off, true));
    } else if (t === FFIType.int32 || t === FFIType.bool || t === FFIType.char) {
      result.push(view.getInt32(off, true));
    } else if (t === FFIType.int64 || t === FFIType.ptr || t === FFIType.callback) {
      result.push(Number(view.getBigInt64(off, true)));
    } else {
      result.push(view.getInt32(off, true));
    }
  }
  return result;
}

function joinTypes(types: string[]): string {
  let result: string = '';
  for (let i = 0; i < types.length; i++) {
    result += types[i];
  }
  return result;
}

export function CFunction(def: { args: (string | StructSchema)[]; returns: string | StructSchema; ptr: number }): {
  (...args: any[]): number;
  close(): void;
} {
  let flatArgs: string[] = [];
  for (let j = 0; j < def.args.length; j++) {
    if (typeof def.args[j] === 'string') {
      flatArgs.push(def.args[j] as string);
    } else {
      expandFields(def.args[j] as StructSchema).forEach(t => flatArgs.push(t));
    }
  }
  let typeStr: string = joinTypes(flatArgs);
  let isStructReturn = typeof def.returns !== 'string';
  let retEnc: string = returnEncoding(def.returns);

  let wrapper: any = (...rawArgs: any[]): number => {
    let numArgs: number[] = [];
    let strArgs: string[] = [];
    for (let j = 0; j < def.args.length; j++) {
      let argDef = def.args[j];
      if (argDef === FFIType.CString) {
        strArgs.push(rawArgs[j] as string);
      } else if (typeof argDef !== 'string') {
        unpackStructBuf(rawArgs[j] as ArrayBuffer, argDef as StructSchema).forEach(v => numArgs.push(v));
      } else if (argDef === FFIType.callback) {
        numArgs.push((rawArgs[j] as any).ptr ?? (rawArgs[j] as number));
      } else {
        numArgs.push(rawArgs[j] as number);
      }
    }
    if (isStructReturn) {
      let buf: ArrayBuffer = ffi.callPtr(def.ptr, typeStr, retEnc, numArgs, strArgs) as unknown as ArrayBuffer;
      let ptrOut: number = ffi.ptr(buf);
      g_structResults.set(ptrOut, buf);
      return ptrOut;
    }
    return ffi.callPtr(def.ptr, typeStr, def.returns as string, numArgs, strArgs);
  };
  wrapper.close = (): void => {
  };
  return wrapper;
}

export function ptr(buffer: any): number {
  return ffi.ptr(buffer);
}

export function callAsync(
  handle: bigint,
  funcName: string,
  argTypes: string,
  returnType: string,
  numArgs: number[],
  strArgs: string[],
): Promise<number> {
  return ffi.callAsync(handle, funcName, argTypes, returnType, numArgs, strArgs);
}

export function callBySigAsArrayBuffer(
  handle: bigint,
  funcName: string,
  numArgs: number[],
  strArgs: string[],
): ArrayBuffer {
  let value: any = ffi.callBySig(handle, funcName, numArgs, strArgs);
  return value as ArrayBuffer;
}

export function callPtrAsync(
  ptr: number,
  argTypes: string,
  returnType: string,
  numArgs: number[],
  strArgs: string[],
): Promise<number> {
  return ffi.callPtrAsync(ptr, argTypes, returnType, numArgs, strArgs);
}

export function AsyncCFunction(def: { args: (string | StructSchema)[]; returns: string | StructSchema; ptr: number }): {
  (...args: any[]): Promise<number>;
  close(): void;
} {
  let flatArgs: string[] = [];
  for (let j = 0; j < def.args.length; j++) {
    if (typeof def.args[j] === 'string') {
      flatArgs.push(def.args[j] as string);
    } else {
      expandFields(def.args[j] as StructSchema).forEach(t => flatArgs.push(t));
    }
  }
  let typeStr: string = joinTypes(flatArgs);
  let isStructReturn = typeof def.returns !== 'string';
  let retEnc: string = returnEncoding(def.returns);

  let wrapper: any = (...rawArgs: any[]): Promise<number> => {
    let numArgs: number[] = [];
    let strArgs: string[] = [];
    for (let j = 0; j < def.args.length; j++) {
      let argDef = def.args[j];
      if (argDef === FFIType.CString) {
        strArgs.push(rawArgs[j] as string);
      } else if (typeof argDef !== 'string') {
        unpackStructBuf(rawArgs[j] as ArrayBuffer, argDef as StructSchema).forEach(v => numArgs.push(v));
      } else if (argDef === FFIType.callback) {
        numArgs.push((rawArgs[j] as any).ptr ?? (rawArgs[j] as number));
      } else {
        numArgs.push(rawArgs[j] as number);
      }
    }
    if (isStructReturn) {
      let p: Promise<any> = ffi.callPtrAsync(def.ptr, typeStr, retEnc, numArgs, strArgs);
      return p.then((buf: ArrayBuffer): number => {
        let ptrOut: number = ffi.ptr(buf);
        g_structResults.set(ptrOut, buf);
        return ptrOut;
      });
    }
    return ffi.callPtrAsync(def.ptr, typeStr, def.returns as string, numArgs, strArgs);
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
    return ffi.getCallbackPtr(this.handle);
  }

  getHandle(): number {
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

function extractArg(raw: any, typeCode: string): number {
  if (typeCode == 'k') {
    return raw.ptr ?? raw;
  }
  return raw;
}

const TYPE_SIZE: Record<string, number> = {
  'c': 1,
  'i': 4,
  'l': 8,
  'd': 8,
  'f': 4,
  'b': 1,
  's': 8,
  'p': 8,
  'k': 8,
};

const TYPE_ALIGN: Record<string, number> = {
  'c': 1,
  'i': 4,
  'l': 8,
  'd': 8,
  'f': 4,
  'b': 1,
  's': 8,
  'p': 8,
  'k': 8,
};

export class StructSchema {
  readonly fieldNames: string[];
  readonly fieldTypes: (string | StructSchema)[];
  readonly fieldOffsets: number[];
  readonly size: number;
  readonly alignment: number;

  constructor(fields: Record<string, string | StructSchema>) {
    let names: string[] = [];
    let types: (string | StructSchema)[] = [];
    for (let key in fields) {
      names.push(key);
      types.push(fields[key]);
    }
    this.fieldNames = names;
    this.fieldTypes = types;
    this.fieldOffsets = [];

    let offset = 0;
    let maxAlign = 1;
    for (let i = 0; i < names.length; i++) {
      let t = types[i];
      let align: number;
      let sz: number;
      if (typeof t === 'string') {
        align = TYPE_ALIGN[t] ?? 4;
        sz = TYPE_SIZE[t] ?? 4;
      } else {
        align = t.alignment;
        sz = t.size;
      }
      if (align > maxAlign) {
        maxAlign = align;
      }
      let padding = (align - (offset % align)) % align;
      this.fieldOffsets.push(offset + padding);
      offset += padding + sz;
    }
    let finalPad = (maxAlign - (offset % maxAlign)) % maxAlign;
    this.size = offset + finalPad;
    this.alignment = maxAlign;
  }

  create(obj: Record<string, number | bigint | ArrayBuffer>): ArrayBuffer {
    let buf = new ArrayBuffer(this.size);
    let view = new DataView(buf);
    for (let i = 0; i < this.fieldNames.length; i++) {
      let name = this.fieldNames[i];
      let t = this.fieldTypes[i];
      let off = this.fieldOffsets[i];
      let val = obj[name];
      if (typeof t !== 'string') {
        let subBuf = val as ArrayBuffer;
        let src = new Uint8Array(subBuf);
        let dst = new Uint8Array(buf);
        for (let j = 0; j < subBuf.byteLength; j++) {
          dst[off + j] = src[j];
        }
      } else if (t == 'i' || t == 'b' || t == 'c') {
        view.setInt32(off, val as number, true);
      } else if (t == 'l') {
        view.setBigInt64(off, BigInt(val as number), true);
      } else if (t == 'd') {
        view.setFloat64(off, val as number, true);
      } else if (t == 'f') {
        view.setFloat32(off, val as number, true);
      } else if (t == 's' || t == 'p' || t == 'k') {
        view.setBigInt64(off, BigInt(val as number), true);
      }
    }
    return buf;
  }

  fromPtr(ptr: number, byteOffset?: number): Record<string, number> {
    let base = byteOffset ?? 0;
    let buf = ffi.readMemory(ptr + base, this.size);
    let view = new DataView(buf);
    let result: Record<string, number> = {};
    for (let i = 0; i < this.fieldNames.length; i++) {
      let name = this.fieldNames[i];
      let t = this.fieldTypes[i];
      let off = this.fieldOffsets[i];
      if (typeof t !== 'string') {
        result[name] = ptr + base + off;
      } else if (t == 'i' || t == 'b' || t == 'c') {
        result[name] = view.getInt32(off, true);
      } else if (t == 'l') {
        result[name] = Number(view.getBigInt64(off, true));
      } else if (t == 'd') {
        result[name] = view.getFloat64(off, true);
      } else if (t == 'f') {
        result[name] = view.getFloat32(off, true);
      } else if (t == 's' || t == 'p' || t == 'k') {
        result[name] = Number(view.getBigInt64(off, true));
      }
    }
    return result;
  }

  get(buf: ArrayBuffer, field: string): number {
    let idx = this.fieldNames.indexOf(field);
    if (idx < 0) {
      return 0;
    }
    let view = new DataView(buf);
    let off = this.fieldOffsets[idx];
    let t = this.fieldTypes[idx];
    if (typeof t !== 'string') {
      return 0;
    }
    if (t == 'i' || t == 'b' || t == 'c') {
      return view.getInt32(off, true);
    }
    if (t == 'l') {
      return Number(view.getBigInt64(off, true));
    }
    if (t == 'd') {
      return view.getFloat64(off, true);
    }
    if (t == 'f') {
      return view.getFloat32(off, true);
    }
    if (t == 's' || t == 'p' || t == 'k') {
      return Number(view.getBigInt64(off, true));
    }
    return 0;
  }

  set(buf: ArrayBuffer, field: string, value: number): void {
    let idx = this.fieldNames.indexOf(field);
    if (idx < 0) {
      return;
    }
    let view = new DataView(buf);
    let off = this.fieldOffsets[idx];
    let t = this.fieldTypes[idx];
    if (typeof t !== 'string') {
      return;
    }
    if (t == 'i' || t == 'b' || t == 'c') {
      view.setInt32(off, value, true);
    } else if (t == 'l') {
      view.setBigInt64(off,
        BigInt(value), true);
    } else if (t == 'd') {
      view.setFloat64(off, value, true);
    } else if (t == 'f') {
      view.setFloat32(
        off, value, true);
    } else if (t == 's' || t == 'p' || t == 'k') {
      view.setBigInt64(off, BigInt(value), true);
    }
  }
}

export function Struct(fields: Record<string, string | StructSchema>): StructSchema {
  return new StructSchema(fields);
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
  let symbols: Record<string, AsyncableFunction> = {};

  let keys: string[] = Object.keys(defs);
  for (let i = 0; i < keys.length; i++) {
    let name: string = keys[i];
    let def: LooseFFIDef = defs[name];

    let flatArgs: string[] = [];
    for (let j = 0; j < def.args.length; j++) {
      let argDef = def.args[j];
      if (typeof argDef === 'string') {
        flatArgs.push(argDef);
      } else {
        expandFields(argDef as StructSchema).forEach(t => flatArgs.push(t));
      }
    }
    let typeStr: string = joinTypes(flatArgs);
    let retEnc: string = returnEncoding(def.returns);
    let isStructReturn = typeof def.returns !== 'string';

    ffi.defineFunction(handle, name, typeStr, retEnc);

    Object.defineProperty(symbols, name, {
      value: (...rawArgs: any[]): any => {
        let numArgs: number[] = [];
        let strArgs: string[] = [];
        for (let j = 0; j < def.args.length; j++) {
          let argDef = def.args[j];
          if (argDef === FFIType.CString) {
            strArgs.push(rawArgs[j] as string);
          } else if (typeof argDef !== 'string') {
            unpackStructBuf(rawArgs[j] as ArrayBuffer, argDef as StructSchema).forEach(v => numArgs.push(v));
          } else {
            numArgs.push(extractArg(rawArgs[j], argDef));
          }
        }
        if (isStructReturn) {
          let buf: ArrayBuffer = ffi.callBySig(handle, name, numArgs, strArgs) as unknown as ArrayBuffer;
          let ptrOut: number = ffi.ptr(buf);
          g_structResults.set(ptrOut, buf);
          return ptrOut;
        }
        return ffi.callBySig(handle, name, numArgs, strArgs);
      },
      writable: true,
      enumerable: true,
      configurable: true,
    });

    Object.defineProperty(symbols[name], 'async', {
      value: (...rawArgs: any[]): Promise<number> => {
        let numArgs: number[] = [];
        let strArgs: string[] = [];
        for (let j = 0; j < def.args.length; j++) {
          let argDef = def.args[j];
          if (argDef === FFIType.CString) {
            strArgs.push(rawArgs[j] as string);
          } else if (typeof argDef !== 'string') {
            unpackStructBuf(rawArgs[j] as ArrayBuffer, argDef as StructSchema).forEach(v => numArgs.push(v));
          } else {
            numArgs.push(extractArg(rawArgs[j], argDef));
          }
        }
        if (isStructReturn) {
          let p: Promise<any> = ffi.callAsync(handle, name, typeStr, retEnc, numArgs, strArgs);
          return p.then((buf: ArrayBuffer): number => {
            let ptrOut: number = ffi.ptr(buf);
            g_structResults.set(ptrOut, buf);
            return ptrOut;
          });
        }
        return ffi.callAsync(handle, name, typeStr, retEnc, numArgs, strArgs);
      },
      writable: true,
      enumerable: true,
      configurable: true,
    });
  }

  return new Library<Fns>(handle, symbols as ConvertFns<Fns>);
}
