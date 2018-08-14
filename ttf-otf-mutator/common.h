/////////////////////////////////////////////////////////////////////////
//
// Author: Mateusz Jurczyk (mjurczyk@google.com)
//
// Copyright 2018 Google LLC
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// https://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef COMMON_H_
#define COMMON_H_

#if defined(clang) || defined(__GNUC__)
#define SWAP32(value) __builtin_bswap32(value)
#define SWAP16(value) __builtin_bswap16(value)
#elif defined(_MSC_VER)
#define SWAP32(value) _byteswap_ulong(value)
#define SWAP16(value) _byteswap_ushort(value)
#endif

#endif  // COMMON_H_

