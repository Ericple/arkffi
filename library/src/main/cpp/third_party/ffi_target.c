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
#include <string.h>
#include "ffi_target.h"

double add(double a, double b) { return a + b; }

double multiply(double a, double b) { return a * b; }

double divide(double a, double b) {
    if (b == 0.0) {
        return 0.0;
    }
    return a / b;
}

long long factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    long long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

const char *getVersion(void) { return "1.0.0"; }

double compute(int mode, double value, const char *name) {
    if (strcmp(name, "square") == 0) {
        return value * value;
    }
    if (strcmp(name, "half") == 0) {
        return value / 2.0;
    }
    if (mode == 1) {
        return value * 2;
    }
    return value;
}

int32_t compareStrings(const char *a, const char *b) { return strcmp(a, b); }

double weightedSum(int32_t a, double wa, int32_t b, double wb) { return (double)a * wa + (double)b * wb; }

int32_t apply_callback(int32_t (*cb)(int32_t), int32_t arg) { return cb(arg); }
