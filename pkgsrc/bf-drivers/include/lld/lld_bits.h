/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


#ifndef lld_bits_h
#define lld_bits_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/** BITS64:
 *  Simple access macro to extract a bit field from a 64 bit value.
 * This is useful when processing the DRs. Added here as it is used
 * in several sections below.
 */
#define BITS64(x, hi, lo) ((x << (63LL - hi)) >> (63LL - hi + lo))

#define extract_bit_fld_128(wd1, wd0, hi, lo)                             \
  (hi < 64)                                                               \
      ? ((wd0 << (63 - hi)) >> (63 - hi + lo))                            \
      : (lo > 63)                                                         \
            ? ((wd1 << (63 - (hi - 64))) >> (63 - (hi - 64) + (lo - 64))) \
            : ((wd0 >> lo) |                                              \
               (wd1 << (63 - (hi - 64))) >> ((63 - (hi - 64)) - (64 - lo)))

#define FLD_MSK(hi, lo) (((-1ULL << (63 - hi)) >> (63 - hi + lo)) << lo)
#define insert_bit_fld_128(wd1, wd0, hi, lo, fld)                           \
  {                                                                         \
    if (hi < 64) {                                                          \
      wd0 = ((wd0 & ~(FLD_MSK(hi, lo)))) | ((fld << lo) & FLD_MSK(hi, lo)); \
    }                                                                       \
  }

#ifdef __cplusplus
}
#endif /* C++ */

#endif
