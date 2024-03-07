// -*- mode:c++;indent-tabs-mode:nil;c-basic-offset:4;coding:utf-8 -*-
// vi: set et ft=c++ ts=4 sts=4 sw=4 fenc=utf-8 :vi
//
// Copyright 2024 Mozilla Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "macros.h"

//
//                 _   _          ___ _      _   ___
//                | |_(_)_ _ _  _| _ ) |    /_\ / __|
//                |  _| | ' \ || | _ \ |__ / _ \\__ \.
//                 \__|_|_||_\_, |___/____/_/ \_\___/
//                           |__/
//
//                  BASIC LINEAR ALGEBRA SUBPROGRAMS
//

// naive abstract generic matrix multiplication for gpu
//
// this matmul reference implementation is guaranteed to be bug-free and
// can be used for finding bugs in more advanced matmul implementations.

namespace naive {

__device__ __forceinline__ half madd(half x, half y, half z) {
    return (float)x * (float)y + (float)z;
}
__device__ __forceinline__ float madd(float x, float y, float z) {
    return __fmaf_rn(x, y, z);
}
__device__ __forceinline__ double madd(double x, double y, double z) {
    return __fma_rn(x, y, z);
}

template <typename T, typename TA, typename TB, typename TC>
__global__ void kernel(bool aᵀ, bool bᵀ, //
                       int m, int n, int k, T α, //
                       const TA *A, int lda, //
                       const TB *B, int ldb, T β, //
                       TC *C, int ldc) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const int j = blockIdx.y * blockDim.y + threadIdx.y;
    if (i < m && j < n) {
        T d = 0;
        for (int l = 0; l < k; ++l) {
            T a = aᵀ ? A[lda * i + l] : A[lda * l + i];
            T b = bᵀ ? B[ldb * l + j] : B[ldb * j + l];
            d = madd(a, b, d);
        }
        T c = C[ldc * j + i];
        C[ldc * j + i] = madd(α, d, β * c);
    }
}

// multiplies matrix on gpu with column major ordering
//
//     m×k * k×n → m×n
//     k×m * k×n → m×n if aᵀ
//     m×k * n×k → m×n if bᵀ
//     k×m * n×k → m×n if aᵀ and bᵀ
//
template <typename T, typename TA, typename TB, typename TC>
cudaError_t gemm(cudaStream_t stream, //
                 bool aᵀ, bool bᵀ, //
                 int m, int n, int k, T α, //
                 const TA *A, int lda, //
                 const TB *B, int ldb, T β, //
                 TC *C, int ldc) {
    dim3 blockDim(32, 32);
    dim3 gridDim(CEIL_DIV(m, 32), CEIL_DIV(n, 32));
    kernel<<<gridDim, blockDim, 0, stream>>>(aᵀ, bᵀ, m, n, k, α, A, lda, B, ldb, β, C, ldc);
    return cudaGetLastError();
}

} // namespace naive