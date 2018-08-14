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

#ifndef MUTATOR_H_
#define MUTATOR_H_

#include <vector>
#include <string>

#define MIN3(a, b, c) ((((a) < (b) ? (a) : (b)) < (c)) ? ((a) < (b) ? (a) : (b)) : (c))

enum MutationType {
  MUTATION_BITFLIPPING,
  MUTATION_BYTEFLIPPING,
  MUTATION_CHUNKSPEW,
  MUTATION_SPECIAL_INTS,
  MUTATION_ADD_SUB_BINARY
};

struct MutationStrategy {
  enum MutationType type;
  double min_mutation_ratio;
  double max_mutation_ratio;
};

class Mutator {
 public:
  static void MutateString(const std::vector<MutationStrategy>& strategies, std::string *buffer, unsigned int *changed_bytes);
  static void MutateString(MutationType type, double ratio, std::string *buffer, unsigned int *changed_bytes);

 private:
  static void Bitflipping(std::string *buffer, double ratio, unsigned int *changed_bytes);
  static void Byteflipping(std::string *buffer, double ratio, unsigned int *changed_bytes);
  static void Chunkspew(std::string *buffer, double ratio, unsigned int *changed_bytes);
  static void SpecialInts(std::string *buffer, double ratio, unsigned int *changed_bytes);
  static void AddSubBinary(std::string *buffer, double ratio, unsigned int *changed_bytes);
};

#endif  // MUTATOR_H_
