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
