export const load: (libName: string) => bigint;

export const close: (handle: bigint) => void;

export const defineFunction: (handle: bigint, funcName: string, argTypes: string, returnType: string) => void;

export const callBySig: (handle: bigint, funcName: string, numArgs: number[], strArgs: string[]) => number;

export const callMixed: (handle: bigint, funcName: string, argTypes: string, returnType: string, numArgs: number[],
  strArgs: string[]) => number;

export const callString: (handle: bigint, funcName: string) => string;

export const readCString: (ptr: number) => string;

export const callPtr: (ptr: number, argTypes: string, returnType: string, numArgs: number[],
  strArgs: string[]) => number;

export const getSymbolPtr: (handle: bigint, funcName: string) => number;

export const createCallback: (callback: (...args: any[]) => any, argTypes: string, returnType: string,
  threadsafe?: boolean) => number;

export const destroyCallback: (handle: number) => void;

export const invokeCallback: (handle: number, ...args: any[]) => any;

export const getCallbackThreadsafe: (handle: number) => boolean;

export const getCallbackPtr: (handle: number) => number;

export const callCallbackThreadSafe: (handle: number, arg: number) => void;

export const ptr: (buffer: ArrayBuffer | TypedArray) => number;
