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
#ifndef FFI_TARGET_H
#define FFI_TARGET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

double add(double a, double b);
double multiply(double a, double b);
double divide(double a, double b);
long long factorial(int n);
int gcd(int a, int b);
const char* getVersion(void);

double compute(int mode, double value, const char* name);

int32_t compareStrings(const char* a, const char* b);

double weightedSum(int32_t a, double wa, int32_t b, double wb);

int32_t apply_callback(int32_t (*cb)(int32_t), int32_t arg);

#ifdef __cplusplus
}
#endif

#endif
