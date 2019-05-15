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

#include "random.h"

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <time.h>

namespace globals {
  std::mt19937 generator;
  std::uniform_int_distribution<unsigned int> int_distrib(0, UINT_MAX);
  std::uniform_real_distribution<double> real_distrib(0.0, 1.0);
}  // namespace globals

static void EnsureSeeded() {
  static bool seeded = false;
  if (!seeded) {
    std::random_device rd;
    globals::generator.seed(rd());

    seeded = true;
  }
}

uint32_t Random32() {
  EnsureSeeded();
  return globals::int_distrib(globals::generator);
}

double RandomFloat() {
  EnsureSeeded();
  return globals::real_distrib(globals::generator);
}
